// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include "smi_drv.h"

#include <drm/drm_crtc_helper.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
#include <drm/drmP.h>
#endif
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_plane_helper.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
#include <linux/iosys-map.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
#include <linux/dma-buf-map.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
#include <drm/drm_framebuffer.h>
#endif

#include "smi_dbg.h"

#include "hw750.h"
#include "hw768.h"
#include "ddk768/ddk768_video.h"


__attribute__((unused)) static void colorcur2monocur(void *data);

static const uint32_t smi_cursor_plane_formats[] = { DRM_FORMAT_RGB565, DRM_FORMAT_BGR565,
						     DRM_FORMAT_ARGB8888 };

static const uint32_t smi_formats[] = { DRM_FORMAT_RGB565,   DRM_FORMAT_BGR565,
					DRM_FORMAT_RGB888,
					DRM_FORMAT_XRGB8888,
					DRM_FORMAT_RGBA8888,
					DRM_FORMAT_ARGB8888};


static int smi_cursor_atomic_check(struct drm_plane *plane, 
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)  
				struct drm_plane_state *state
#else
				struct drm_atomic_state *atom_state
#endif
)
{
#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
#endif
	struct drm_crtc *crtc = state->crtc;
	int src_w, src_h;

	if (!crtc || !state->fb)
		return 0;

	src_w = (state->src_w >> 16);
	src_h = (state->src_h >> 16);
	if (src_w > 64 || src_h > 64) {
		DRM_ERROR("Invalid cursor size (%dx%d)\n", src_w, src_h);
		return -EINVAL;
	}

	dbg_msg("(%dx%d)@(%dx%d)\n", state->crtc_w, state->crtc_h, state->crtc_x, state->crtc_y);
	return 0;
}

static void smi_cursor_atomic_update(struct drm_plane *plane, 
#if KERNEL_VERSION(5, 13, 0) >  LINUX_VERSION_CODE
				 struct drm_plane_state *old_state
#else
				struct drm_atomic_state *atom_state
#endif
)
{
#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atom_state, plane);
	struct drm_plane_state *new_state = drm_atomic_get_new_plane_state(atom_state, plane);
#else
	struct drm_plane_state *new_state = plane->state;
#endif
	
	struct drm_crtc *crtc = new_state->crtc;
	struct drm_framebuffer *fb = new_state->fb;
	void *plane_addr;
	u64 cursor_offset;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	struct drm_gem_vram_object *gbo;
#else
	struct smi_bo *bo;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
	struct iosys_map map;
	int ret;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	struct dma_buf_map map;
	int ret;
#endif

	int x, y;
	disp_control_t disp_ctrl;
	int i, ctrl_index = 0, max_enc = 0;	
	struct smi_device *sdev = plane->dev->dev_private;
	

	if (!crtc || !fb)
		return;


	if(sdev->specId == SPC_SM750)
		max_enc = MAX_CRTC;
	else
		max_enc = MAX_ENCODER;

	for(i = 0;i < max_enc; i++)
	{
		if(crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
		disp_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;

	if(ctrl_index >= MAX_CRTC)  //calc which path should we use for HDMI.
	{
		disp_ctrl = (disp_control_t)smi_calc_hdmi_ctrl(sdev->m_connector);
	}


	if (fb != old_state->fb) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
		gbo = drm_gem_vram_of_gem(fb->obj[0]);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
		if (drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM) < 0) {
			dbg_msg("vram_pin failed\n");
			LEAVE();
		}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0) */

		cursor_offset = drm_gem_vram_offset(gbo);
		if (cursor_offset < 0) {
			dbg_msg("get gpu_addr failed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
			drm_gem_vram_unpin(gbo);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0) */
			LEAVE();
		}
		if (sdev->specId == SPC_SM750) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
			ret = drm_gem_vram_vmap(gbo, &map);
			
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
			plane_addr = drm_gem_vram_vmap(gbo);
#else
			plane_addr = drm_gem_vram_kmap(gbo, true, NULL);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
			if (ret) {				
#else
			if (IS_ERR(plane_addr)) {
#endif
				dbg_msg("failed to map fbcon\n");
			} else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)			
				plane_addr = map.vaddr;		
#endif
				ddk750_initCursor(disp_ctrl, (u32)cursor_offset, BPP16_BLACK,
						  BPP16_WHITE, BPP16_BLUE);
				colorcur2monocur(plane_addr);
				
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
				drm_gem_vram_vunmap(gbo, &map);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
				drm_gem_vram_vunmap(gbo, plane_addr);	
#else
				drm_gem_vram_kunmap(gbo);
#endif
				ddk750_enableCursor(disp_ctrl, 1);
			}
		} else {
			ddk768_initCursor(disp_ctrl, (u32)cursor_offset, BPP32_BLACK, BPP32_WHITE,
					  BPP32_BLUE);
			ddk768_enableCursor(disp_ctrl, 3);
		}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
		drm_gem_vram_unpin(gbo);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0) */
