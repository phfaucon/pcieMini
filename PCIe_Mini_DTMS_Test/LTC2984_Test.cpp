/*!



LTC2984.ino:
Generated Linduino code from the LTC2984 Demo Software.
This code (plus the other code in this folder) is designed to be used by a Linduino,
but can also be used to understand how to program the LTC2984.




http://www.analog.com/en/products/analog-to-digital-converters/integrated-special-purpose-converters/digital-temperature-sensors/LTC2984.html

http://www.analog.com/en/products/analog-to-digital-converters/integrated-special-purpose-converters/digital-temperature-sensors/LTC2984#product-evaluationkit

$Revision: 1.7.9 $
$Date: September 7, 2018 $
Copyright (c) 2018, Analog Devices, Inc. (ADI)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of Analog Devices, Inc.

The Analog Devices Linduino is not affiliated with the official Arduino team.
However, the Linduino is only possible because of the Arduino team's commitment
to the open-source community.  Please, visit http://www.arduino.cc and
http://store.arduino.cc , and consider a purchase that will help fund their
ongoing work.
*/





#include <stdio.h>
#include "PCIeMini_DTMS.h"
#include "DtmsTest.h"
#include "LTC2984.h"

#define CHIP_SELECT (1)  // Chip select pin

// Function prototypes

// -------------- Configure the LTC2984 -------------------------------
PCIeMini_status DtmsTest::LTC2984Setup()
{
	PCIeMini_status st = configure_channels();
	if (st!=ERRCODE_NO_ERROR){
		printf("Error: LTC2984 channel configuration failed!\n");
		return st;
	}
	configure_global_parameters();
	return ERRCODE_NO_ERROR;
}

PCIeMini_status DtmsTest::LTC2984AdcSetup()
{
	PCIeMini_status st = configure_channels_as_adc();
	if (st!=ERRCODE_NO_ERROR){
		printf("Error: LTC2984 channel configuration failed!\n");
		return st;
	}
	configure_global_parameters();
	return ERRCODE_NO_ERROR;
}

PCIeMini_status DtmsTest::configure_channels_as_adc()
{
	uint8_t channel_number;
	uint32_t channel_assignment_data;
	LTC2984 *ltc = dut->dtms;
	uint32_t data_readback;
	PCIeMini_status st = ERRCODE_NO_ERROR;
	PCIeMini_status st2;

	for (uint8_t i = 1; i <= ltc->nbrOfChannels; i++)
	{
		// ----- All channels: Assign Direct ADC -----
		channel_assignment_data =
		SENSOR_TYPE__DIRECT_ADC |
		DIRECT_ADC_SINGLE_ENDED;
		st2 = ltc->assign_channel(i, channel_assignment_data, true);
		if (st2 != ERRCODE_NO_ERROR) {
			st = st2;
		}
	}
	  return st;
}

