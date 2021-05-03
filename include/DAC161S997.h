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
/** @file DAC161S997.h
* @brief Definitition of the DAC161S997 class for the DTMS board.
* 
* The class interfaces with the DTMS SPI object to handle of the low level SPI messaging.
*/

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		4/1/2021	phf	Written
//---------------------------------------------------------------------

#include "AlphiDll.h"
#include "DtmsCtrlReg.h"

/** @brief Definitition of the DAC161S997 class for the DTMS board.
*
* The class interfaces with the DTMS SPI object to handle of the low level SPI messaging.
*/
class DLL DAC161S997
{
public:
	SpiOpenCore *spi;				///< SPI controller object used to communicate with the HI_8429
	DtmsCtrlReg *ctrlReg;			///< DTMS control register pointer

	/**
		<b>XFER_REG</b> (WriteOnly) Address= 0x01<br>
		<table>
		    <tr><td>Bit Field 	<td>Field Name		<td>Description
			<tr><td>15:0	<td>XFER[15:0]			<td>When PROTECT_REG_WR is set to 1, then a XFER_REG command is necessary to transfer the previous register write data into the appropriate address. Set this register to 0x00FF to perform a XFER_REG command.
			</table>
	*/
	static const uint8_t XFER_REG_ADDRESS = 0x01;

	/**
	<b>NOP</b>  				Address= 0x02
	<table>
		<tr><td>Bit Field 	<td>Field Name			<td>Description									
		<tr><td>15:0		<td>NOP[15:0]			<td>No Operation. <br>
													A write to this register will not change any device configuration.This command indicates that the SPI connection is functioning and is used to avoid SPI_INACTIVE errors.
	*/
	static const uint8_t NOP_ADDRESS = 0x02;

	/**
	<b>WR_MODE</b>				Address= 0x03; Default= 0x0000
	<table>
		<tr><td>Bit Field 	<td>Field Name			<td>Description									
		<tr><td>0			<td>PROTECT_REG_WR		<td>0: Register write data transfers to appropriate address immediately after CSB goes high. (Default value.)<br>
													1: Enable protected register transfers: all register writes require a subsequent XFER_REG command to finalize the loading of register data.</tr>
			</table>
	*/
	static const uint8_t WR_MODE_ADDRESS = 0x03;

	/**
	<b>DAC CODE</b>			Address= 0x04;	Default= 0x2400,0xE800
	<table>
		<tr><td>Bit Field 	<td>Field Name			<td>Description									
		<tr><td>15:0		<td>DACCODE[15:0]		<td>16-bit natural binary word, where D15 is the MSB, which indicates the desired DAC outputcode.<br>
													Note: the default value of this register is based on the state of the ERR_LVL pin during startup or reset.
	</table>
	*/
	static const uint8_t DACCODE_ADDRESS = 0x04;