#else
		bo = gem_to_smi_bo(fb->obj[0]);
		if (smi_bo_pin(bo, TTM_PL_FLAG_VRAM, &cursor_offset) < 0) {
			dbg_msg("smi_bo_pin failed\n");
			LEAVE();
		}
		if (sdev->specId == SPC_SM750) {
			plane_addr = smi_bo_kmap(bo, true, NULL);
			if (IS_ERR(plane_addr))
				dbg_msg("failed to kmap fbcon\n");
			else {
				ddk750_initCursor(disp_ctrl, (u32)cursor_offset, BPP16_BLACK,
						  BPP16_WHITE, BPP16_BLUE);
				colorcur2monocur(bo->kmap.virtual);
				smi_bo_kunmap(bo);
				ddk750_enableCursor(disp_ctrl, 1);
			}
		} else if (sdev->specId == SPC_SM768) {
			ddk768_initCursor(disp_ctrl, (u32)cursor_offset, BPP32_BLACK, BPP32_WHITE,
					  BPP32_BLUE);
			ddk768_enableCursor(disp_ctrl, 3);
		}
		smi_bo_unpin(bo);
#endif
	}

	x = new_state->crtc_x;
	y = new_state->crtc_y;

	if (sdev->specId == SPC_SM750) {
		ddk750_setCursorPosition(disp_ctrl, x < 0 ? -x : x, y < 0 ? -y : y, y < 0 ? 1 : 0,
					 x < 0 ? 1 : 0);
	} else if (sdev->specId == SPC_SM768) {
		ddk768_setCursorPosition(disp_ctrl, x < 0 ? -x : x, y < 0 ? -y : y, y < 0 ? 1 : 0,
					 x < 0 ? 1 : 0);
	}
}

