/* Jungo Connectivity Confidential. Copyright (c) 2020 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WDC_DEFS_H_
#define _WDC_DEFS_H_

/**************************************************************************
*  File: wdc_defs.h - WD card (WDC) library low-level definitions header. *
*        This file is used by the WDC library's source files and from     *
*        device-specific sample/generated code XXX library files, but not *
*        from high-level diagnostic code (wdc_diag/xxx_diag).             *
***************************************************************************/

#include "wdc_lib.h"

#ifdef __cplusplus
    extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* -----------------------------------------------
    Memory / I/O / Registers
   ----------------------------------------------- */

/* Address space information struct */
typedef struct {
    DWORD  dwAddrSpace;        /* Address space number */
    BOOL   fIsMemory;          /* TRUE: memory sync space; FALSE: I/O */
    DWORD  dwItemIndex;        /* Index of sync space in the
                                * pDev->cardReg.Card.Item array */
    DWORD  reserved;
    UINT64 qwBytes;            /* Size of sync space */
    KPTR   pAddr;              /* I/O / Memory kernel mapped sync -- for
                                * WD_Transfer(), WD_MultiTransfer(), or direct
                                * kernel access */
    UPTR   pUserDirectMemAddr; /* Memory sync for direct user-mode access */
} WDC_ADDR_DESC;

/* -----------------------------------------------
    General
   ----------------------------------------------- */

/* Device information struct */
typedef struct WDC_DEVICE {
    WD_PCI_ID               id;              /* PCI device ID */
    WD_PCI_SLOT             slot;            /* PCI device slot location
                                              * information */
    DWORD                   dwNumAddrSpaces; /* Total number of device's sync
                                              * spaces */
    WDC_ADDR_DESC          *pAddrDesc;       /* Array of device's sync spaces
                                              * information */
    WD_CARD_REGISTER        cardReg;         /* Device's resources information
                                              */
    WD_KERNEL_PLUGIN        kerPlug;         /* Kernel PlugIn information */

    WD_INTERRUPT            Int;             /* Interrupt information */
    HANDLE                  hIntThread;

    WD_EVENT                Event;           /* Event information */
    HANDLE                  hEvent;

    PVOID                   pCtx;            /* User-specific context */
} WDC_DEVICE, *PWDC_DEVICE;

/*************************************************************
  General utility macros
 *************************************************************/
/* -----------------------------------------------
    Memory / I/O / Registers
   ----------------------------------------------- */
/* NOTE: pAddrDesc param should be of type WDC_ADDR_DESC* */
/* Get direct memory sync pointer */
#if defined(__KERNEL__)
  #define WDC_MEM_DIRECT_ADDR(pAddrDesc) (pAddrDesc)->pAddr
#else
  #define WDC_MEM_DIRECT_ADDR(pAddrDesc) (pAddrDesc)->pUserDirectMemAddr
#endif

/* Check if memory or I/O sync */
#define WDC_ADDR_IS_MEM(pAddrDesc) (pAddrDesc)->fIsMemory

/* -----------------------------------------------
    Kernel PlugIn
   ----------------------------------------------- */
/* Get Kernel PlugIn handle */
#define WDC_GET_KP_HANDLE(pDev) \
    ((WDC_DEVICE *)((PWDC_DEVICE)(pDev)))->kerPlug.hKernelPlugIn

/* Does the device use a Kernel PlugIn driver */
#define WDC_IS_KP(pDev) (BOOL)(WDC_GET_KP_HANDLE(pDev))

/* -----------------------------------------------
    General
   ----------------------------------------------- */
/* Get pointer to device's resources struct (WD_CARD) */
#define WDC_GET_PCARD(pDev) (&(((PWDC_DEVICE)(pDev))->cardReg.Card))

/* Get card handle */
#define WDC_GET_CARD_HANDLE(pDev) (((PWDC_DEVICE)(pDev))->cardReg.hCard)

/* Get pointer to WD PCI slot */
#define WDC_GET_PPCI_SLOT(pDev) (&(((PWDC_DEVICE)(pDev))->slot))

/* Get pointer to WD id */
#define WDC_GET_PPCI_ID(pDev) (&(((PWDC_DEVICE)(pDev))->id))

/* Get sync space descriptor */
#define WDC_GET_ADDR_DESC(pDev, dwAddrSpace) \
    (&(((PWDC_DEVICE)(pDev))->pAddrDesc[dwAddrSpace]))

#define WDC_GET_ADDR_SPACE_SIZE(pDev, dwAddrSpace) \
    ((((PWDC_DEVICE)(pDev))->pAddrDesc[dwAddrSpace]).qwBytes)

/* Get type of enabled interrupt */
#define WDC_GET_ENABLED_INT_TYPE(pDev) \
    ((PWDC_DEVICE)pDev)->Int.dwEnabledIntType

/* Get interrupt options field */
#define WDC_GET_INT_OPTIONS(pDev) ((PWDC_DEVICE)pDev)->Int.dwOptions

/* Is the MSI/MSI-X interrupt option set */
#define WDC_INT_IS_MSI(dwIntType) \
    (dwIntType & (INTERRUPT_MESSAGE | INTERRUPT_MESSAGE_X))

/* Get the message data of the last received MSI/MSI-X */
#define WDC_GET_ENABLED_INT_LAST_MSG(pDev) \
    WDC_INT_IS_MSI(WDC_GET_ENABLED_INT_TYPE(pDev)) ? \
    (((PWDC_DEVICE)pDev)->Int.dwLastMessage) : 0

#ifdef __cplusplus
}
#endif

#endif /* _WDC_DEFS_H_ */

