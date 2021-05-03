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
// OR INABILITY TO USE THE SOFTWARE.  Alphi assumes no responsibility for
// for the maintenance or support of the SOFTWARE
//
/** @file PCIeMini_DTMS.cpp
 * @brief Implementation of the PCIeMini_AVIO board class.
 */

 // Maintenance Log
//---------------------------------------------------------------------
// v1.0		2/23/2020	phf	Written
//---------------------------------------------------------------------

#include <stdio.h>
#include "PCIeMini_DTMS.h"

/* command mailbox
*/

/*! The constructor does not take any parameter. The board is not actually
 usable until the open method connects it to real hardware.
 */
PCIeMini_DTMS::PCIeMini_DTMS() :
	AlphiBoard(0x13c5, 0x508)
{
	ledPio = NULL;
	dac = NULL;
	dtms = NULL;
	ctrlReg = NULL;
	statusReg = NULL;
	rstCtrl = NULL;
}

PCIeMini_DTMS::~PCIeMini_DTMS()
{
	close();
}

//! Open: connect to an actual board
/*!
	\param brdNbr The board number is actually system dependent but if you have only one board, it should be 0.
	\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_DTMS::open(int brdNbr)
{
	if (brdNbr < 0 || brdNbr>9)
		return ERRCODE_INVALID_BOARD_NUM;
	HRESULT result = AlphiBoard::Open(brdNbr);
	if (result != ERRCODE_SUCCESS) {
		return result;
	}

	// create the device objects

	ledPio = new AlteraPio(getBar2Address(ledPio_offset), AlteraPio::CAP_OUTPUT);
	ctrlReg = new DtmsCtrlReg(getBar2Address(ctrlReg_offset));
	dac = new DAC161S997(getBar2Address(dac_offset), ctrlReg);
	dtms = new LTC2984(getBar2Address(dtms_offset), ctrlReg);
	statusReg = new ParallelInput(getBar2Address(statusReg_offset));
	rstCtrl = new AlteraPio(getBar2Address(ledPio_offset), AlteraPio::CAP_OUTPUT);

	return ERRCODE_NO_ERROR;
}

//! Close the connection to a board object and free the resources
/*!
 *		@retval  ERRCODE_NO_ERROR if successful.
 */
PCIeMini_status PCIeMini_DTMS::close()
{
	PCIeMini_status st;

	st = AlphiBoard::Close();
	if (st != ERRCODE_NO_ERROR)
		return st;

	delete 	ledPio;
	delete	dac;
	delete	dtms;
	delete	ctrlReg;
	delete	statusReg;
	delete	rstCtrl;

	ledPio = NULL;
	dac = NULL;
	dtms = NULL;
	ctrlReg = NULL;
	statusReg = NULL;
	rstCtrl = NULL;

	return ERRCODE_NO_ERROR;
}

PCIeMini_status PCIeMini_DTMS::enableLtc2984Irq(MINIPCIE_INT_HANDLER uicr, uint8_t mask)
{
	// Configure the parallel inputs
	statusReg->setEdgeCapture(status_irqLtc2984Mask);
	statusReg->setIrqEnable(status_irqLtc2984Mask);

	// Configure the PCI interrupts
	hookInterruptServiceRoutine(uicr);
	enableInterrupts(0xffff);
	return ERRCODE_NO_ERROR;
}

PCIeMini_status PCIeMini_DTMS::disableLtc2984Irq()
{
	// disable the PCI interrupts
	disableInterrupts();
	unhookInterruptServiceRoutine();

	// reset the parallel inputs

	return ERRCODE_NO_ERROR;
}