static void smi_cursor_atomic_disable(struct drm_plane *plane, 
#if KERNEL_VERSION(5, 13, 0) >  LINUX_VERSION_CODE
				struct drm_plane_state *old_state
#else
				struct drm_atomic_state *atom_state
#endif
)
{
#if KERNEL_VERSION(5, 13, 0) <= LINUX_VERSION_CODE
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atom_state, plane);
#endif
	disp_control_t disp_ctrl;
	int i, ctrl_index = 0, max_enc = 0;	
	struct smi_device *sdev = plane->dev->dev_private;	
	
	if (!old_state || !old_state->crtc) {
		dbg_msg("drm plane:%d not enabled\n", plane->base.id);
		return;
	}

	
	if(sdev->specId == SPC_SM750)
		max_enc = MAX_CRTC;
	else
		max_enc = MAX_ENCODER;

	for(i = 0;i < max_enc; i++)
	{
		if(old_state->crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
		disp_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;

	if(ctrl_index >= MAX_CRTC)  //calc which path should we use for HDMI.
	{
		disp_ctrl = (disp_control_t)smi_calc_hdmi_ctrl(sdev->m_connector);
	}

	
	if (sdev->specId == SPC_SM750) {
		ddk750_enableCursor(disp_ctrl, 0);
	} else {
		ddk768_enableCursor(disp_ctrl, 0);
	}
}


static const struct drm_plane_helper_funcs smi_cursor_helper_funcs = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
	.prepare_fb = drm_gem_vram_plane_helper_prepare_fb,
	.cleanup_fb = drm_gem_vram_plane_helper_cleanup_fb,
#endif
	.atomic_check = smi_cursor_atomic_check,
	.atomic_update = smi_cursor_atomic_update,
	.atomic_disable = smi_cursor_atomic_disable,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
__attribute__((unused)) static int smi_primary_plane_prepare_fb(struct drm_plane *plane, struct drm_plane_state *new_state)
{

	size_t i;
	int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	struct drm_gem_vram_object *gbo;
#else
	struct smi_bo *bo;
#endif

	ENTER();
	if (!new_state->fb)
		LEAVE(0);

	for (i = 0; i < ARRAY_SIZE(new_state->fb->obj); ++i) {
		if (!new_state->fb->obj[i])
			continue;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
		gbo = drm_gem_vram_of_gem(new_state->fb->obj[i]);
		ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
		if (ret < 0)
			goto err_drm_gem_vram_unpin;
#else
		bo = gem_to_smi_bo(new_state->fb->obj[i]);
		ret = smi_bo_pin(bo, TTM_PL_FLAG_VRAM, NULL);
		if (ret)
			goto err_smi_bo_unpin;
#endif
	}

	LEAVE(0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
err_drm_gem_vram_unpin:
	while (i) {
		--i;
		DRM_INFO("drm_gem_vram_pin failed!");
		gbo = drm_gem_vram_of_gem(new_state->fb->obj[i]);
		drm_gem_vram_unpin(gbo);
	}
#else
err_smi_bo_unpin:
	while (i) {
		--i;
		bo = gem_to_smi_bo(new_state->fb->obj[i]);
		smi_bo_unpin(bo);
	}
#endif
	LEAVE(ret);

}

__attribute__((unused)) static void smi_primary_plane_cleanup_fb(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	size_t i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	struct drm_gem_vram_object *gbo;
#else
	struct smi_bo *bo;
#endif

	ENTER();
	if (!old_state->fb)
		LEAVE();

	for (i = 0; i < ARRAY_SIZE(old_state->fb->obj); ++i) {
		if (!old_state->fb->obj[i])
			continue;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
		gbo = drm_gem_vram_of_gem(old_state->fb->obj[i]);
		drm_gem_vram_unpin(gbo);
#else
		bo = gem_to_smi_bo(old_state->fb->obj[i]);
		smi_bo_unpin(bo);
#endif
	}
	LEAVE();
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0) */

static void smi_primary_plane_atomic_update(struct drm_plane *plane,
#if KERNEL_VERSION(5, 13, 0) >  LINUX_VERSION_CODE
				 struct drm_plane_state *old_state
#else
				struct drm_atomic_state *atom_state
#endif
)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state,
									   plane);
#else
	struct drm_plane_state *state = plane->state;
#endif
	disp_control_t disp_ctrl;
	struct drm_framebuffer *fb = state->fb;
	u64 plane_addr;
	uint32_t w, h, x, y;
	uint32_t dst_x, dst_y;
	uint32_t offset;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	struct drm_gem_vram_object *gbo;
#else
	struct smi_bo *bo;
#endif
	int i, ctrl_index = 0, max_enc = 0;	
	struct smi_device *sdev = plane->dev->dev_private;
	ENTER();
	if (!state->crtc || !state->fb)
		LEAVE();

	dbg_msg("source: (%u, %u) - (%u, %u)\n", (state->src_x >> 16),
		(state->src_y >> 16), (state->src_w >> 16),
		(state->src_h >> 16));
	dbg_msg("dest: (%d, %d) - (%d, %d)\n", state->crtc_x, state->crtc_y,
		state->crtc_w, state->crtc_h);

	if (!state->visible)
		LEAVE();

	

	if(sdev->specId == SPC_SM750)
		max_enc = MAX_CRTC;
	else
		max_enc = MAX_ENCODER;

	for(i = 0;i < max_enc; i++)
	{
		if(state->crtc == sdev->smi_enc_tab[i]->crtc)
		{
			ctrl_index = i;
			break;
		}
	}
		disp_ctrl = (ctrl_index == CHANNEL1_CTRL)?CHANNEL1_CTRL:CHANNEL0_CTRL;

	if(ctrl_index >= MAX_CRTC)  //calc which path should we use for HDMI.
	{
		disp_ctrl = (disp_control_t)smi_calc_hdmi_ctrl(sdev->m_connector);
	}


	x = (state->src_x >> 16);
	y = (state->src_y >> 16);
	w = (state->src_w >> 16);
	h = (state->src_h >> 16);
	dst_x = (state->crtc_x >> 16);
	dst_y = (state->crtc_y >> 16);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	gbo = drm_gem_vram_of_gem(fb->obj[0]);
	plane_addr = drm_gem_vram_offset(gbo);
#else
	bo = gem_to_smi_bo(fb->obj[0]);
	smi_bo_pin(bo, TTM_PL_FLAG_VRAM, &plane_addr);
#endif

	/* For PRIME imported buffers, blit source GPU data into local VRAM only when fb changes */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	{
		struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atom_state, plane);
		if (fb->obj[0]->import_attach && old_state && fb != old_state->fb) {
			struct drm_clip_rect clip = { 0, 0, fb->width, fb->height };
			smi_handle_damage(fb, clip);
		}
	}