	/**
	<b>ERR_CONFIG</b>			Address= 0x05;	Default= 0x0102<br>
	<table>
		<tr><th>Bit Field 	<th>Field Name			<th>Description									
		<tr><td>10:8		<td>L_RETRY_TIME[2:0]	<td>L_RETRY_TIME sets the time interval between successive attempts to reassert the<br>
									desired DAC CODE output current when a loop error is present.This has no effect if<br>
									either MASK_LOOP_ERR is set to 1 or if DIS_RETRY_LOOP is set to 1.LOOP Retry time = (L_RETRY_TIME+ 1) × 50 ms<br>
									Default value= 1 (100ms).
		<tr><td>7			<td>DIS_RETRY_LOOP		<td>0: Whena loop error is occurring, periodically attempt to send desired DAC CODE
													output current instead of the set ERR_LOW current.The interval between attempts is
													set by L_RETRY_TIMER. Default value.<br>
													1: Do not periodically reassert DAC CODE output when a loop error is present;
													reassert DAC CODE after STATUS Register is readout.
		<tr><td>6			<td>MASK_LOOP_ERR		<td>0: When a LOOP error is detected the DAC161S997 outputs the current indicated by
													ERR_LOW instead of DAC CODE. Default value.<br>
													1: When a Loop Error is detected the DAC161S997 tries to maintain DAC CODE current on pin OUT.
		<tr><td>5			<td>DIS_LOOP_ERR_ERRB	<td>0: When a LOOP error is detected the DAC161S997 drives ERRB pin low.
									Default value.<br>
									1: Whena LOOP error is detected the DAC161S997 does not drive ERRB pin low.
		<tr><td>4			<td>MASK_SPI_ERR		<td>0: SPI timeout errors changethe OUT pin current to an error value, which is determined by the ERRLVL pin and contents of ERR_LOW or ERR_HIGH.<br>
											Note:MASK_SPI_TOUT must be set to 0 for this to be reported. Defaultvalue.<br>
											1: SPI timeout errors do not change the OUT pin current to an error value.
		<tr><td>3:1			<td>SPI_TIMEOUT[2:0]	<td>SPI_TIMEOUT sets the time interval for SPI timeout error reporting. After each SPI
									write command, an internal timer is reset; if no subsequent write occurs before the
									timer reaches SPI timeout, a SPI timeout error is reported. SPI_ERROR reporting is
									inhibited by setting MASK_SPI_TOUT.<br>
									A NOP write is considered a valid write and resets the timer without changing the
									device configuration.<br>
									<i>SPI Timeout= (SPI_TIMEOUT+ 1) × 50 ms</i><br>
									SPI_TIMEOUT default value = 1 (100ms)</tr>
		<tr><td>0			<td>MASK_SPI_TOUT		<td>0: SPI time-out error reporting is enabled. A SPI time-out error drives ERRB low when a
									SPI Time-out error occurs. Default value.<br>
									1: SPI time-out error reporting is inhibited.
	</table>
	*/
	static const uint8_t ERR_CONFIG_ADDRESS = 0x05;

	/**
	<b>ERR_LOW</b>				Address= 0x06;	Default= 0x2400
	<table>
		<tr><td>Bit Field 	<td>Field Name			<td>Description									
		<tr><td>15:8		<td>ERR_LOW[7:0]		<td>Under some error conditions the output current corresponding to this value is the DAC
													output, regardless of the value of DAC CODE. The ERR_LOW value is used as the
													upper byte of the DAC CODE, while the lower byte is forced to 0x00.ERR_LOW must be between 0x00 (0mA) and 0x80 (12mA).The DAC161S997
													ignores any value outside of that range and retains the previous value in the register.
													The default value is 0x24, which corresponds to approximately 3.37mA on pin OUT.
			</table>
	*/
	static const uint8_t ERR_LOW_ADDRESS = 0x06;

	/**
	<b>ERR_HIGH</b>			Address= 0x07;	Default= 0xE800
	<table>
		<tr><td>Bit Field 	<td>Field Name	<td>Description
									
		<tr><td>15:8	<td>ERR_HIGH[7:0]		<td>Under some error conditions the output current corresponding to this value is the DAC
									output,regardless of the value of DACCODE. The ERR_HIGH value is used as the
									upper byte of the DACCODE, while the lower byte is forced to 0x00.
									ERR_HIGH must be greater than or equal to 0x80(12 mA). The DAC161S997
									ignores any value below 0x80 and retains the previous value in the register.
									The default value is 0xE8, which corresponds to approximately 21.8mA on pin OUT.
	</table>
	*/
	static const uint8_t ERR_HIGH_ADDRESS = 0x07;

	/**
	<b>RESET</b>				Address= 0x08
	<table>
		<tr><td>Bit Field 	<td>Field Name			<td>Description									
		<tr><td>15:0		<td>RESET[15:0]			<td>Write 0xC33C to the RESET register followed by a NOP to reset the device. All writable registers are returned to default values.
	</table>
	*/
	static const uint8_t RESET_ADDRESS = 0x08;

