/* Jungo Connectivity Confidential. Copyright (c) 2020 Jungo Connectivity Ltd.  https://www.jungo.com */

/**********************************************************************
 * File - USB_DIAG_LIB.H
 *
 * Library for USB diagnostics and samples, using WinDriver functions.
 **********************************************************************/

#ifndef _USB_DIAG_LIB_H_
#define _USB_DIAG_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "windrvr.h"
#include "diag_lib.h"

enum {MAX_BUFFER_SIZE = 4096};

/* Function: pipeType2Str()
     Returns a string identifying the pipe type
*/
char *pipeType2Str(ULONG pipeType);

/* Function: PrintPipesInfo()
     Prints the pipes information for the specified alternate setting
*/
void PrintPipesInfo(WDU_ALTERNATE_SETTING *pAltSet);

/* Function: PrintPipe0Info()
     Prints control pipe (pipe 0) information
*/
void PrintPipe0Info(WDU_DEVICE *pDevice);

/* Function: PrintDevicePipesInfoByHandle()
     Prints the pipes information for all the active device pipes
*/
void PrintDevicePipesInfoByHandle(HANDLE hDevice);

/* Function: PrintDeviceConfigurations()
     Prints the device's configurations information
*/
void PrintDeviceConfigurations(HANDLE hDevice);

/* Function: ReadWritePipesMenu()
     Displays a menu to read/write from/to the device's pipes
*/
void ReadWritePipesMenu(HANDLE hDevice);

/* Function: ListenToPipe()
     Listening to a USB device pipe
*/
void ListenToPipe(HANDLE hDevice, WDU_PIPE_INFO *pPipe, BOOL fStreamMode,
    DWORD dwBufferSize, BOOL fUserKeyWait, BOOL fPrint);

/* Function: FindPipeInDevice()
     Finds a pipe in any of the device's active interfaces
*/
WDU_PIPE_INFO *FindPipeInDevice(WDU_DEVICE *pDevice, DWORD dwPipeNumber);

/* Function: SelectiveSuspendMenu()
     Displays a menu to enter/exit Selective Suspend mode
*/
void SelectiveSuspendMenu(WDU_DEVICE_HANDLE hDevice);

/* Function: PrintDeviceSerialNumberByHandle()
     Prints the device serial number if available
*/
void PrintDeviceSerialNumberByHandle(HANDLE hDevice);

/* Function: PrintDeviceProperties()
     Prints the device properties if available
*/
void PrintDeviceProperties(HANDLE hDevice);

#ifdef __cplusplus
}
#endif

#endif /* _USB_DIAG_LIB_H_ */