#else
	if (fb->obj[0]->import_attach && fb != old_state->fb) {
		struct drm_clip_rect clip = { 0, 0, fb->width, fb->height };
		smi_handle_damage(fb, clip);
	}
#endif
	fb->pitches[0] = (fb->pitches[0] + 15) & ~15;

	offset = plane_addr + y * fb->pitches[0] + x * fb->format->cpp[0];
	if (sdev->specId == SPC_SM750) {
		hw750_set_base(disp_ctrl, fb->pitches[0], offset);
	} else if (sdev->specId == SPC_SM768) {
		hw768_set_base(disp_ctrl, fb->pitches[0], offset);
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
	smi_bo_unpin(bo);
#endif
	LEAVE();
}

static int smi_primary_plane_atomic_check(struct drm_plane *plane, 
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state
#else
	struct drm_atomic_state *atom_state
#endif
)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
#endif
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;

	ENTER();
	
	if (!crtc)
		LEAVE(0);
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
#else
	crtc_state = drm_atomic_get_crtc_state(atom_state, crtc);
#endif	
	if (IS_ERR(crtc_state))
		LEAVE(PTR_ERR(crtc_state));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	LEAVE(drm_atomic_helper_check_plane_state(state, crtc_state, DRM_PLANE_NO_SCALING,
						  DRM_PLANE_NO_SCALING, false, true));
#else
	LEAVE(drm_atomic_helper_check_plane_state(state, crtc_state, DRM_PLANE_HELPER_NO_SCALING,
						  DRM_PLANE_HELPER_NO_SCALING, false, true));
#endif
}

static const struct drm_plane_helper_funcs smi_primary_plane_helper_funcs = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
	.prepare_fb = drm_gem_vram_plane_helper_prepare_fb,
#else
	.prepare_fb = smi_primary_plane_prepare_fb,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
	.cleanup_fb = drm_gem_vram_plane_helper_cleanup_fb,
#else
	.cleanup_fb = smi_primary_plane_cleanup_fb,
#endif
	.atomic_check = smi_primary_plane_atomic_check,
	.atomic_update = smi_primary_plane_atomic_update,
};

static const struct drm_plane_funcs smi_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.reset = drm_atomic_helper_plane_reset,
	.destroy = drm_plane_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state
};

/* ------------------------------------------------------------------ */
/* SM768 hardware video overlay plane (Xv / KMS overlay)              */
/* ------------------------------------------------------------------ */

static const uint32_t smi_video_plane_formats[] = {
	DRM_FORMAT_YUYV,   /* packed YUV 4:2:2 -- FORMAT_YUYV  */
	DRM_FORMAT_YUV420, /* planar  YUV 4:2:0 -- FORMAT_YUV420 */
};

/*
 * smi_video_get_disp_ctrl - map a CRTC to the SM768 display controller channel.
 */
static int smi_video_get_disp_ctrl(struct smi_device *sdev, struct drm_crtc *crtc)
{
	int i;
	for (i = 0; i < MAX_ENCODER; i++) {
		if (sdev->smi_enc_tab[i] && crtc == sdev->smi_enc_tab[i]->crtc)
			break;
	}
	if (i >= MAX_CRTC)
		return smi_calc_hdmi_ctrl(sdev->m_connector);
	return (i == CHANNEL1_CTRL) ? CHANNEL1_CTRL : CHANNEL0_CTRL;
}

