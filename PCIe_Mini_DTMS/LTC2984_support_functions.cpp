/**

  @file LTC2984_support_functions.cpp

This file contains all the support functions used by the LTC2984 in the main program.


http://www.analog.com/en/products/analog-to-digital-converters/integrated-special-purpose-converters/digital-temperature-sensors/LTC2984.html

http://www.analog.com/en/products/analog-to-digital-converters/integrated-special-purpose-converters/digital-temperature-sensors/LTC2984#product-evaluationkit
*/
#include "stdio.h"
#include "stdint.h"
#include "LTC2984.h"

/** @brief Program the part
* 
 * @param channel_number LTC2984 channel number.
 * @param channel_assignment_data Bit pattern used to program the channel. Refer to the LTC2984 manual.
 * @param verify When true, read back the chip to verify that the data was stored properly.
 * @retval ERRCODE_NO_ERROR, or ERRCODE_INTERNAL_ERROR if the read back was in error.
 */
PCIeMini_status LTC2984::assign_channel(uint8_t channel_number, uint32_t channel_assignment_data, bool verify)
{
	if(channel_number == 0 || channel_number > nbrOfChannels) {
		printf ("Error invalid channel number %d\n", channel_number);
		return ERRCODE_INTERNAL_ERROR;
	}
  uint16_t start_address = get_start_address(CH_ADDRESS_BASE, channel_number);
  transfer_four_bytes(WRITE_TO_RAM, start_address, channel_assignment_data);
  if (verify) {
  	uint32_t data_readback = transfer_four_bytes(READ_FROM_RAM, start_address, channel_assignment_data);
  	uint32_t rb = 0;
  	for (int i=0; i<4; i++) {
  		rb = (rb << 8) +transfer_byte(READ_FROM_RAM, start_address+i, 0);
  	}
    if (data_readback != channel_assignment_data) {
  	  printf("Error configuring channel %d: Wrote 0x%08x, read back 0x%08x, rb 0x%08x\n", channel_number, channel_assignment_data, data_readback, rb);
  	  return ERRCODE_INTERNAL_ERROR;
    }
    else {
    	  printf("Success configuring channel %d: Wrote 0x%08x, read back 0x%08x, rb 0x%08x\n", channel_number, channel_assignment_data, data_readback, rb);
    }
  }
  return ERRCODE_NO_ERROR;
}

/** @brief EEPROM transfer
 *
 * Read from or write to the EEPROM.
 * 
 * @param  eeprom_read_or_write To read from the EEPROM, use READ_FROM_EEPROM, to write to the EEPROM, use WRITE_TO_EEPROM.
 * @retval ERRCODE_NO_ERROR, or ERRCODE_INTERNAL_ERROR if the read back was in error.
 *
 */
PCIeMini_status LTC2984::eeprom_transfer(uint8_t eeprom_read_or_write)
{
	uint8_t eeprom_status;
	printf("** EEPROM transfer started ** \n");

	// Set EEPROM key
	transfer_four_bytes(WRITE_TO_RAM, EEPROM_START_ADDRESS, EEPROM_KEY);

	// Set EEPROM read/write
	transfer_byte(WRITE_TO_RAM, COMMAND_STATUS_REGISTER, eeprom_read_or_write);

	// Wait for read/write to finish
	wait_for_process_to_finish();

	// Check for success
	eeprom_status = transfer_byte(READ_FROM_RAM, EEPROM_STATUS_REGISTER, 0);
	if (eeprom_status == 0)
	{
	  return ERRCODE_NO_ERROR;
	}
	else
	{
		printf("** EEPROM transfer had a problem. Status byte = %d\n", eeprom_status);
		return ERRCODE_INTERNAL_ERROR;
	}
}

/** @brief Print channel measurement
* 
* @param channel_number LTC2984 channel number.
* @param channel_output Read as VOLTAGE or TEMPERATURE
*/
void LTC2984::measure_channel(uint8_t channel_number, uint8_t channel_output)
{
    convert_channel(channel_number);
    get_result(channel_number, channel_output);
}


/** @brief Start conversion on one channel
 *
 * @param channel_number LTC2984 channel number.
  *
 */
void LTC2984::convert_channel(uint8_t channel_number)
{
  // Start conversion
  transfer_byte(WRITE_TO_RAM, COMMAND_STATUS_REGISTER, COMMAND_REG_START | channel_number);
  wait_for_process_to_finish();
}

/** @brief check if the LTC2984 is busy */
bool LTC2984::isLTC2984Busy(void)
{
	uint8_t data;
	data = transfer_byte(READ_FROM_RAM, COMMAND_STATUS_REGISTER, 0);
	return (data & COMMAND_REG_DONE) == 0;
}

/** @brief wait for the current action to be finished */
void LTC2984::wait_for_process_to_finish(void)
{
	while (isLTC2984Busy());
}

