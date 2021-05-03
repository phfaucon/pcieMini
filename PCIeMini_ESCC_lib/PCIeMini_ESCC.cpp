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

#include <stdio.h>
#include "PCIeMini_ESCC.h"
/* command mailbox 
*/

/*! The constructor does not take any parameter. The board is not actually
 usable until the open method connects it to real hardware.
 */
PCIeMini_ESCC::PCIeMini_ESCC() :
	AlphiBoard(0x13c5, 0x508)
{
	isOpen = false;

	// initialize all the local pointers to NULL until the board is opened
	uicr = NULL;
	sysid = NULL;
	dpr = NULL;
	interruptEnabled = 0;
	ledPio = NULL;
	sccDevice_0 = NULL;
	sccDevice_1 = NULL;
	cra = NULL;
}

//! Open: connect to an actual board
/*!
	\param brdNbr The board number is actually system dependent but if you have only one board, it should be 0.
	\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_ESCC::open(int brdNbr)
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
	sysid = new BoardVersion((volatile uint32_t*)getBar2Address(sysid_offset));
	ledPio = (uint32_t *)getBar2Address(led_pio_offset);
	sccDevice_0 = new SccChannel(getBar2Address(scc_0_offset));
	sccDevice_1 = new SccChannel(getBar2Address(scc_1_offset));
	cra = new PcieCra(getBar0Address(0));
	isOpen = false;

	return ERRCODE_NO_ERROR;
}

//! Close the connection to a board object and free the resources
/*!
		\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_ESCC::close()
{
	isOpen = false;
	delete sccDevice_0;
	sccDevice_0 = NULL;
	delete sccDevice_1;
	sccDevice_1 = NULL;
	delete sysid;
	sysid = NULL;
	ledPio = NULL;

	AlphiBoard::Close();

	return ERRCODE_NO_ERROR;
}

//! Reset the board ARINC 429 controllers
/*!
		\return  ERRCODE_NO_ERROR if successful.
*/
PCIeMini_status PCIeMini_ESCC::reset()
{
	if (sccDevice_0 != NULL)
		return ERRCODE_INVALID_HANDLE;
	return ERRCODE_NO_ERROR;
}

//! Return a text description corresponding to an error code
/*!
		\return  A pointer to a null terminated character string.
*/
char* PCIeMini_ESCC::getErrorMsg(PCIeMini_status errorNbr)
{
	return getAlphiErrorMsg(errorNbr);
}

void PCIeMini_ESCC::setLedPio(uint32_t val)
{
	ledPio[0] = val;
}

uint32_t PCIeMini_ESCC::getLedPio()
{
	return ledPio[0];
}

/*! \brief get pointer to an instance serial channel object.
 *
 * @param  channelNbr Channel number 0, or 1.
 * @return A pointer to a channel object if successful, else NULL.
*/
SccChannel* PCIeMini_ESCC::getScc(int channelNbr)
{
	if (channelNbr == 0)
		return sccDevice_0;
	if (channelNbr == 1)
		return sccDevice_1;
	return NULL;
}