static int smi_video_atomic_check(struct drm_plane *plane,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
					  struct drm_plane_state *state
#else
					  struct drm_atomic_state *atom_state
#endif
)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
#endif
	if (!state->fb)
		return 0;
	if (!state->crtc_w || !state->crtc_h)
		return -EINVAL;
	/*
	 * SM768 video overlay only supports upscaling (dst >= src).
	 * Reject downscale requests so the compositor falls back to
	 * software rendering rather than displaying garbage.
	 */
	if (state->crtc_w < (state->src_w >> 16) ||
	    state->crtc_h < (state->src_h >> 16))
		return -EINVAL;
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)

static void smi_video_atomic_update(struct drm_plane *plane,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
					   struct drm_plane_state *old_state
#else
					   struct drm_atomic_state *atom_state
#endif
)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atom_state, plane);
#else
	struct drm_plane_state *state = plane->state;
#endif
	struct smi_device *sdev = plane->dev->dev_private;
	struct drm_framebuffer *fb;
	struct drm_gem_vram_object *gbo;
	video_format_t vfmt;
	unsigned long y_addr, u_addr, v_addr;
	unsigned long y_pitch, uv_pitch;
	unsigned long src_w, src_h, dst_w, dst_h;
	long dst_x, dst_y;
	int disp_ctrl;

	if (!state || !state->fb || !state->crtc)
		return;
	if (sdev->specId != SPC_SM768)
		return;

	fb       = state->fb;
	disp_ctrl = smi_video_get_disp_ctrl(sdev, state->crtc);

	/* Source size (fixed-point 16.16 -> pixels) */
	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;

	/* Destination rectangle (crtc_x/y may be negative when clipped) */
	dst_x = state->crtc_x < 0 ? 0 : state->crtc_x;
	dst_y = state->crtc_y < 0 ? 0 : state->crtc_y;
	dst_w = state->crtc_w;
	dst_h = state->crtc_h;

	if (!src_w || !src_h || !dst_w || !dst_h)
		return;

	/*
	 * Hardware consumes local VRAM addresses.  prepare_fb pins every GEM
	 * object before this callback, so drm_gem_vram_offset() is stable here.
	 * Add the per-plane framebuffer offset as multi-planar framebuffers may
	 * share GEM objects and distinguish Y/U/V by offsets[].
	 */
	gbo     = drm_gem_vram_of_gem(fb->obj[0]);
	y_addr  = (unsigned long)drm_gem_vram_offset(gbo) + fb->offsets[0];
	y_pitch = fb->pitches[0];

	if (fb->format->format == DRM_FORMAT_YUV420 && fb->obj[1] && fb->obj[2]) {
		struct drm_gem_vram_object *ugbo = drm_gem_vram_of_gem(fb->obj[1]);
		struct drm_gem_vram_object *vgbo = drm_gem_vram_of_gem(fb->obj[2]);
		u_addr   = (unsigned long)drm_gem_vram_offset(ugbo) + fb->offsets[1];
		v_addr   = (unsigned long)drm_gem_vram_offset(vgbo) + fb->offsets[2];
		uv_pitch = fb->pitches[1];
		vfmt     = FORMAT_YUV420;
	} else {
		u_addr = v_addr = uv_pitch = 0;
		vfmt   = FORMAT_YUYV;
	}

	DRM_INFO("overlay plane=%u crtc=%u fb=%u fmt=%p4cc "
		 "fb=%ux%u src=%lux%lu+%u+%u dst=%lux%lu+%ld+%ld "
		 "addr=%#lx offset=%u pitch=%lu uv_pitch=%lu ctrl=%d\n",
		 plane->base.id, state->crtc->base.id, fb->base.id,
		 &fb->format->format, fb->width, fb->height,
		 src_w, src_h, state->src_x >> 16, state->src_y >> 16,
		 dst_w, dst_h, dst_x, dst_y, y_addr, fb->offsets[0],
		 y_pitch, uv_pitch, disp_ctrl);

	/* Enable bilinear interpolation for smooth scaling */
	videoSetInterpolation(disp_ctrl, 1, 1);

	videoSetupEx(disp_ctrl,
		     (unsigned long)dst_x, (unsigned long)dst_y,
		     src_w, src_h, dst_w, dst_h,
		     0 /* single buffer */,
		     y_addr, u_addr, v_addr, uv_pitch,
		     y_pitch, y_pitch,
		     vfmt, 0, 0);
	startVideo(disp_ctrl);
}

