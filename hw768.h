// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef LYNX_HW768_H__
#define LYNX_HW768_H__
#include "hw_com.h"
#ifdef USE_EP952
#include "ddk768/ep952api.h"
#endif


unsigned char HDMI_connector_detect(void);
void ddk768_setDisplayPlaneDisableOnly(
   disp_control_t dispControl /* Channel 0 or Channel 1) */
);


void hw768_enable_lvds(int channels);

void ddk768_set_mmio(volatile unsigned char * addr,unsigned short devId,char revId);
unsigned long ddk768_getFrameBufSize(void);
long ddk768_initChip(void);
void ddk768_deInit(void);

void ddk768_swPanelPowerSequence(disp_control_t dispControl, disp_state_t dispState, unsigned long vSyncDelay);


long ddk768_edidHeaderReadMonitorEx(
    unsigned char sclGpio,
    unsigned char sdaGpio
);

long ddk768_edidHeaderReadMonitorExHwI2C(
    unsigned char i2cNumber
);



long ddk768_detectCRTMonitor(disp_control_t dispControl, unsigned char redValue, 
	unsigned char greenValue, unsigned char blueValue);

long ddk768_edidReadMonitor(
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char i2cNumber
);





long ddk768_edidReadMonitorEx(
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char sclGpio,
    unsigned char sdaGpio
);



int hw768_get_hdmi_edid(unsigned char *pEDIDBuffer);



long ddk768_edidReadMonitorExHwI2C(
    unsigned char *pEDIDBuffer,
    unsigned long bufferSize,
    unsigned char edidExtNo,
    unsigned char i2cNumber
);

/*
 * Disable double pixel clock. 
 * This is a teporary function, used to patch for the random fuzzy font problem. 
 */
void DisableDoublePixel(disp_control_t dispControl);
void EnableDoublePixel(disp_control_t dispControl);
void hw768_SetPixelClockFormat(disp_control_t dispControl,unsigned int is_half);


/*
 * This function initializes the cursor attributes.
 */
void ddk768_initCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long base,             /* Base Address */ 
    unsigned long color1,           /* Cursor color 1 in RGB 5:6:5 format */
    unsigned long color2,           /* Cursor color 2 in RGB 5:6:5 format */
    unsigned long color3            /* Cursor color 3 in RGB 5:6:5 format */
);

/*
 * This function sets the cursor position.
 */
void ddk768_setCursorPosition(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long dx,               /* X Coordinate of the cursor */
    unsigned long dy,               /* Y Coordinate of the cursor */
    unsigned char topOutside,       /* Top Boundary Select: either partially outside (= 1) 
                                       or within the screen top boundary (= 0) */
    unsigned char leftOutside       /* Left Boundary Select: either partially outside (= 1) 
                                       or within the screen left boundary (= 0) */
);
 
void hw768_set_base(int display,int pitch,int base_addr);
 
/*
 * This function enables/disables the cursor.
 */
void ddk768_enableCursor(
    disp_control_t dispControl,     /* Display control (CHANNEL0_CTRL or CHANNEL1_CTRL) */
    unsigned long enable
);

void HDMI_Enable_Output(void);

void HDMI_Disable_Output(void);

 
long ddk768_setMode(
    logicalMode_t *pLogicalMode
);
long setSingleViewOn(disp_control_t dispOutput);
long initDisplay(void);	

void setDisplayDPMS(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   DISP_DPMS_t state /* DPMS state */
   );

void HDMI_Init(void);
int hw768_set_hdmi_mode(logicalMode_t *pLogicalMode, struct drm_display_mode mode, bool isHDMI);



void ddk768_setDisplayEnable(disp_control_t dispControl, /* Channel 0 or Channel 1) */
disp_state_t dispState /* ON or OFF */);

int hw768_check_iis_interrupt(void);

int hw768_check_vsync_interrupt(int path);
void hw768_clear_vsync_interrupt(int path);

long hw768_setMode(logicalMode_t *pLogicalMode, struct drm_display_mode mode);


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
int hw768_en_dis_interrupt(int status, int pipe);
#else
int hw768_en_dis_interrupt(int status);
#endif

int hdmi_detect(void);

int hdmi_hotplug_detect(void);

void HDMI_Audio_Mute (void);

void HDMI_Audio_Unmute (void);


void ddk768_disable_IntMask(void);

void hw768_suspend(struct smi_768_register * pSave);
void hw768_resume(struct smi_768_register * pSave);

void hw768_setgamma(disp_control_t dispCtrl, unsigned long enable,unsigned long lvds_ch);
void hw768_load_lut(disp_control_t dispCtrl, int size, u8 lut_r[], u8 lut_g[], u8 lut_b[]);

long hw768_AdaptI2CCleanBus(struct drm_connector *connector);
long hw768_AdaptI2CInit(struct smi_connector *smi_connector);

#endif
