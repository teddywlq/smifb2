// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include <linux/pci.h>
#include <drm/drm_print.h>
#include "smi_drv.h"
#include "ddk768/ddk768_pwm.h"
#include <drm/drm_debugfs.h>

extern int pwm_ctrl;
extern int smi_debug;
extern int force_connect;


static int debugfs_pwm_set(void *data, u64 val)
{
	int ctrl = *(u32*)data;

	if(val == ctrl) {
		pr_info("Nothing to do with %u\n", (int)val);
		return 0;
	} 

	if(ctrl) {
		if((val & 0xf) != (ctrl & 0xf)) {
			ddk768_pwmClose(ctrl & 0xf);
		} 
	
		if((val & ~0xf) != (ctrl & ~0xf)) {
			ddk768_pwmStop(ctrl & 0xf);
		}
	}

	if (val) {
		unsigned long pwm, divider, highCounter, lowCounter;

		pwm = val & 0xf;
		divider = (val & 0xf0) >> 4;
		highCounter = (val & 0xfff00) >> 8;
		lowCounter = (val & 0xfff00000) >> 20;

		if((val & 0xf) != (ctrl & 0xf)) {
			ddk768_pwmOpen(pwm);
		}

		if((val & ~0xf) != (ctrl & ~0xf)) {
			ddk768_pwmStart(pwm, divider, highCounter, lowCounter, 0);
		}
	}

	*(u32 *)data = val;
	pr_info("Setting pwm_ctrl to %u\n", (int)val);

	return 0;
}

static int debugfs_pwm_get(void *data, u64 *val)
{
	*val = *(u32 *)data;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_pwm, debugfs_pwm_get, debugfs_pwm_set, "%llu\n");

void smi_debugfs_init(struct drm_minor *minor)
{
//	struct drm_device *drm = minor->dev;

	debugfs_create_file_unsafe("pwm_ctl", S_IRUGO | S_IWUSR, minor->debugfs_root, &pwm_ctrl, &fops_pwm);

	debugfs_create_u32("smi_debug", S_IRUGO | S_IWUSR, minor->debugfs_root, &smi_debug);

	debugfs_create_u32("nopnp", S_IRUGO | S_IWUSR, minor->debugfs_root, &force_connect);
}

