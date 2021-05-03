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
/** @file LTC2984.h
* @brief Definitition of the PCIeMini_AVIO board class.
*/

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		2/23/2021	phf	Written
//---------------------------------------------------------------------

#include "AlphiDll.h"
#include "SpiOpenCore.h"
#include "DtmsCtrlReg.h"
#include "LTC2984_configuration_constants.h"
#include "LTC2984_table_coeffs.h"
#include "LTC2984_support_functions.h"

class DLL ChannelAssignment
{
public:
	uint32_t sensorType;
	virtual uint32_t composeWord(void){ return 0; }
};

class DiodeAssignment : public ChannelAssignment
{

};

class RtdAssignment : public ChannelAssignment
{
	uint32_t rsenseChannel;
	uint32_t numWires;
	uint32_t excitationMode;
	uint32_t excitationCurrent;
	uint32_t curve;
	uint8_t tableStart;
	uint8_t tableLength;
	uint32_t composeWord(void)
	{
		uint32_t assignWord;
		assignWord = sensorType;
		assignWord |= rsenseChannel;
		assignWord |= numWires;
		assignWord |= excitationMode;
		assignWord |= excitationCurrent;
		assignWord |= tableStart << RTD_CUSTOM_ADDRESS_LSB;
		assignWord |= tableLength << RTD_CUSTOM_LENGTH_1_LSB;
		return assignWord;
	}
};

class DLL DirectAdcAssignment : public ChannelAssignment
{
	bool	isDifferential;
};

//! PCIeMini_AVIO controller board object
class DLL LTC2984
{
public:
	static const int nbrOfChannels = 20;
	static const uint8_t chip_select = 1;

	static const uint8_t COMMAND_REG_START = 0x80;
	static const uint8_t COMMAND_REG_DONE = 0x40;

	SpiOpenCore *spi;				///< SPI controller object used to communicate with the HI_8429
	DtmsCtrlReg *ctrlReg;
	ChannelAssignment channels[nbrOfChannels];

	inline LTC2984(volatile void *spiController, DtmsCtrlReg* pioControl)
	{
		spi = new SpiOpenCore(spiController);				///< SPI controller object used to communicate with the HI_8429
		ctrlReg = pioControl;
		spi_init();
	}

	void reset(void);

	void spi_init(void);
	uint32_t transfer_four_bytes(uint8_t ram_read_or_write, uint16_t start_address, uint32_t input_data);
	uint32_t read_four_bytes(uint16_t start_address);
	uint32_t write_four_bytes(uint16_t start_address, uint32_t input_data);
	uint8_t transfer_byte(uint8_t ram_read_or_write, uint16_t start_address, uint8_t input_data);
	uint8_t read_byte(uint16_t start_address);
	uint8_t write_byte(uint16_t start_address, uint8_t input_data);
	bool isLTC2984Busy(void);

	PCIeMini_status assign_channel(uint8_t channel_number, uint32_t channel_assignment_data, bool verify = false);
	void write_custom_table(struct table_coeffs coefficients[64], uint16_t start_address, uint8_t table_length);
	void write_custom_steinhart_hart(uint32_t steinhart_hart_coeffs[6], uint16_t start_address);
	PCIeMini_status eeprom_transfer(uint8_t eeprom_read_or_write);
	void measure_channel(uint8_t channel_number, uint8_t channel_output);
	void convert_channel(uint8_t channel_number);
	void get_result(uint8_t channel_number, uint8_t channel_output);
	void print_conversion_result(uint32_t raw_conversion_result, uint8_t channel_output);
	double read_voltage_or_resistance_results(uint8_t channel_number);
	void print_fault_data(uint8_t fault_byte);


private:
	static uint16_t get_start_address(uint16_t base_address, uint8_t channel_number);
	static bool is_number_in_array(uint8_t number, uint8_t* array, uint8_t array_length);
	void wait_for_process_to_finish(void);

};
