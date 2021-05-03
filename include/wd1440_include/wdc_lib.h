/* Jungo Connectivity Confidential. Copyright (c) 2020 Jungo Connectivity Ltd.  https://www.jungo.com */

#ifndef _WDC_LIB_H_
#define _WDC_LIB_H_

/*********************************************************************
*  File: wdc_lib.h - Shared WD card (WDC) library header.            *
*                    This file defines the WDC library's high-level  *
*                    interface                                       *
**********************************************************************/

#if defined(__KERNEL__)
    #include "kpstdlib.h"
#endif
#include "windrvr.h"
#include "windrvr_int_thread.h"
#include "windrvr_events.h"
#include "bits.h"
#include "pci_regs.h"

#ifdef __cplusplus
    extern "C" {
#endif
/**************************************************************
  General definitions
 **************************************************************/
#define MAX_NAME 128
#define MAX_DESC 128
#define MAX_NAME_DISPLAY 22

/* Handle to device information struct */
typedef void *WDC_DEVICE_HANDLE;

#ifndef __KERNEL__
/* PCI scan results */
typedef struct {
    DWORD       dwNumDevices;             /* Number of matching devices */
    WD_PCI_ID   deviceId[WD_PCI_CARDS];   /* Array of matching device IDs */
    WD_PCI_SLOT deviceSlot[WD_PCI_CARDS]; /* Array of matching device locations
                                           */
} WDC_PCI_SCAN_RESULT;
#endif

/* PCI capabilities scan results */
typedef struct {
    DWORD      dwNumCaps; /* Number of matching PCI capabilities */
    WD_PCI_CAP pciCaps[WD_PCI_MAX_CAPS]; /* Array of matching PCI capabilities
                                          */
} WDC_PCI_SCAN_CAPS_RESULT;

/* Driver open options */
/* Basic driver open flags */
#define WDC_DRV_OPEN_CHECK_VER 0x1 /* Compare source files WinDriver version
                                    * with that of the running WinDriver kernel
                                    */
#define WDC_DRV_OPEN_REG_LIC   0x2 /* Register WinDriver license */
/* Convenient driver open options */
#define WDC_DRV_OPEN_BASIC     0x0 /* No option -> perform only the basic open
                                      driver tasks, which are always performed
                                      by WDC_DriverOpen (mainly - open a handle
                                      to WinDriver) */
#define WDC_DRV_OPEN_KP WDC_DRV_OPEN_BASIC /* Kernel PlugIn driver open
                                            * options <=> basic */
#define WDC_DRV_OPEN_ALL (WDC_DRV_OPEN_CHECK_VER | WDC_DRV_OPEN_REG_LIC)
#if defined(__KERNEL__)
    #define WDC_DRV_OPEN_DEFAULT WDC_DRV_OPEN_KP
#else
    #define WDC_DRV_OPEN_DEFAULT WDC_DRV_OPEN_ALL
#endif
typedef DWORD WDC_DRV_OPEN_OPTIONS;

/* Debug information display options */
#define WDC_DBG_OUT_DBM     0x1  /* Send WDC debug messages to the
                                  * Debug Monitor */
#define WDC_DBG_OUT_FILE    0x2  /* Send WDC debug messages to a debug file
                                  * (default: stderr) [User-mode only] */

#define WDC_DBG_LEVEL_ERR   0x10 /* Display only error WDC debug messages */
#define WDC_DBG_LEVEL_TRACE 0x20 /* Display error and trace
                                  * WDC debug messages */

#define WDC_DBG_NONE        0x100 /* Do not print debug messages */

/* Convenient debug options combinations/defintions */
#define WDC_DBG_DEFAULT     (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_TRACE)

#define WDC_DBG_DBM_ERR   (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_ERR)
#define WDC_DBG_DBM_TRACE (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_TRACE)

#if !defined(__KERNEL__)
    #define WDC_DBG_FILE_ERR   (WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_ERR)
    #define WDC_DBG_FILE_TRACE (WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_TRACE)

    #define WDC_DBG_DBM_FILE_ERR   \
        (WDC_DBG_OUT_DBM | WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_ERR)
    #define WDC_DBG_DBM_FILE_TRACE \
        (WDC_DBG_OUT_DBM | WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_TRACE)

    #define WDC_DBG_FULL \
        (WDC_DBG_OUT_DBM | WDC_DBG_OUT_FILE | WDC_DBG_LEVEL_TRACE)
#else
    #define WDC_DBG_FULL (WDC_DBG_OUT_DBM | WDC_DBG_LEVEL_TRACE)
#endif

typedef DWORD WDC_DBG_OPTIONS;

/* Sleep options */
#define WDC_SLEEP_BUSY 0
#define WDC_SLEEP_NON_BUSY SLEEP_NON_BUSY
typedef DWORD WDC_SLEEP_OPTIONS;

/* -----------------------------------------------
    Memory / I/O / Registers
   ----------------------------------------------- */
typedef enum {
    WDC_WRITE,
    WDC_READ,
    WDC_READ_WRITE
} WDC_DIRECTION;

/* Read/write sync options */
typedef enum {
    WDC_ADDR_RW_DEFAULT = 0x0, /* Default: memory resource - direct access;
                                * autoincrement on block transfers */
    WDC_ADDR_RW_NO_AUTOINC = 0x4 /* Hold device sync constant while
                                  * reading/writing a block */
} WDC_ADDR_RW_OPTIONS;

/* Memory/I/O sync size and access mode definitions (size - in bytes) */
#define WDC_SIZE_8 ((DWORD)sizeof(BYTE))
#define WDC_SIZE_16 ((DWORD)sizeof(WORD))
#define WDC_SIZE_32 ((DWORD)sizeof(UINT32))
#define WDC_SIZE_64 ((DWORD)sizeof(UINT64))
typedef DWORD WDC_ADDR_SIZE;

typedef enum {
    WDC_MODE_8 = WDC_SIZE_8,
    WDC_MODE_16 = WDC_SIZE_16,
    WDC_MODE_32 = WDC_SIZE_32,
    WDC_MODE_64 = WDC_SIZE_64
} WDC_ADDR_MODE;

#define WDC_ADDR_MODE_TO_SIZE(mode) (DWORD)mode
#define WDC_ADDR_SIZE_TO_MODE(size) (((size) > WDC_SIZE_32) ? WDC_MODE_64 : \
    ((size) > WDC_SIZE_16) ? WDC_MODE_32 : \
    ((size) > WDC_SIZE_8) ? WDC_MODE_16 : WDC_MODE_8)