	/**
		<b>STATUS(Read-Only)</b>	Address= 0x09 or 0x7F
		<table>
			<tr><td>Bit Field</td> 	<td>Field Name</td>	<td>Description	</td></tr>
			<tr><td>7:5</td>		<td>DAC_RES[2:0]</td>	<td>DACresolution<br>
															On DAC161S997, returns a 111.</td></tr>
			<tr><td>4</td>			<td>ERRLVL_PIN</td>		<td>Returns the state of the ERRLVL pin:<br>
															1 = ERRLVL pin is tied HIGH<br>
															0 = ERRLVLpin is tied LOW</td></tr>
			<tr><td>3</td>			<td>FERR_STS</td>		<td>Frame-error status sticky bit<br>
															1 = A frame error has occurred since the last STATUS read.<br>
															0 = No frame error occurred since the last STATUS read.<br>
															This error is cleared by reading the STATUS register. A frame error is caused by an
															incorrect number of clocks during a register write. A register write without an integer
															multiple of 24 clock cycles will cause a Frame error.</td></tr>
			<tr><td>2</td>		<td>SPI_TIMEOUT_ERR		<td>SPI time out error<br>
										1 = The SPI interface has not received a valid command within the interval set by SPI_TIMEOUT.<br>
										0 = The SPI interface has received a valid command within the interval set by SPI_TIMEOUT<br>
										If this error occurs, it is cleared with a properly formatted write command to a valid address.</td></tr>
			<tr><td>1</td>		<td>LOOP_STS</td>	<td>Loop status sticky bit<br>
										1 = A loop error has occurred since last read of STATUS.<br>
										0 = No loop error has occurred since last read of STATUS.<br>
										Returns the loop error status. When the value in this register is 1, the DAC161S997 is
										unable to maintain the output current set by DACCODE at some point since the last
										STATUS read. This indicator clears after reading the STATUS register.</td></tr>
			<tr><td>0</td> 		<td>CURR_LOOP_STS</td>	<td>Current loop status<br>
										1 = A loop error is occurring.<br>
										0 = No loop error is occurring.<br>
										Returns the current Loop error status. When the value in this register is 1, the
										DAC161S997 is unable to maintain the output current set by DACCODE.</td></tr>
		</table>
	*/
	static const uint8_t STATUS_REG_ADDRESS = 0x09;

	static const uint8_t STATUS_ERRLVL_PIN = 0x10;	///< Status register: ErrLevel pin state mask
	static const uint8_t STATUS_FERR_STS = 0x08;	///< Status register: Frame error sticky bit
	static const uint8_t SPI_TIMEOUT_ERR = 0x04;	///< Status register: Time-out error
	static const uint8_t SPI_LOOP_STS = 0x02;		///< Status register: Loop error sticky bit
	static const uint8_t SPI_CURR_LOOP_STS = 0x01;	///< Status register: Current loop error

	/** @brief Constructor
	* @param spiController Pointer to the DAC SPI controller
	* @param pioControl Pointer to the DTMS control register
	*/
	inline DAC161S997(volatile void *spiController, DtmsCtrlReg* pioControl)
	{
		spi = new SpiOpenCore(spiController);				//
		ctrlReg = pioControl;
		spi_init();
	}

	/** @brief SPI controller initialization
	* 
	* Set default values in the DAC SPI controller
	*/
	inline void spi_init(void)
	{
		// 24-bit words
		// upper bit first
		// latching data on leading edges
		spi->setSpiControl(SpiOpenCore::SPI_CTRL_ASS | SpiOpenCore::SPI_CTRL_TX_NEGEDGE | 0x18);	// automatic slave select + 24 bits
		spi->setSpiDivider(0x05); 								// DAC161S997 SPI f_max = 10 MHz.  62.5 MHz / ((5 + 1) * 2 ) = 5.2 MHz
		spi->selectSpiSlave(0xff); 								// we only have 1

	}

	/** @brief Send the XFR command
	* 
	* When PROTECT_REG_WR is set to 1, then a XFER_REG command is necessary to transfer the previous register write data into the appropriate address. 
	*/
	inline void SendXfrCmd(void)
	{
		sendCommand(XFER_REG_ADDRESS, 0x00ff);
	}

	/** @brief Send the NOP command
	*
	* This command will not change any device configuration.This command indicates that the SPI connection is functioning and is used to avoid SPI_INACTIVE errors.
	*/
	inline uint32_t SendNopCmd(void)
	{
		return sendCommand(NOP_ADDRESS, 0);
	}

	/** @brief Set the DAC protected mode
	*
	* When protected register transfersmode is enabled, all register writes require a subsequent XFER_REG command to finalize the loading of register data.
	* @param protectedMode True will enable the protected mode.
	*/
	inline void SetProtectedMode(bool protectedMode)
	{
		sendCommand(WR_MODE_ADDRESS, protectedMode?1:0);
	}

	/** @brief Set the DAC value
	*
	* Set the DAC current output.
	* @param val 16-bit natural binary word, where D15 is the MSB, which indicates the desired DAC outputcode.
	*/
	inline uint32_t SetDacCode(uint16_t val)
	{
		return sendCommand(DACCODE_ADDRESS, val);
	}

