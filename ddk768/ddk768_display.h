#ifndef _DDK768_DISPLAY_H_
#define _DDK768_DISPLAY_H_

#include "ddk768_mode.h"
#include "../hw_com.h"


//Cheok(10/172013): New interface for Falcon.
typedef enum _disp_interface_t
{
    DIGITAL_24BIT = 0,
    DIGITAL_48BIT,
    CRT,
	LVDS,
    HDMI
}
disp_interface_t;

typedef enum _disp_format_t
{
    SINGLE_PIXEL_24BIT = 0,
    DOUBLE_PIXEL_48BIT
}
disp_format_t;

/*
 * This function initializes the display.
 *
 * Output:
 *      0   - Success
 *      1   - Fail 
 */
long initDisplay(void);

/*
 * This function sets the display DPMS state 
 * It is used to set CRT monitor to On, off, or suspend states, 
 * while display channel are still active.
 */
void setDisplayDPMS(
   disp_control_t dispControl, /* Channel 0 or Channel 1) */
   DISP_DPMS_t state /* DPMS state */
   );

/*
 * This funciton sets:
 * 1. Output from channel 0 or channel 1 is 24 single or 48 double pixel.
 * 2. Output data comes from path of channel 0 or 1.
 *
 * Input: See the commnet in the input parameter below.
 *
 * Return: 0 is OK, -1 is error.
 */
long setDisplayFormat(
   disp_control_t outputInterface, /* Use the output of channel 0 or 1 */
   disp_control_t dataPath,        /* Use the data path from channel 0 or 1 */
   disp_format_t dispFormat         /* 24 bit single or 48 bit double pixel */
   );

/*
 * This functions sets the CRT Path.
 */
void setCRTPath(disp_control_t dispControl);

/*
 * This functions uses software sequence to turn on/off the panel.
 */
void ddk768_swPanelPowerSequence(disp_control_t dispControl, disp_state_t dispState, unsigned long vsync_delay);

/*
 * This functions uses software sequence to turn on/off the digital interface.
 */
void swDispPowerSequence(disp_control_t dispControl, disp_state_t dispState, unsigned long vSyncDelay);

/* 
 * This function turns on/off the DAC for CRT display control.
 * Input: On or off
 */
void ddk768_setDAC(disp_state_t state);

/*
 * Wait number of Vertical Vsync
 *
 * Input:
 *          dispControl - Display Control (either channel 0 or 1)
 *          vSyncCount  - Number of vertical sync to wait.
 *
 * Note:
 *      This function is waiting for the next vertical sync.         
 */
void waitDispVerticalSync(disp_control_t dispControl, unsigned long vSyncCount);

/*
 * Use panel vertical sync line as time delay function.
 * This function does not wait for the next VSync. Instead, it will wait
 * until the current line reaches the Vertical Sync line.
 * This function is really useful when flipping display to prevent tearing.
 *
 * Input: display control (CHANNEL0_CTRL or CHANNEL1_CTRL)
 */
void ddk768_waitVSyncLine(disp_control_t dispControl);

/*
 * This function detects if the CRT monitor is attached.
 *
 * Input:
 *      redValue    - Threshold value to be detected on the red color.
 *      greenValue  - Threshold value to be detected on the green color.
 *      blueValue   - Threshold value to be detected on the blue color.
 *
 * Output:
 *      0   - Success
 *     -1   - Fail 
 */
long ddk768_detectCRTMonitor(
    disp_control_t dispControl, 
    unsigned char redValue,
    unsigned char greenValue,
    unsigned char blueValue
);



/*
 * This function turns on/off the display control of Channel 0 or channel 1.
 *
 * Note:
 *      Turning on/off the timing and the plane requires programming sequence.
 *      The plane can not be changed without turning on the timing. However,
 *      changing the plane has no effect when the timing (clock) is off. Below,
 *      is the description of the timing and plane combination setting.
 */
void ddk768_setDisplayEnable(
disp_control_t dispControl, /* Channel 0 or Channel 1) */
disp_state_t dispState /* ON or OFF */
);

/*
 * This function controls monitor on/off and data path.
 * It can be used to set up any veiws: single view, clone view, dual view, output with channel swap, etc.
 * However, it needs too many input parameter.
 * There are other set view functions with less parameters, but not as flexible as this one.
 *
 */
long setDisplayView(
	disp_control_t dispOutput, 			/* Monitor 0 or 1 */
	disp_state_t dispState,				/* On or off */
	disp_control_t dataPath,			/* Use the data path of channel 0 or channel 1 (optional when OFF) */
	disp_format_t dispFormat);			/* 24 or 48 bit digital interface (optional when OFF */

/*
 * Convenient function to trun on single view 
 */
long setSingleViewOn(disp_control_t dispOutput);

/*
 * Convenient function to trun off single view 
 */
long setSingleViewOff(disp_control_t dispOutput);

/*
 * Convenient function to trun on clone view 
 */
long setCloneViewOn(disp_control_t dataPath);

/*
 * Convenient function to trun on dual view 
 */
long setDualViewOn(void);

/*
 * Convenient function to trun off all views
 */
long setAllViewOff(void);


/*
 * Disable double pixel clock. 
 * This is a teporary function, used to patch for the random fuzzy font problem. 
 */
void DisableDoublePixel(disp_control_t dispControl);
void EnableDoublePixel(disp_control_t dispControl);


void enableLVDS(
	unsigned short lvds, //LVDS 1 or 2
	unsigned short on    //On =1, and off = 0
);


void setupLVDS(unsigned short lvds);


long set48bitLVDS(disp_control_t dataPath);
long setSingleLVDS(disp_control_t dataPath);



#endif /* _DISPLAY_H_ */