/* Device configuration space identifier (PCI configuration space) */
#define WDC_AD_CFG_SPACE 0xFF

/**************************************************************
  Function Prototypes
 **************************************************************/
/* -----------------------------------------------
    General
   ----------------------------------------------- */
/* Get a handle to WinDriver (required for WD_XXX functions) */
HANDLE DLLCALLCONV WDC_GetWDHandle(void);

/* Get a device's user context */
PVOID DLLCALLCONV WDC_GetDevContext(WDC_DEVICE_HANDLE hDev);

/* Get a device's bus type */
WD_BUS_TYPE DLLCALLCONV WDC_GetBusType(WDC_DEVICE_HANDLE hDev);

/* Sleep (default option - WDC_SLEEP_BUSY) */
DWORD DLLCALLCONV WDC_Sleep(DWORD dwMicroSecs, WDC_SLEEP_OPTIONS options);

/* Get WinDriver's kernel module version */
DWORD DLLCALLCONV WDC_Version(CHAR *sVersion, DWORD dwLen, DWORD *pdwVersion);

/* -----------------------------------------------
    Open/close driver and init/uninit WDC library
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_DriverOpen(WDC_DRV_OPEN_OPTIONS openOptions,
    const CHAR *sLicense);
DWORD DLLCALLCONV WDC_DriverClose(void);

/* -----------------------------------------------
    Scan bus (PCI)
   ----------------------------------------------- */