	/** @brief Set the error configuration
	*
	* @param val The new error configuration.
	*/
	inline uint32_t SetErrorConfig(uint16_t val)
	{
		return sendCommand(ERR_CONFIG_ADDRESS, val);
	}

	/** @brief Read the status register
	
	<table>
			<tr><td>Bit Field</td> 	<td>Field Name</td>	<td>Description	</td></tr>
			<tr><td>7:5</td>		<td>DAC_RES[2:0]</td>	<td>DACresolution<br>
															On DAC161S997, returns a 111.</td></tr>
			<tr><td>4</td>			<td>ERRLVL_PIN</td>		<td>Returns the state of the ERRLVL pin:<br>
															1 = ERRLVL pin is tied HIGH<br>
															0 = ERRLVLpin is tied LOW</td></tr>
			<tr><td>3</td>			<td>FERR_STS</td>		<td>Frame-error status sticky bit<br>
															1 = A frame error has occurred since the last STATUS read.<br>
															0 = No frame error occurred since the last STATUS read.<br>
															This error is cleared by reading the STATUS register. A frame error is caused by an
															incorrect number of clocks during a register write. A register write without an integer
															multiple of 24 clock cycles will cause a Frame error.</td></tr>
			<tr><td>2</td>		<td>SPI_TIMEOUT_ERR		<td>SPI time out error<br>
										1 = The SPI interface has not received a valid command within the interval set by SPI_TIMEOUT.<br>
										0 = The SPI interface has received a valid command within the interval set by SPI_TIMEOUT<br>
										If this error occurs, it is cleared with a properly formatted write command to a valid address.</td></tr>
			<tr><td>1</td>		<td>LOOP_STS</td>	<td>Loop status sticky bit<br>
										1 = A loop error has occurred since last read of STATUS.<br>
										0 = No loop error has occurred since last read of STATUS.<br>
										Returns the loop error status. When the value in this register is 1, the DAC161S997 is
										unable to maintain the output current set by DACCODE at some point since the last
										STATUS read. This indicator clears after reading the STATUS register.</td></tr>
			<tr><td>0</td> 		<td>CURR_LOOP_STS</td>	<td>Current loop status<br>
										1 = A loop error is occurring.<br>
										0 = No loop error is occurring.<br>
										Returns the current Loop error status. When the value in this register is 1, the
										DAC161S997 is unable to maintain the output current set by DACCODE.</td></tr>
		</table>

	* @retval Status register content.
	*/
	inline uint8_t getStatus(void)
	{
		sendCommand(STATUS_REG_ADDRESS + 0x80, 0);
		uint32_t val = SendNopCmd();
		return val;
	}

	/** @brief Low level generic SPI command
	* @param address Command address
	* @data data to send to the register
	* @retval Value read on the SPI bus
	*/
	inline uint32_t sendCommand(uint8_t address, uint16_t data)
	{
		uint32_t read_val;
		uint32_t write_val = ((uint32_t)address << 16) + data;
		read_val = spi->rw(write_val);
		return read_val;

	}

	/** @brief Debug print utility
	* 
	* @param status Content of the status register to be decoded.
	*/
	inline void printStatus(uint8_t status)
	{
		printf ("DAC161S997 status register:\n");
		if ((status & 0xe0) != 0xe0)
		{
			printf("Not a valid status register value: (top 3 bits should be 1s)\n");
			return;
		}
		if (status & STATUS_ERRLVL_PIN) {
			printf ("    ERRLVL pin is tied HIGH\n");
		}
		else {
			printf ("    ERRLVL pin is tied LOW\n");
		}
		if (status & STATUS_FERR_STS) {
			printf ("    Frame error occurred\n");
		}
		else {
			printf ("    Frame error did not occur\n");
		}
		if (status & SPI_TIMEOUT_ERR) {
			printf ("    Time-out error occurred\n");
		}
		else {
			printf ("    Time-out error did not occur\n");
		}
		if (status & SPI_LOOP_STS) {
			printf ("    A loop error has occured\n");
		}
		else {
			printf ("    A loop error has not occurred\n");
		}
		if (status & SPI_CURR_LOOP_STS) {
			printf ("    A loop error is occurring\n");
		}
		else {
			printf ("    A loop error is not occurring\n");
		}
	}
private:

};

