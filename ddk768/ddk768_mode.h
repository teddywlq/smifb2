#ifndef _DDK768_MODE_H_
#define _DDK768_MODE_H_

#include "../hw_com.h"

/*
 *  ddk768_getUserDataSignature
 *      This function gets the user data mode signature
 *
 *  Output:
 *      The signature to be filled in the user_data_mode_t structure to be considered
 *      a valid structure.
 */
unsigned long ddk768_getUserDataSignature(void);

/*
 *  compareModeParam
 *      This function compares two mode parameters
 *
 *  Input:
 *      pModeParam1 - Pointer to the first mode parameter to be compared
 *      pModeParam2 - Pointer to the second mode parameter to be compared
 *
 *  Output:
 *      0   - Identical mode
 *     -1   - Mode is not identical
 */
long compareModeParam(
    mode_parameter_t *pModeParam1,
    mode_parameter_t *pModeParam2
);

/*
 *  getDuplicateModeIndex
 *      This function retrieves the index of dupicate modes, but having different timing.
 *
 *  Input:
 *      dispCtrl    - Display Control where the mode table belongs to.
 *      pModeParam  - The mode parameters which index to be checked.
 *
 *  Output:
 *      The index of the given parameters among the duplicate modes.
 *          0 means that the mode param is the first mode encountered in the table
 *          1 means that the mode param is the second mode encountered in the table
 *          etc...
 */
unsigned short getDuplicateModeIndex(
    disp_control_t dispCtrl,
    mode_parameter_t *pModeParam
);

/*
 *  ddk768_findModeParamFromTable
 *      This function locates the requested mode in the given parameter table
 *
 *  Input:
 *      width           - Mode width
 *      height          - Mode height
 *      refresh_rate    - Mode refresh rate
 *      index           - Index that is used for multiple search of the same mode 
 *                        that have the same width, height, and refresh rate, 
 *                        but have different timing parameters.
 *
 *  Output:
 *      Success: return a pointer to the mode_parameter_t entry.
 *      Fail: a NULL pointer.
 */
mode_parameter_t *ddk768_findModeParamFromTable(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index,
    mode_parameter_t *pModeTable
);

/*
 *  Locate in-stock parameter table for the requested mode.
 *  Success: return a pointer to the mode_parameter_t entry.
 *  Fail: a NULL pointer.
 */
mode_parameter_t *ddk768_findModeParam(
    disp_control_t dispCtrl,
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate,
    unsigned short index
);

/*
 *  Use the
 *  Locate timing parameter for the requested mode from the default mode table.
 *  Success: return a pointer to the mode_parameter_t entry.
 *  Fail: a NULL pointer.
 */
mode_parameter_t *ddk768_findVesaModeParam(
    unsigned long width, 
    unsigned long height, 
    unsigned long refresh_rate
);

/*
 * Return a point to the gDefaultModeParamTable.
 * Function in other files used this to get the mode table pointer.
 */
mode_parameter_t *ddk768_getStockModeParamTable(void);

/*
 * Return the size of the Stock Mode Param Table
 */
unsigned long ddk768_getStockModeParamTableSize(void);

/* 
 *  ddk768_getStockModeParamTableEx
 *      This function gets the mode parameters table associated to the
 *      display control (CHANNEL0_CTRL or SECONDAR_CTRL).
 *
 *  Input:
 *      dispCtrl    - Display Control of the mode table that is associated to.
 *
 *  Output:
 *      Pointer to the mode table
 */
mode_parameter_t *ddk768_getStockModeParamTableEx(
    disp_control_t dispCtrl
);

/*
 *  ddk768_getStockModeParamTableSizeEx
 *      This function gets the size of the mode parameter table associated with
 *      specific display control
 *
 *  Input:
 *      dispCtrl    - Display control of the mode param table that is associated to.
 *
 *  Output:
 *      Size of the requeted mode param table.
 */
unsigned long ddk768_getStockModeParamTableSizeEx(
    disp_control_t dispCtrl
);

/* 
 * This function returns the current mode.
 */
mode_parameter_t ddk768_getCurrentModeParam(
    disp_control_t dispCtrl
);

/*
 *  getMaximumModeEntries
 *      This function gets the maximum entries that can be stored in the mode table.
 *
 *  Output:
 *      Total number of maximum entries
 */
unsigned long getMaximumModeEntries(void);

/*
 *  addTiming
 *      This function adds the SM750 mode parameter timing to the specified mode table
 *
 *  Input:
 *      dispCtrl        - Display control where the mode will be associated to
 *      pNewModeList    - Pointer to a list table of SM750 mode parameter to be added 
 *                        to the current specified display control mode table.
 *
 *  Output:
 *      0   - Success
 *     -1   - Fail
 */
long addTiming(
    disp_control_t dispCtrl,
    mode_parameter_t *pNewModeList,
    unsigned long totalList,
    unsigned char clearTable
);



/*
 *    This function sets the display base address
 *
 *    Input:
 *        dispControl        - display control of which base address to be set.
 *        ulBaseAddress    - Base Address value to be set.
 */
void ddk768_setDisplayBaseAddress(
    disp_control_t dispControl,
    unsigned long ulBaseAddress
);

/*
 *    This function checks if change of "display base address" has effective.
 *    Change of DC base address will not effective until next VSync, SW sets pending bit to 1 during address change.
 *    HW resets pending bit when it starts to use the new address.
 *
 *    Input:
 *        dispControl        - display control of which display status to be retrieved.
 *
 *  Output:
 *      1   - Display is pending
 *      0   - Display is not pending
 */
long isDisplayBasePending(
    disp_control_t dispControl
);

/*
 * Input:
 *     1) pLogicalMode contains information such as x, y resolution and bpp.
 *     2) A user defined parameter table for the mode.
 *
 * This function calculate and programs the hardware to set up the
 * requested mode.
 *
 * This function allows the use of user defined parameter table if
 * predefined Vesa parameter table (gDefaultModeParamTable) does not fit.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk768_setCustomMode(
    logicalMode_t *pLogicalMode, 
    mode_parameter_t *pUserModeParam
);

/*
 * Input pLogicalMode contains information such as x, y resolution and bpp 
 * The main difference between setMode and setModeEx userData.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk768_setModeEx(
    logicalMode_t *pLogicalMode
);

/*
 * Input pLogicalMode contains information such as x, y resolution and bpp.
 * If there is no special parameters, use this function.
 * If there are special parameters, use setModeEx.
 *
 * Return: 0 (or NO_ERROR) if mode can be set successfully.
 *         -1 if any set mode error.
 */
long ddk768_setMode(
    logicalMode_t *pLogicalMode
);


#endif /* _MODE_H_ */