/** @brief Get results
 *
 * Display a channel conversion results
 * @param chip_select SPI chip select number: always 1 on the PCIeMini_DTMS board
 * @param channel_number LTC2984 channel number.
 * @param channel_output Read as VOLTAGE or TEMPERATURE
 */
void LTC2984::get_result(uint8_t channel_number, uint8_t channel_output)
{
	uint32_t raw_data;
	uint8_t fault_data;
	uint16_t start_address = get_start_address(CONVERSION_RESULT_MEMORY_BASE, channel_number);
	uint32_t raw_conversion_result;

	raw_data = transfer_four_bytes(READ_FROM_RAM, start_address, 0);

	printf("\nChannel %d: %s\n", channel_number, (channel_output != VOLTAGE)?"TEMPERATURE":"VOLTAGE");
	printf("Address 0x%02x, value 0x%08x\n", start_address, raw_data);

	// 24 LSB's are conversion result
	raw_conversion_result = raw_data & 0xFFFFFF;
	print_conversion_result(raw_conversion_result, channel_output);

	// If you're interested in the raw voltage or resistance, use the following
	if (channel_output != VOLTAGE)
	{
		double value;
		value = read_voltage_or_resistance_results(channel_number);
		printf("  Voltage or resistance = %f\n", value);

	}

	// 8 MSB's show the fault data
	fault_data = raw_data >> 24;
	print_fault_data(fault_data);
}

/** @brief print the conversion result as a temperature or a voltage .
* @param raw_conversion_result 24-bit value from the result array.
* @channel_output TEMPERATURE or VOLTAGE.
 */
void LTC2984::print_conversion_result(uint32_t raw_conversion_result, uint8_t channel_output)
{
  int32_t signed_data = raw_conversion_result;
  float scaled_result;

  // Convert the 24 LSB's into a signed 32-bit integer
  if(signed_data & 0x800000)
    signed_data = signed_data | 0xFF000000;

  // Translate and print result
  if (channel_output == TEMPERATURE)
  {
    scaled_result = float(signed_data) / 1024;
    printf("  Temperature = %f\n", scaled_result);
  }
  else if (channel_output == VOLTAGE)
  {
    scaled_result = float(signed_data) / 2097152;
    printf("  Direct ADC reading in V = %f\n", scaled_result);
  }

}


/** @brief Read results on one channel from the LTC2984
 *
 * @param chip_select SPI chip select number: always 1 on the PCIeMini_DTMS board
 * @param channel_number LTC2984 channel number.
 * @param channel_output Read as VOLTAGE or TEMPERATURE
 */
double LTC2984::read_voltage_or_resistance_results(uint8_t channel_number)
{
  int32_t raw_data;
  double voltage_or_resistance_result;
  uint16_t start_address = get_start_address(VOUT_CH_BASE, channel_number);

  raw_data = transfer_four_bytes(READ_FROM_RAM, start_address, 0);
  voltage_or_resistance_result = (double)raw_data/1024;
  return voltage_or_resistance_result;
}


/** @brief Translate the fault byte into usable fault data and print it out
*/
void LTC2984::print_fault_data(uint8_t fault_byte)
{
  //
	if (fault_byte == 1) {
		printf("DATA IS VALID!\n");
		return;
	}

	printf("  FAULT DATA = %02x", fault_byte);
	if (fault_byte & SENSOR_HARD_FAILURE)
		printf("  - SENSOR HARD FALURE\n");
	if (fault_byte & ADC_HARD_FAILURE)
		printf("  - ADC_HARD_FAILURE\n");
	if (fault_byte & CJ_HARD_FAILURE)
		printf("  - CJ_HARD_FAILURE\n");
	if (fault_byte & CJ_SOFT_FAILURE)
		printf("  - CJ_SOFT_FAILURE\n");
	if (fault_byte & SENSOR_ABOVE)
		printf("  - SENSOR_ABOVE\n");
	if (fault_byte & SENSOR_BELOW)
		printf("  - SENSOR_BELOW\n");
	if (fault_byte & ADC_RANGE_ERROR)
		printf("  - ADC_RANGE_ERROR\n");
	if (!(fault_byte & VALID))
		printf("INVALID READING !!!!!!\n");
	if (fault_byte == 0b11111111)
		printf("CONFIGURATION ERROR !!!!!!\n");
}


// ******************************
// Misc support functions
// ******************************
uint16_t LTC2984::get_start_address(uint16_t base_address, uint8_t channel_number)
{
	if(channel_number == 0 || channel_number > nbrOfChannels) {
		printf ("Error invalid channel number %d\n", channel_number);
		return base_address;
	}

	return base_address + 4 * (channel_number-1);
}


bool LTC2984::is_number_in_array(uint8_t number, uint8_t *array, uint8_t array_length)
// Find out if a number is an element in an array
{
  bool found = false;
  for (uint8_t i=0; i< array_length; i++)
  {
    if (number == array[i])
    {
      found = true;
    }
  }
  return found;
}