static void smi_video_atomic_disable(struct drm_plane *plane,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
					    struct drm_plane_state *old_state
#else
					    struct drm_atomic_state *atom_state
#endif
)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0)
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(atom_state, plane);
#endif
	struct smi_device *sdev = plane->dev->dev_private;
	int disp_ctrl;

	if (sdev->specId != SPC_SM768 || !old_state->crtc)
		return;
	disp_ctrl = smi_video_get_disp_ctrl(sdev, old_state->crtc);
	stopVideo(disp_ctrl);
}

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0) */

static const struct drm_plane_helper_funcs smi_video_plane_helper_funcs = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 17, 0)
	.prepare_fb    = drm_gem_vram_plane_helper_prepare_fb,
	.cleanup_fb    = drm_gem_vram_plane_helper_cleanup_fb,
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
	/*
	 * drm_gem_vram_plane_helper_prepare_fb() was added in 5.5.  On 5.3/5.4,
	 * use the driver's equivalent helpers so the overlay BO is pinned in
	 * local VRAM before its hardware address is programmed.
	 */
	.prepare_fb    = smi_primary_plane_prepare_fb,
	.cleanup_fb    = smi_primary_plane_cleanup_fb,
#endif
	.atomic_check   = smi_video_atomic_check,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	.atomic_update  = smi_video_atomic_update,
	.atomic_disable = smi_video_atomic_disable,
#endif
};

struct drm_plane *smi_plane_init(struct smi_device *cdev, unsigned int possible_crtcs,
				 enum drm_plane_type type)
{
	int err;
	int num_formats;
	const uint32_t *formats;
	struct drm_plane *plane;
	const struct drm_plane_funcs *funcs;
	const struct drm_plane_helper_funcs *helper_funcs;

	switch (type) {
	case DRM_PLANE_TYPE_PRIMARY:
		funcs = &smi_plane_funcs;
		formats = smi_formats;
		num_formats = ARRAY_SIZE(smi_formats);
		helper_funcs = &smi_primary_plane_helper_funcs;
		break;
	case DRM_PLANE_TYPE_CURSOR:
		funcs = &smi_plane_funcs;
		formats = smi_cursor_plane_formats;
		num_formats = ARRAY_SIZE(smi_cursor_plane_formats);
		helper_funcs = &smi_cursor_helper_funcs;
		break;
	case DRM_PLANE_TYPE_OVERLAY:
		funcs        = &smi_plane_funcs;
		formats      = smi_video_plane_formats;
		num_formats  = ARRAY_SIZE(smi_video_plane_formats);
		helper_funcs = &smi_video_plane_helper_funcs;
		break;
	default:
		return ERR_PTR(-EINVAL);
	}

	plane = kzalloc(sizeof(*plane), GFP_KERNEL);
	if (!plane)
		return ERR_PTR(-ENOMEM);

	err = drm_universal_plane_init(cdev->dev, plane, possible_crtcs, funcs, formats,
				       num_formats, NULL, type, NULL);

	if (err)
		goto free_plane;

	drm_plane_helper_add(plane, helper_funcs);

	return plane;

free_plane:
	kfree(plane);
	return ERR_PTR(-EINVAL);
}

__attribute__((unused)) static void colorcur2monocur(void *data)
{
	unsigned int *col = (unsigned int *)data;
	unsigned char *mono = (unsigned char *)data;
	unsigned char pixel = 0;
	char bit_values;

	int i;
	for (i = 0; i < 64 * 64; i++) {
		if (*col >> 24 < 0xe0) {
			bit_values = 0;
		} else {
			int val = *col & 0xff;

			if (val < 0x80) {
				bit_values = 1;
			} else {
				bit_values = 2;
			}
		}
		col++;
		/* Copy bits into cursor byte */
		switch (i & 3) {
		case 0:
			pixel = bit_values;
			break;

		case 1:
			pixel |= bit_values << 2;
			break;

		case 2:
			pixel |= bit_values << 4;
			break;

		case 3:
			pixel |= bit_values << 6;
			*mono = pixel;
			mono++;
			pixel = 0;
			break;
		}
	}
}
