#ifndef _ALTERA_SPI_DRV_H
#define _ALTERA_SPI_DRV_H

//! This class defines the low level access to a device on the SPI bus.
class AlteraSpiDrv{
public:
	/*! This constructor only takes address information to access the chip and does not do any any initialization of the chip itself
	@param spiController Pointer to the SPI controller in the user address space
	@param cs chip select number to access the desired chip on the SPI bus, since several chips can share the same controller
	 */
	AlteraSpiDrv(UINT32 *spiController, UINT8 cs);

	//! Send a one byte command to the SPI bus with no parameter
	/*!
		@param cmd Command to be sent.
	*/
	void	sendByteCommand(UINT8 cmd);

	//! Send a one byte command to the SPI bus followed by a one-byte parameter
	/*!
		@param cmd Command to be sent.
		@param value Parameter for the command, typically the value to put in a register.
	*/
	void	setRegister(UINT8 cmd, UINT8 value);

	//! Send a one byte command to the SPI bus, and returns a one-byte value returned by the device.
	/*!
		@param cmd Command to be sent.
		@return Typically the value read from a register.
	*/
	UINT8	getRegister(UINT8 cmd);

	//! Send a one byte command to the SPI bus, and returns data with arbitrary length from the device.
	/*!
		@param cmd Command to be sent.
		@param count Size of the receive buffer.
		@param passedArray Where to put the data returned by the device.
		@return the number of bytes actually returned.
	*/
	int	MultiByteRead(UINT8 cmd, UINT8 count, unsigned char *passedArray);

	//! Send a one byte command to the SPI bus, followed by data with arbitrary length to the device.
	/*!
		@param cmd Command to be sent.
		@param count Size of the transmit buffer.
		@param passedArray Transmit buffer address.
	*/
	void	MultiByteWrite(UINT8 cmd, UINT8 count, unsigned char *passedArray);

	//! General purpose sending of a command on the SPI bus to the device
	/*!
		@param write_length size of the MOSI buffer, going from the master to the slave.
		@param write_data A UINT8 pointer, address of the MOSI buffer. The first byte is the command
		@param read_length size of the MISO buffer, going from the slave to the master.
		@param read_data A UINT8 pointer, address of the MOSI buffer.
		@param flags SPI flags, unused.
		@return  length of the read operation.
	*/
	int		spiCommand(UINT32 write_length, const UINT8 * write_data,
			UINT32 read_length, UINT8 * read_data,
			UINT32 flags);

private:
	UINT32 *spiCtrlBase;
	UINT8	spiCtrlDeviceNbr;

};

#endif
