/* Jungo Connectivity Confidential. Copyright (c) 2020 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WDS_DIAG_LIB_H_
#define _WDS_DIAG_LIB_H_

/******************************************************************************
*  File: wds_diag_lib.h - Shared WDS all devices' user-mode diagnostics       *
*        API header.                                                          *
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "windrvr.h"

/*************************************************************
  General definitions
 *************************************************************/

/* Example IPC messages IDs */
enum {
    IPC_MSG_KERNEL_BUFFER_READY = 1,
                                     /* Kernel buffer (Created with
                                      * WDS_SharedBufferAlloc()) ready to be
                                      * shared between processes. Kernel Buffer
                                      * handle is passed in the qwMsgData */

    IPC_MSG_CONTIG_DMA_BUFFER_READY = 2,
                                     /* Kernel buffer (Created with
                                      * WDC_DMAContigBufLock()) ready to be
                                      * shared between processes. DMA Buffer
                                      * handle is passed in the qwMsgData */

    /* TODO: Modify/Add values to communicate between processes */
};

/* -----------------------------------------------
    All buses
   ----------------------------------------------- */

void MenuIpc(void);
/* Group ID is determined in WDS_IpcRegister() call */
DWORD WDS_DIAG_IpcSendDmaContigToGroup(WD_DMA *pDma);

void MenuSharedBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* _WDS_DIAG_LIB_H_ */

