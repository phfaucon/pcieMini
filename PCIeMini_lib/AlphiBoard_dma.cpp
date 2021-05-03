//
// Copyright (c) 2020 Alphi Technology Corporation, Inc.  All Rights Reserved
//
// You are hereby granted a copyright license to use, modify and
// distribute this SOFTWARE so long as the entire notice is retained
// without alteration in any modified and/or redistributed versions,
// and that such modified versions are clearly identified as such.
// No licenses are granted by implication, estopple or otherwise under
// any patents or trademarks of Alphi Technology Corporation (Alphi).
//
// The SOFTWARE is provided on an "AS IS" basis and without warranty,
// to the maximum extent permitted by applicable law.
//
// ALPHI DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED, INCLUDING
// WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
// AND ANY WARRANTY AGAINST INFRINGEMENT WITH REGARD TO THE SOFTWARE
// (INCLUDING ANY MODIFIED VERSIONS THEREOF) AND ANY ACCOMPANYING
// WRITTEN MATERIAL.
//
// To the maximum extent permitted by applicable law, IN NO EVENT SHALL
// ALPHI BE LIABLE FOR ANY DAMAGE WHATSOEVER (INCLUDING WITHOUT LIMITATION,
// DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
// BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM THE USE
// OR INABILITY TO USE THE SOFTWARE.  GMS assumes no responsibility for
// for the maintenance or support of the SOFTWARE
//
/** @file AlphiBoard_dma.cpp
* @brief Implementation of the DMAs for the base PCIe board class with Jungo driver and Altera PCIe hardware
*/

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		10/23/2020	phf	Written
//---------------------------------------------------------------------
#include "AlphiBoard.h"
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "status_strings.h"
#include "stdio.h"
#include "string.h"
#include "utils.h"

MINIPCIE_INT_HANDLER MyDmaIntHandler;
void AlphiBoard::hwDMAStart(TransferDesc* tfrDesc) {}
bool AlphiBoard::hwDMAWaitForCompletion(TransferDesc* tfrDesc, bool fPolling) { return false; }
bool AlphiBoard::hwDMAInterruptEnable(MINIPCIE_INT_HANDLER MyDmaIntHandler, void* pDMA) { return false; }
void AlphiBoard::hwDMAInterruptDisable() {}
void AlphiBoard::hwDMAProgram(
    WD_DMA_PAGE* Page,
    DWORD dwPages,
    bool fToDev,
    uint32_t u32LocalAddr,
    TransferDesc* tfrDesc)
{}

bool AlphiBoard::DMARoutine(DWORD dwDMABufSize,
    uint32_t u32LocalAddr, bool fPolling, bool fToDev,
    TransferDesc* tfrDesc)
{
    WD_DMA* pDma = NULL;
    bool fRet = FALSE;

    /* Allocate a DMA buffer and open DMA for the selected channel */
    if (!DMAOpen(u32LocalAddr, dwDMABufSize, fToDev, tfrDesc))
        goto Exit;

    /* Enable DMA interrupts (if not polling) */
    if (!fPolling)
    {
        if (!hwDMAInterruptEnable(MyDmaIntHandler, pDma))
            goto Exit; /* Failed enabling DMA interrupts */
    }

    /* Flush the CPU caches (see documentation of WDC_DMASyncCpu()) */
    WDC_DMASyncCpu(pDma);

    /* Start DMA - write to the device to initiate the DMA transfer */
    hwDMAStart(tfrDesc);

    /* Wait for the DMA transfer to complete */
    hwDMAWaitForCompletion(tfrDesc, fPolling);

    /* Flush the I/O caches (see documentation of WDC_DMASyncIo()) */
    WDC_DMASyncIo(pDma);

    fRet = TRUE;

Exit:
    DMAClose(fPolling);
    return fRet;
}

/** @brief Allocates and locks a contiguous DMA buffer 
* 
* @param fToDev true means DMA to device, false means DMA from device.
* @param dwDMABufSize Size of the DMA buffer allocated in user space.
* @param tfrDesc Pointer to a transfer information structure.
* @param u32LocalAddr Local FPGA address of the DMA source or destination inside the board.
*/
PCIeMini_status AlphiBoard::DMAOpen(uint32_t u32LocalAddr,
    DWORD dwDMABufSize, bool fToDev,
    TransferDesc* tfrDesc)
{
    DWORD dwStatus;
    DWORD dwOptions = fToDev ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
    void* pBuf;

    if (dwDMABufSize % 8 != 0)
        return ERRCODE_INVALID_DATALENGTH;

    /* Allocate and lock a contiguous DMA buffer */
    dwStatus = WDC_DMAContigBufLock(hDev, &pBuf, dwOptions, dwDMABufSize, &pDma);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        printf("Failed locking a contiguous DMA buffer. Error 0x%lx - %s\n",
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    tfrDesc->bufLength = pDma->Page[0].dwBytes;
    tfrDesc->flags = 0;
    tfrDesc->userSpaceBuffer = (uint32_t *)pBuf;

    memset(pBuf, 0, dwDMABufSize);

    /* Program the device's DMA registers for the physical DMA page */
    hwDMAProgram(pDma->Page, pDma->dwPages, fToDev, u32LocalAddr, tfrDesc);

    return WD_STATUS_SUCCESS;
}

/** @brief Frees a previously allocated contiguous DMA buffer */
void AlphiBoard::DMAClose(bool fPolling)
{
    /* Disable DMA interrupts (if not polling) */
    if (!fPolling)
        hwDMAInterruptDisable();

    /* Unlock and free the DMA buffer */
    WDC_DMABufUnlock(pDma);
}

void AlphiBoard::DMATransfer(TransferDesc* tfrDesc, bool fPolling)
{
    /* Flush the CPU caches (see documentation of WDC_DMASyncCpu()) */
    WDC_DMASyncCpu(pDma);

    for (int i = 0; i < 40; i++) {
        /* Start DMA - write to the device to initiate the DMA transfer */
        hwDMAStart(tfrDesc);

        /* Wait for the DMA transfer to complete */
        hwDMAWaitForCompletion(tfrDesc, fPolling);
    }

    /* Flush the I/O caches (see documentation of WDC_DMASyncIo()) */
    WDC_DMASyncIo(pDma);

}