PCIeMini_status DtmsTest::configure_channels()
{
  uint32_t channel_assignment_data;
  LTC2984 *ltc = dut->dtms;
  PCIeMini_status st = ERRCODE_NO_ERROR;
  PCIeMini_status st2;

  // ----- Channel 2: Assign Type R Thermocouple -----
  channel_assignment_data =
    SENSOR_TYPE__TYPE_R_THERMOCOUPLE |
    TC_COLD_JUNCTION_CH__NONE |
    TC_SINGLE_ENDED |
    TC_OPEN_CKT_DETECT__NO |
    TC_OPEN_CKT_DETECT_CURRENT__10UA;
  st2 = ltc->assign_channel(2, channel_assignment_data, true);
  if (st2 != ERRCODE_NO_ERROR) {
	  st = st2;
  }
  // ----- Channel 4: Assign Sense Resistor -----
  channel_assignment_data =
    SENSOR_TYPE__SENSE_RESISTOR |
    (uint32_t) 0x19000 << SENSE_RESISTOR_VALUE_LSB;		// sense resistor - value: 100.
  st2 = ltc->assign_channel(4, channel_assignment_data, true);
  if (st2 != ERRCODE_NO_ERROR) {
	  st = st2;
  }
  // ----- Channel 5: Assign Direct ADC -----
  channel_assignment_data =
    SENSOR_TYPE__DIRECT_ADC |
    DIRECT_ADC_SINGLE_ENDED;
  st2 = ltc->assign_channel(5, channel_assignment_data, true);
  if (st2 != ERRCODE_NO_ERROR) {
	  st = st2;
  }
  // ----- Channel 9: Assign Type N Thermocouple -----
  channel_assignment_data =
    SENSOR_TYPE__TYPE_N_THERMOCOUPLE |
    TC_COLD_JUNCTION_CH__NONE |
    TC_SINGLE_ENDED |
    TC_OPEN_CKT_DETECT__NO |
    TC_OPEN_CKT_DETECT_CURRENT__10UA;
  st2 = ltc->assign_channel(9, channel_assignment_data, true);
  if (st2 != ERRCODE_NO_ERROR) {
	  st = st2;
  }
  // ----- Channel 11: Assign RTD PT-500 -----
  channel_assignment_data =
    SENSOR_TYPE__RTD_PT_500 |
    RTD_RSENSE_CHANNEL__NONE |
    RTD_NUM_WIRES__4_WIRE |
    RTD_EXCITATION_MODE__ROTATION_SHARING |
    RTD_EXCITATION_CURRENT__50UA |
    RTD_STANDARD__AMERICAN;
  st2 = ltc->assign_channel(11, channel_assignment_data, true);
  if (st2 != ERRCODE_NO_ERROR) {
	  st = st2;
  }
  // ----- Channel 13: Assign Off-Chip Diode -----
  channel_assignment_data =
    SENSOR_TYPE__OFF_CHIP_DIODE |
    DIODE_SINGLE_ENDED |
    DIODE_NUM_READINGS__3 |
    DIODE_AVERAGING_OFF |
    DIODE_CURRENT__20UA_80UA_160UA |
    (uint32_t) 0x100C49 << DIODE_IDEALITY_FACTOR_LSB;		// diode - ideality factor(eta): 1.00299930572509765625
  st2 = ltc->assign_channel(13, channel_assignment_data, true);
  if (st2 != ERRCODE_NO_ERROR) {
	  st = st2;
  }
  return st;
}


PCIeMini_status DtmsTest::configure_global_parameters()
{
	  LTC2984 *ltc = dut->dtms;
  // -- Set global parameters
	  ltc->transfer_byte(WRITE_TO_RAM, 0xF0, TEMP_UNIT__C |
    REJECTION__50_60_HZ);
  // -- Set any extra delay between conversions (in this case, 0*100us)
	  ltc->transfer_byte(WRITE_TO_RAM, 0xFF, 0);

	  return ERRCODE_NO_ERROR;
}

// -------------- Run the LTC2984 -------------------------------------

void DtmsTest::LTC2984MeasurementLoop()
{
	LTC2984 *ltc = dut->dtms;

	ltc->measure_channel(2, TEMPERATURE);      // Ch 2: Type R Thermocouple
	ltc->measure_channel(5, VOLTAGE);          // Ch 5: Direct ADC
	ltc->measure_channel(9, TEMPERATURE);      // Ch 9: Type N Thermocouple
	ltc->measure_channel(11, TEMPERATURE);     // Ch 11: RTD PT-500
	ltc->measure_channel(13, TEMPERATURE);     // Ch 13: Off-Chip Diode
}

void DtmsTest::LTC2984AdcMeasurementLoop ()
{
	LTC2984 *ltc = dut->dtms;

	for (uint8_t i = 1; i <= ltc->nbrOfChannels; i++)
	{
		ltc->measure_channel(i, VOLTAGE);          // Direct ADC
	}
}



