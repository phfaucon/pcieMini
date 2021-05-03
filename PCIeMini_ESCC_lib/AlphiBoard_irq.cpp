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

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		7/23/2020	phf	Written
//---------------------------------------------------------------------
#include "AlphiBoard.h"
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "stdio.h"
#include "string.h"
#include "utils.h"
#include "minipcie_arinc429_lib.h"		// jungo library interface
#include "status_strings.h"
#include "wdc_diag_lib.h" 

/* -----------------------------------------------
Interrupts
----------------------------------------------- */
/** Interrupt handler routine */
static void MINIPCIE_ARINC429_IntHandler(PVOID pData)
{
	PWDC_DEVICE pDev = (PWDC_DEVICE)pData;
	PMINIPCIE_DEV_CTX pDevCtx = (PMINIPCIE_DEV_CTX)WDC_GetDevContext(pDev);
	MINIPCIE_INT_RESULT intResult;

	BZERO(intResult);
	intResult.dwCounter = pDev->Int.dwCounter;
	intResult.dwLost = pDev->Int.dwLost;
	intResult.waitResult = (WD_INTERRUPT_WAIT_RESULT)pDev->Int.fStopped;
	intResult.dwEnabledIntType = WDC_GET_ENABLED_INT_TYPE(pDev);
	intResult.dwLastMessage = WDC_GET_ENABLED_INT_LAST_MSG(pDev);

	/* Execute the diagnostics application's interrupt handler routine */
	pDevCtx->funcDiagIntHandler((WDC_DEVICE_HANDLE)pDev, &intResult);
}

/** Check whether a given device contains an item of the specified type */
static BOOL doesItemExists(PWDC_DEVICE pDev, ITEM_TYPE item)
{
	DWORD i, dwNumItems = pDev->cardReg.Card.dwItems;

	for (i = 0; i < dwNumItems; i++)
	{
		if ((ITEM_TYPE)(pDev->cardReg.Card.Item[i].item) == item)
			return TRUE;
	}

	return FALSE;
}

//! Setup the interrupt of the board
/*!
Specify and interrupt service routine and enable the interrupts.
	@param mask board dependent interrupt mask.
	@param uicr pointer to the interrupt service routine.
	@return WD_STATUS_SUCCESS when the operation succeeded
		WD_INVALID_PARAMETER if the board is not opened
		WD_OPERATION_FAILED if the board does not have an interrupt resource
		WD_OPERATION_ALREADY_DONE if there is already an isr active for the interrupt.
*/
DWORD AlphiBoard::HookMailboxInterrupt(uint32_t mask, MINIPCIE_INT_HANDLER uicr, void *userData)
{
	PMINIPCIE_DEV_CTX pDevCtx;

	/* Validate the WDC device handle */
	if (!IsValidDevice((const CHAR *)"IntEnable"))
		return WD_INVALID_PARAMETER;

	/* Verify that the device has an interrupt item */
	if (!doesItemExists(pDev, ITEM_INTERRUPT))
		return WD_OPERATION_FAILED;

	pDevCtx = (PMINIPCIE_DEV_CTX)WDC_GetDevContext(pDev);

	/* Define the number of interrupt transfer commands to use */
#define NUM_TRANS_CMDS 0

	/* Store the diag interrupt handler routine, which will be executed by
	MINIPCIE_ARINC429_IntHandler() when an interrupt is received */
	pDevCtx->funcDiagIntHandler = uicr;

	return WD_STATUS_SUCCESS;
}

//! Disable the board interrupt
/*!
	@param mask board dependent interrupt mask.
	@param uicr pointer to the interrupt service routine.
	@return WD_STATUS_SUCCESS when the operation succeeded
		WD_INVALID_PARAMETER if the board is not opened
		WD_OPERATION_FAILED if the board does not have an interrupt resource
		WD_OPERATION_ALREADY_DONE if there the interrupt is already disabled.
*/
DWORD AlphiBoard::UnhookMailboxInterrupt()
{
	return HookMailboxInterrupt(0, NULL, NULL);
}

/** @brief Enable PCIe interrupts
 *
 * Enable the generation of PCIe interrupts by the PCIe interface only. This is a low level function 
 * that does not do anything board specific for the interrupt generation.
 * @retval Status code
 */
DWORD AlphiBoard::EnableInterrupts()
{
	DWORD dwStatus;

	/* Check whether interrupts are already enabled */
	if (WDC_IntIsEnabled(hDev))
	{
//		ErrLog("HookMailboxInterrupt: Interrupts are already enabled ...\n");
		return WD_OPERATION_ALREADY_DONE;
	}

	/* NOTE: When adding read transfer commands, set the INTERRUPT_CMD_COPY
	flag in the 4th argument passed to WDC_IntEnable() (dwOptions) */
//DWORD DLLCALLCONV WDC_IntEnable(WDC_DEVICE_HANDLE hDev, WD_TRANSFER *pTransCmds,
 //   DWORD dwNumCmds, DWORD dwOptions, INT_HANDLER funcIntHandler,
 //   PVOID pData, BOOL fUseKP);

	dwStatus = WDC_IntEnable(hDev, NULL, 0, 0,
		(INT_HANDLER)MINIPCIE_ARINC429_IntHandler, (PVOID)pDev, 0);

	if (WD_STATUS_SUCCESS != dwStatus)
	{
		ErrLog("EnableInterrupts: Failed enabling interrupts. Error 0x%lx - %s\n",
			dwStatus, (const char *)Stat2Str(dwStatus));
	}
	return dwStatus;
}

/** @brief Disable PCIe interrupts
 *
 * Disable the generation of PCIe interrupts by the PCIe interface.
 * @retval Status code
 */
DWORD AlphiBoard::DisableInterrupts()
{
	DWORD dwStatus;
//	PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;

	/* Validate the WDC device handle */
	if (!IsValidDevice((const CHAR *)"DisableInterrupts"))
		return WD_INVALID_PARAMETER;

	/* Check whether interrupts are already disabled */
	if (!WDC_IntIsEnabled(hDev))
	{
//		ErrLog("DisableInterrupts: Interrupts are already disabled ...\n");
		return WD_OPERATION_ALREADY_DONE;
	}

	/* Disable interrupts */
	dwStatus = WDC_IntDisable(hDev);
	if (WD_STATUS_SUCCESS != dwStatus)
	{
//		ErrLog("DisableInterrupts: Failed disabling interrupts. Error 0x%lx - %s\n",
//			dwStatus, (const char *)Stat2Str((int)dwStatus));
	}

	return dwStatus;
}