#ifndef __KERNEL__
DWORD DLLCALLCONV WDC_PciScanDevices(DWORD dwVendorId, DWORD dwDeviceId,
    WDC_PCI_SCAN_RESULT *pPciScanResult);
DWORD DLLCALLCONV WDC_PciScanDevicesByTopology(DWORD dwVendorId,
    DWORD dwDeviceId, WDC_PCI_SCAN_RESULT *pPciScanResult);
DWORD DLLCALLCONV WDC_PciScanRegisteredDevices(DWORD dwVendorId,
    DWORD dwDeviceId, WDC_PCI_SCAN_RESULT *pPciScanResult);
#endif

/* -----------------------------------------------
    Scan PCI Capabilities
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_PciGetExpressOffset(WDC_DEVICE_HANDLE hDev, DWORD
    *pdwOffset);
DWORD DLLCALLCONV WDC_PciGetHeaderType(WDC_DEVICE_HANDLE hDev,
    WDC_PCI_HEADER_TYPE *header_type);
DWORD DLLCALLCONV WDC_PciScanCaps(WDC_DEVICE_HANDLE hDev, DWORD dwCapId,
    WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult);
DWORD DLLCALLCONV WDC_PciScanCapsBySlot(WD_PCI_SLOT *pPciSlot, DWORD dwCapId,
    WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult);
DWORD DLLCALLCONV WDC_PciScanExtCaps(WDC_DEVICE_HANDLE hDev, DWORD dwCapId,
    WDC_PCI_SCAN_CAPS_RESULT *pScanCapsResult);
DWORD DLLCALLCONV WDC_PciGetExpressGenBySlot(WD_PCI_SLOT *pPciSlot);
DWORD DLLCALLCONV WDC_PciGetExpressGen(WDC_DEVICE_HANDLE hDev);

/* -------------------------------------------------
    Get device's resources information (PCI)
   ------------------------------------------------- */
#ifndef __KERNEL__
DWORD DLLCALLCONV WDC_PciGetDeviceInfo(WD_PCI_CARD_INFO *pDeviceInfo);
#endif

/* -------------------------------------------------
    Control device's SR-IOV capability (PCIe)
   ------------------------------------------------- */
/* SR-IOV API functions are not part of the standard WinDriver API, and not
 * included in the standard version of WinDriver. The functions are part of
 * "WinDriver for Server" API and require "WinDriver for Server" license.
 * Note that "WinDriver for Server" APIs are included in WinDriver evaluation
 * version. */
DWORD DLLCALLCONV WDC_PciSriovEnable(WDC_DEVICE_HANDLE hDev, DWORD dwNumVFs);
DWORD DLLCALLCONV WDC_PciSriovDisable(WDC_DEVICE_HANDLE hDev);
DWORD DLLCALLCONV WDC_PciSriovGetNumVFs(WDC_DEVICE_HANDLE hDev,
    DWORD *pdwNumVFs);

/* -----------------------------------------------
    Open/Close device
   ----------------------------------------------- */
#if !defined(__KERNEL__)
DWORD DLLCALLCONV WDC_PciDeviceOpen(WDC_DEVICE_HANDLE *phDev,
    const WD_PCI_CARD_INFO *pDeviceInfo, const PVOID pDevCtx);
DWORD DLLCALLCONV WDC_IsaDeviceOpen(WDC_DEVICE_HANDLE *phDev,
    const WD_CARD *pDeviceInfo, const PVOID pDevCtx);

DWORD DLLCALLCONV WDC_PciDeviceClose(WDC_DEVICE_HANDLE hDev);
DWORD DLLCALLCONV WDC_IsaDeviceClose(WDC_DEVICE_HANDLE hDev);
#endif

/* -----------------------------------------------
    Set card cleanup commands
   ----------------------------------------------- */
DWORD WDC_CardCleanupSetup(WDC_DEVICE_HANDLE hDev, WD_TRANSFER *Cmd,
    DWORD dwCmds, BOOL bForceCleanup);

