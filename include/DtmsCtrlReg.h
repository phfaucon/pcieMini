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
/** @file DtmsCtrlReg.h
* @brief Definition of bits and operation for the DTMS control register.
*/

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		2/23/2021	phf	Written
//---------------------------------------------------------------------

#include "AlphiDll.h"
#include "AlteraPio.h"

//! PCIeMini_AVIO controller board object
class DLL DtmsCtrlReg : public AlteraPio
{
public:
	/** @brief constructor
	 * @param addr Address of the hardware register in user space.
	 */
	inline DtmsCtrlReg(volatile void* addr)
		: AlteraPio(addr, AlteraPio::CAP_OUTPUT)
	{
		setData(dtmsResetMask | dacResetMask);
	}

	/** @brief Set the open-collector outputs
	 * @param data State of the open-collector outputs.
	 */
	inline void setOcOutputs(uint16_t data)
	{
		uint16_t current = getData() & ~ocOutputMask;
		uint16_t newData = (data << ocOutputBit) & ocOutputMask;
		setData(current | newData);
	}

	/** @brief Read the open-collector outputs
	 * @retval Return the programming of the open-collector outputs. This is not the state of the output line.
	 */
	inline uint8_t getOcOutputs()
	{
		uint16_t current = getData() & ocOutputMask;
		return current >> ocOutputBit;
	}

	/** @brief Set the IO outputs
	 * @param data State of the open-collector outputs.
	 */
	inline void setOutputs(uint16_t data)
	{
		uint16_t current = getData() & ~outputsMask;
		uint16_t newData = (data << outputsBit) & outputsMask;
		setData(current | newData);
	}

	/** @brief Read the outputs programming
	 * @retval Return the programming of the outputs. This is not the state of the I/O line.
	 */
	inline uint8_t getOutputs()
	{
		uint16_t current = getData() & outputsMask;
		return current >> outputsBit;
	}

	/** @brief Set the IO lines direction
	 * @param data The lower 4-bits are used. A 1 in a bit means that the corresponding line is an output.
	 */
	inline void setOutputDirection(uint16_t data)
	{
		uint16_t current = getData() & ~outputDirMask;
		uint16_t newData = (data << outputDirBit) & outputDirMask;
		setData(current | newData);
	}

	/** @brief Read the outputs direction programming
	 * @retval The lower 4-bits are used. A 1 in a bit means that the corresponding line is an output.
	 */
	inline uint8_t getOutputDirection()
	{
		uint16_t current = getData() & outputDirMask;
		return current >> outputDirBit;
	}

	/** @brief Set the relay position
	 * @param data The lower 2-bits are used. A 1 in a bit means that the relay is enabled.
	 */
	inline void setRelays(uint16_t data)
	{
		uint16_t current = getData() & ~relayMask;
		uint16_t newData = (data << relayBit) & relayMask;
		setData(current | newData);
	}

	/** @brief Read the relays programming
	 * @retval The lower 2-bits are used. A 1 in a bit means that the relay is enabled.
	 */
	inline uint8_t getRelays()
	{
		uint16_t current = getData() & relayMask;
		return current >> relayBit;
	}


	/** @brief Enable the 5V power
	 * @param data True if the 5V power is enabled.
	 */
	inline void setPower5v(bool power5V)
	{
		uint16_t current = getData() & ~power5vMask;
		uint16_t newData = power5V ? power5vMask : 0;
		setData(current | newData);
	}

	/** @brief Read the 5V programming
	 * @retval True if the 5V power is enabled.
	 */
	inline bool getPower5v()
	{
		uint16_t current = getData() & power5vMask;
		return current != 0;
	}

	/** @brief Set the state of the LTC2984 reset line
	 *  @param rst When true set the reset line as active (the state of the line is low)
	 */
	inline void resetDtms(bool rst)
	{
		uint16_t current = getData() & ~dtmsResetMask;
		uint16_t newData = rst ? 0 : dtmsResetMask;
		setData(current | newData);
	}

	/** @brief Reset the LTC2984
	 *
	 *	Activate the reset line for one millisecond and then deactivate it. The control register new position reflects the deactivated reset line.
	 */
	inline void resetDtms()
	{
		uint16_t current = getData() & ~dtmsResetMask;
		setData(current);
		Sleep(1);
		current = getData() | dtmsResetMask;
		setData(current);
	}

	/** @brief Set the state of the DAC161S997 reset line
	 *  @param rst When true set the reset line as active (the state of the line is low)
	 */
	inline void resetDac(bool rst)
	{
		uint16_t current = getData() & ~dacResetMask;
		uint16_t newData = rst ? 0 : dacResetMask;
		setData(current | newData);
	}

	/** @brief Reset the DAC161S997
	 *
	 *	Activate the reset line for one millisecond and then deactivate it. The control register new position reflects the deactivated reset line.
	 */
	inline void resetDac()
	{
		uint16_t current = getData() & ~dacResetMask;
		setData(current);
		Sleep(1);
		current = getData() | dacResetMask;
		setData(current);
	}

private:
	static const uint16_t outputsMask = 0x000f;
	static const uint16_t outputsBit = 0;
	static const uint16_t ocOutputMask = 0x30;
	static const uint16_t ocOutputBit = 4;
	static const uint16_t outputDirMask = 0x03c0;
	static const uint16_t outputDirBit = 6;
	static const uint16_t relayMask = 0x0c00;
	static const uint16_t relayBit = 10;
	static const uint16_t power5vMask = 0x1000;
	static const uint16_t dtmsResetMask = 0x2000;
	static const uint16_t dacResetMask = 0x4000;
};
