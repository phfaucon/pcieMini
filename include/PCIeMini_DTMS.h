#pragma once

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
/** @file PCIeMini_AVIO.h
* @brief Definitition of the PCIeMini_AVIO board class.
*/

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		2/23/2021	phf	Written
//---------------------------------------------------------------------

#include <stdint.h>
#include <iostream>
//using namespace std;
#include "AlphiDll.h"
#include "AlphiBoard.h"
#include "AlphiErrorCodes.h"
#include "IrigDecoder.h"
#include "AlteraPio.h"
#include "ParallelInput.h"
#include "PcieCra.h"
#include "SpiOpenCore.h"
#include "DAC161S997.h"
#include "LTC2984.h"
#include "DtmsCtrlReg.h"

// check DEMO CIRCUIT 2211A

//! PCIeMini_AVIO controller board object
class DLL PCIeMini_DTMS : public AlphiBoard
{
public:
	PCIeMini_DTMS();
	~PCIeMini_DTMS();

	PCIeMini_status open(int brdNbr);
	PCIeMini_status close();
	PCIeMini_status reset();

	AlteraPio* ledPio;					///< LED control
	DtmsCtrlReg* ctrlReg;				///< Control register
	ParallelInput* statusReg;			///< Status register
	AlteraPio* rstCtrl;					///< Reset control
	DAC161S997* dac;					///< SPI interface to the DAC161S997
	LTC2984* dtms;

	// Status register
	static const uint16_t status_ioReadbackMask = 0x000f;
	static const uint16_t status_ioReadbackBit = 0x0000;
	static const uint16_t status_ocReadbackMask = 0x0030;
	static const uint16_t status_ocReadbackBit = 0x004;
	static const uint16_t status_irqDacMask = 0x0040;
	static const uint16_t status_irqLtc2984Mask = 0x0080;

	inline uint16_t getStatus(void)
	{
		return statusReg->getData();
	}


	/** @brief Read the open-collector outputs
	 * @retval Return the programming of the open-collector outputs. This is not the state of the output line.
	 */
	inline uint8_t getInputs(void)
	{
		return statusReg->getData() & 0x0f;		// only the 4 lower bits
	}

	/** @brief Set the open-collector outputs
	 * @param data State of the open-collector outputs.
	 */
	inline void setOcOutputs(uint16_t data)
	{
		ctrlReg->setOcOutputs(data);
	}

	/** @brief Read the open-collector outputs
	 * @retval Return the programming of the open-collector outputs. This is not the state of the output line.
	 */
	inline uint8_t getOcOutputs(void)
	{
		return ctrlReg->getOcOutputs();
	}

	/** @brief Set the IO outputs
	 * @param data State of the open-collector outputs.
	 */
	inline void setOutputs(uint16_t data)
	{
		ctrlReg->setOutputs(data);
	}

	/** @brief Read the outputs programming
	 * @retval Return the programming of the outputs. This is not the state of the I/O line.
	 */
	inline uint8_t getOutputs(void)
	{
		return ctrlReg->getOutputs();
	}

	/** @brief Set the IO lines direction
	 * @param data The lower 4-bits are used. A 1 in a bit means that the corresponding line is an output.
	 */
	inline void setOutputDirection(uint16_t data)
	{
		ctrlReg->setOutputDirection(data);
	}

	/** @brief Read the outputs direction programming
	 * @retval The lower 4-bits are used. A 1 in a bit means that the corresponding line is an output.
	 */
	inline uint8_t getOutputDirection(void)
	{
		return ctrlReg->getOutputDirection();
	}

	/** @brief Set the relay position
	 * @param data The lower 2-bits are used. A 1 in a bit means that the relay is enabled.
	 */
	inline void setRelays(uint16_t data)
	{
		ctrlReg->setRelays(data);
	}

	/** @brief Read the relays programming
	 * @retval The lower 2-bits are used. A 1 in a bit means that the relay is enabled.
	 */
	inline uint8_t getRelays(void)
	{
		return ctrlReg->getRelays();
	}


	/** @brief Enable the 5V power
	 * @param data True if the 5V power is enabled.
	 */
	inline void setPower5v(bool power5V)
	{
		ctrlReg->setPower5v(power5V);
	}

	/** @brief Read the 5V programming
	 * @retval True if the 5V power is enabled.
	 */
	inline bool getPower5v(void)
	{
		return ctrlReg->getPower5v();
	}

	/** @brief Set the state of the LTC2984 reset line
	 *  @param rst When true set the reset line as active (the state of the line is low)
	 */
	inline void resetDtms(bool rst)
	{
		ctrlReg->resetDtms(rst);
	}

	/** @brief Reset the LTC2984
	 *
	 *	Activate the reset line for one millisecond and then deactivate it. The control register new position reflects the deactivated reset line.
	 */
	inline void resetDtms(void)
	{
		ctrlReg->resetDtms();
	}

	/** @brief Set the state of the DAC161S997 reset line
	 *  @param rst When true set the reset line as active (the state of the line is low)
	 */
	inline void resetDac(bool rst)
	{
		ctrlReg->resetDac(rst);
	}

	/** @brief Reset the DAC161S997
	 *
	 *	Activate the reset line for one millisecond and then deactivate it. The control register new position reflects the deactivated reset line.
	 */
	inline void resetDac(void)
	{
		ctrlReg->resetDac();
	}

	PCIeMini_status enableLtc2984Irq(MINIPCIE_INT_HANDLER uicr, uint8_t mask);
	PCIeMini_status disableLtc2984Irq();


private:
	// Board configuration: These are the offsets of the different hardware components within BAR2
	static const uint32_t	ledPio_offset = 0x0010;		///< R/W   32-bit	LEDs
	static const uint32_t	ctrlReg_offset = 0x0050;	///< 32-bit	Control register
	static const uint32_t	statusReg_offset = 0x0060;		///< 32-bit	General Purpose Inputs
	static const uint32_t	avComp_offset = 0x0080;		///< AV comparator
	static const uint32_t	rstCtrl_offset = 0x0070;	///< Reset control
	static const uint32_t	dtms_offset = 0x0100;	///< SPI interface to the LTC2984
	static const uint32_t	dac_offset = 0x0180;	///< DAC161S997 current generator address
};