/* -----------------------------------------------
    Open a handle to Kernel PlugIn driver
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_KernelPlugInOpen(WDC_DEVICE_HANDLE hDev,
    const CHAR *pcKPDriverName, PVOID pKPOpenData);

/* -----------------------------------------------
    Send Kernel PlugIn messages
   ----------------------------------------------- */
DWORD DLLCALLCONV WDC_CallKerPlug(WDC_DEVICE_HANDLE hDev, DWORD dwMsg,
    PVOID pData, PDWORD pdwResult);

/* -----------------------------------------------
    Read/Write memory and I/O addresses
   ----------------------------------------------- */
/* Direct memory read/write macros */
#define WDC_ReadMem8(addr, off) *(volatile BYTE *)((UPTR)(addr) + (UPTR)(off))
#define WDC_ReadMem16(addr, off) \
    *(volatile WORD *)((UPTR)(addr) + (UPTR)(off))
#define WDC_ReadMem32(addr, off) \
    *(volatile UINT32 *)((UPTR)(addr) + (UPTR)(off))
#define WDC_ReadMem64(addr, off) \
    *(volatile UINT64 *)((UPTR)(addr) + (UPTR)(off))

#define WDC_WriteMem8(addr, off, val) \
    *(volatile BYTE * )(((UPTR)(addr) + (UPTR)(off))) = (val)
#define WDC_WriteMem16(addr, off, val) \
    *(volatile WORD * )(((UPTR)(addr) + (UPTR)(off))) = (val)
#define WDC_WriteMem32(addr, off, val) \
    *(volatile UINT32 *)(((UPTR)(addr) + (UPTR)(off))) = (val)
#define WDC_WriteMem64(addr, off, val) \
    *(volatile UINT64 *)(((UPTR)(addr) + (UPTR)(off))) = (val)

/* Read/write a device's sync space (8/16/32/64 bits) */
DWORD DLLCALLCONV WDC_ReadAddr8(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, BYTE *val);
DWORD DLLCALLCONV WDC_ReadAddr16(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, WORD *val);
DWORD DLLCALLCONV WDC_ReadAddr32(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, UINT32 *val);
DWORD DLLCALLCONV WDC_ReadAddr64(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, UINT64 *val);

DWORD DLLCALLCONV WDC_WriteAddr8(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, BYTE val);
DWORD DLLCALLCONV WDC_WriteAddr16(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, WORD val);
DWORD DLLCALLCONV WDC_WriteAddr32(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, UINT32 val);
DWORD DLLCALLCONV WDC_WriteAddr64(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, UINT64 val);

/* Read/write a block of data */
DWORD DLLCALLCONV WDC_ReadAddrBlock(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, DWORD dwBytes, PVOID pData, WDC_ADDR_MODE mode,
    WDC_ADDR_RW_OPTIONS options);

DWORD DLLCALLCONV WDC_WriteAddrBlock(WDC_DEVICE_HANDLE hDev, DWORD dwAddrSpace,
    KPTR dwOffset, DWORD dwBytes, PVOID pData, WDC_ADDR_MODE mode,
    WDC_ADDR_RW_OPTIONS options);

#define WDC_ReadAddrBlock8(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_8, options)
#define WDC_ReadAddrBlock16(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_16, options)
#define WDC_ReadAddrBlock32(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_32, options)
#define WDC_ReadAddrBlock64(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_ReadAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_64, options)

#define WDC_WriteAddrBlock8(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_8, options)
#define WDC_WriteAddrBlock16(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_16, options)
#define WDC_WriteAddrBlock32(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_32, options)
#define WDC_WriteAddrBlock64(hDev,dwAddrSpace,dwOffset,dwBytes,pData,options) \
    WDC_WriteAddrBlock(hDev, dwAddrSpace, dwOffset, dwBytes, pData, \
        WDC_MODE_64, options)

/* Issue multiple read/write requests */
DWORD DLLCALLCONV WDC_MultiTransfer(WD_TRANSFER *pTrans, DWORD dwNumTrans);

/* Is sync space active */
BOOL DLLCALLCONV WDC_AddrSpaceIsActive(WDC_DEVICE_HANDLE hDev,
    DWORD dwAddrSpace);

