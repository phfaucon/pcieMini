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
/** @file PCIeMini_Synch.cpp
 * @brief Main class PCIeMini_Synch implementation.
 */

 // Maintenance Log
//---------------------------------------------------------------------
// v1.0		7/23/2020	phf	Written
//---------------------------------------------------------------------

#include <stdio.h>
#include "PCIeMini_Synch.h"
/* command mailbox 
*/

/*! The constructor does not take any parameter. The board is not actually
 usable until the open method connects it to real hardware.
 */
PCIeMini_Synch::PCIeMini_Synch() :
	AlphiBoard(0x13c5, 0x508)
{
	isOpen = false;

	// initialize all the local pointers to NULL until the board is opened
	uicr = NULL;
	ledPio = NULL;

	controlRegister = NULL;
	statusRegister = NULL;
	da = NULL;
	sync = NULL;
	spi_da = NULL;
}

//! Open: connect to an actual board
/*!
	\param brdNbr The board number is actually system dependent but if you have only one board, it should be 0.
	\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_Synch::open(int brdNbr)
{
	if (brdNbr<0 || brdNbr>9) 
		return ERRCODE_INVALID_BOARD_NUM;
	HRESULT result = AlphiBoard::Open(brdNbr);
	if (result != WD_STATUS_SUCCESS) {
		return result;
	}

	if (verbose) {
			std::cout << "bar #0 address = 0x" << std::hex << bar0.Address << std::endl;
			std::cout << "bar #2 address = 0x" << std::hex << bar2.Address << std::endl;
	}
	
	// create the device objects
	ledPio = (uint32_t *)getBar2Address(led_pio_offset);

	controlRegister =  new ControlRegister(getBar2Address(gpOutput_offset));

	statusRegister = new StatusRegister(getBar2Address(gpInput_offset));
	sync = new Rd19231(getBar2Address(sync_offset), statusRegister, controlRegister);

	// D/A control
	da = new Ltc2664_av(getBar2Address(ltc2664_av_offset));
	spi_da = new Ltc2664_SPI(getBar2Address(spi_da_offset), controlRegister);

	isOpen = true;

	return ERRCODE_NO_ERROR;
}

//! Close the connection to a board object and free the resources
/*!
		\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_Synch::close()
{
	isOpen = false;
	ledPio = NULL;
	controlRegister = NULL;
	statusRegister = NULL;
	da = NULL;
	sync = NULL;
	spi_da = NULL;

	AlphiBoard::Close();

	return ERRCODE_NO_ERROR;
}

//! Reset the board controllers
/*!
		\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_Synch::reset()
{
	if (controlRegister != NULL)
		return ERRCODE_INVALID_HANDLE;
	controlRegister->reset();
	statusRegister->reset();
	spi_da->reset();
	AlphiBoard::reset();
	return ERRCODE_NO_ERROR;
}

//! Return a text description corresponding to an error code
/*!
		\return  A pointer to a null terminated character string.
*/
char* PCIeMini_Synch::getErrorMsg(PCIeMini_status errorNbr)
{
	return getAlphiErrorMsg(errorNbr);
}

void PCIeMini_Synch::setLedPio(uint32_t val)
{
	ledPio[0] = val;
}

uint32_t PCIeMini_Synch::getLedPio()
{
	return ledPio[0];
}