/* -----------------------------------------------
    Access PCI configuration space
   ----------------------------------------------- */
/* Read/write a block of any length from the PCI configuration space */
  /* Identify device by its location. */
DWORD DLLCALLCONV WDC_PciReadCfgBySlot(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    PVOID pData, DWORD dwBytes);
DWORD DLLCALLCONV WDC_PciWriteCfgBySlot(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    PVOID pData, DWORD dwBytes);
  /* Identify device by handle */
DWORD DLLCALLCONV WDC_PciReadCfg(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    PVOID pData, DWORD dwBytes);
DWORD DLLCALLCONV WDC_PciWriteCfg(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    PVOID pData, DWORD dwBytes);

/* Read/write 8/16/32/64 bits from the PCI configuration space.
   Identify device by its location. */
DWORD DLLCALLCONV WDC_PciReadCfgBySlot8(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    BYTE *val);
DWORD DLLCALLCONV WDC_PciReadCfgBySlot16(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    WORD *val);
DWORD DLLCALLCONV WDC_PciReadCfgBySlot32(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    UINT32 *val);
DWORD DLLCALLCONV WDC_PciReadCfgBySlot64(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    UINT64 *val);

DWORD DLLCALLCONV WDC_PciWriteCfgBySlot8(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    BYTE val);
DWORD DLLCALLCONV WDC_PciWriteCfgBySlot16(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    WORD val);
DWORD DLLCALLCONV WDC_PciWriteCfgBySlot32(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    UINT32 val);
DWORD DLLCALLCONV WDC_PciWriteCfgBySlot64(WD_PCI_SLOT *pPciSlot, DWORD dwOffset,
    UINT64 val);

/* Read/write 8/16/32/64 bits from the PCI configuration space.
   Identify device by handle */
DWORD DLLCALLCONV WDC_PciReadCfg8(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    BYTE *val);
DWORD DLLCALLCONV WDC_PciReadCfg16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    WORD *val);
DWORD DLLCALLCONV WDC_PciReadCfg32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 *val);
DWORD DLLCALLCONV WDC_PciReadCfg64(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT64 *val);

DWORD DLLCALLCONV WDC_PciWriteCfg8(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    BYTE val);
DWORD DLLCALLCONV WDC_PciWriteCfg16(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    WORD val);
DWORD DLLCALLCONV WDC_PciWriteCfg32(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT32 val);
DWORD DLLCALLCONV WDC_PciWriteCfg64(WDC_DEVICE_HANDLE hDev, DWORD dwOffset,
    UINT64 val);

/* -----------------------------------------------
    DMA (Direct Memory Access)
   ----------------------------------------------- */
/* Allocate and lock a contiguous DMA buffer */
DWORD DLLCALLCONV WDC_DMAContigBufLock(WDC_DEVICE_HANDLE hDev, PVOID *ppBuf,
    DWORD dwOptions, DWORD dwDMABufSize, WD_DMA **ppDma);

/* Lock a Scatter/Gather DMA buffer */
DWORD DLLCALLCONV WDC_DMASGBufLock(WDC_DEVICE_HANDLE hDev, PVOID pBuf,
    DWORD dwOptions, DWORD dwDMABufSize, WD_DMA **ppDma);


typedef struct {
    WD_TRANSFER *pTransCmds;
    DWORD dwNumCmds;
    DWORD dwOptions;
    INT_HANDLER funcIntHandler;
    PVOID pData;
    BOOL fUseKP;
} WDC_INTERRUPT_PARAMS;

DWORD DLLCALLCONV WDC_DMATransactionContigInit(WDC_DEVICE_HANDLE hDev,
    PVOID *ppBuf, DWORD dwOptions, DWORD dwDMABufSize, WD_DMA **ppDma,
    WDC_INTERRUPT_PARAMS *interruptParams, DWORD dwAlignment);
DWORD DLLCALLCONV WDC_DMATransactionSGInit(WDC_DEVICE_HANDLE hDev, PVOID pBuf,
    DWORD dwOptions, DWORD dwDMABufSize, WD_DMA **ppDma,
    WDC_INTERRUPT_PARAMS *interruptParams, DWORD dwMaxTransferSize,
    DWORD dwTransferElementSize);
DWORD DLLCALLCONV WDC_DMATransactionExecute(WD_DMA *pDma,
    DMA_TRANSACTION_CALLBACK DMATransactionCallback,
    PVOID DMATransactionCallbackCtx);
DWORD DLLCALLCONV WDC_DMATransferCompletedAndCheck(WD_DMA *pDma,
    BOOL fRunCallback);
DWORD DLLCALLCONV WDC_DMATransactionRelease(WD_DMA *pDma);
DWORD DLLCALLCONV WDC_DMATransactionUninit(WD_DMA *pDma);

/* Lock a reserved contiguous DMA buffer */
DWORD DLLCALLCONV WDC_DMAReservedBufLock(WDC_DEVICE_HANDLE hDev,
    PHYS_ADDR qwAddr, PVOID *ppBuf, DWORD dwOptions, DWORD dwDMABufSize,
    WD_DMA **ppDma);

/* Unlock a DMA buffer */
DWORD DLLCALLCONV WDC_DMABufUnlock(WD_DMA *pDma);

/* Get DMA buffer global handle */
#define WDC_DMAGetGlobalHandle(pDma) ((pDma)->hDma)

/* Get a shared DMA buffer.
 * This API function is not part of the standard WinDriver API, and not included
 * in the standard version of WinDriver. It is part of
 * "WinDriver for Server" API and requires "WinDriver for Server" license. Note
 * that "WinDriver for Server" APIs are included in WinDriver evaluation
 * version. */
DWORD DLLCALLCONV WDC_DMABufGet(DWORD hDma, WD_DMA **ppDma);

/* Synchronize cache of all CPUs with the DMA buffer,
 * should be called before DMA transfer */
DWORD DLLCALLCONV WDC_DMASyncCpu(WD_DMA *pDma);

/* Flush the data from I/O cache and update the CPU caches.
 * Should be called after DMA transfer. */
DWORD DLLCALLCONV WDC_DMASyncIo(WD_DMA *pDma);

/* -----------------------------------------------
    Interrupts
   ----------------------------------------------- */
/* Enable/disable interrupts */
DWORD DLLCALLCONV WDC_IntEnable(WDC_DEVICE_HANDLE hDev, WD_TRANSFER *pTransCmds,
    DWORD dwNumCmds, DWORD dwOptions, INT_HANDLER funcIntHandler,
    PVOID pData, BOOL fUseKP);

DWORD DLLCALLCONV WDC_IntDisable(WDC_DEVICE_HANDLE hDev);

/* Are interrupts enabled */
BOOL DLLCALLCONV WDC_IntIsEnabled(WDC_DEVICE_HANDLE hDev);

/* Converts interrupts type to string */
const CHAR * DLLCALLCONV WDC_IntType2Str(DWORD dwIntType);

/* -----------------------------------------------
    Plug-and-play and power management events
   ----------------------------------------------- */
/* Register/unregister to receive events notifications */
DWORD DLLCALLCONV WDC_EventRegister(WDC_DEVICE_HANDLE hDev, DWORD dwActions,
    EVENT_HANDLER funcEventHandler, PVOID pData, BOOL fUseKP);
DWORD DLLCALLCONV WDC_EventUnregister(WDC_DEVICE_HANDLE hDev);

/* Is device registered to receive events notifications */
BOOL DLLCALLCONV WDC_EventIsRegistered(WDC_DEVICE_HANDLE hDev);

/* -----------------------------------------------
    Debugging and error handling
   ----------------------------------------------- */
/* Set debug options for the WDC library */
DWORD DLLCALLCONV WDC_SetDebugOptions(WDC_DBG_OPTIONS dbgOptions,
    const CHAR *sDbgFile);

/* Debug messages display */
void DLLCALLCONV WDC_Err(const CHAR *format, ...);
void DLLCALLCONV WDC_Trace(const CHAR *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* _WDC_LIB_H_ */

