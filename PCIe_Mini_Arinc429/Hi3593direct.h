#include "Windows.h" 
#include "Hi3593.h" 

class Hi3593direct{
public:
	/*! This constructor only takes address information to access the chip and does not do any any initialization of the chip itself
	@param spiController Pointer to the SPI controller in the user address space
	@param cs chip select number to access the desired chip on the SPI bus, since several chips can share the same controller
	 */
	Hi3593direct(UINT32 *spiController, UINT8 cs);

	//! Send an SPI command to the HI-3593
	/*!
	The size of the data transfered is specified by the command.
		@param	cmd A supported SPI command
		@param	readBuffer The read buffer for the data received from the device
		@param	writeBuffer The source buffer for what to send to the device
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_COMMAND if the command number is invalid.
	*/
	ARINC_status sendCommand(UINT8 cmd, UINT8 *readBuffer, UINT8 *writeBuffer);

	//! Get a word from the FIFO
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param[out] w A pointer to where to return the word.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_RX_UNDERFLOW if there was no valid data in the receive FIFO.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status receiveWord(UINT8 chNbr, UINT32 *w);

	//! Get a word from the FIFO with timeout
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param[out] w A pointer to where to return the word.
		@param timeout How much we should wait for a word.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_RX_UNDERFLOW if there was no valid data in the receive FIFO.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status receiveWord(UINT8 chNbr, UINT32 *w, int timeout);

	//! Set or reset filtering on one label
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param labelNbr A label number in the 0-255 range.
		@param onOff "0" not to receive a message with the selected value, else allows receiving it.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_LABELNUMBER if the label number is invalid.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status setLabel(UINT8 chNbr, UINT8 labelNbr, UINT8 onOff);

	//! Retrieve the label filter values
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param labels A pointer to a 32-byte area where the data will be copied. Each bit corresponds to 1 label.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status getLabels(UINT8 chNbr, UINT8 *labels);


	//! Retrieve the label filter values
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param[out] labels A pointer to a 32-byte area from where the data will be read. Each bit corresponds to 1 label.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status setLabels(UINT8 chNbr, UINT8 *labels);

	//! Retrieve the label filter values
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param labelNbr The number of the label to check.
		@return  true if there is the label number is valid and enabled.
	*/
	bool isLabelEnable(UINT8 chNbr, UINT8 labelNbr);

	//! Read a receiver status register
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  the value read from the control register.
	*/
	UINT8 getRxStatusReg(UINT8 chNbr);

	//! Read a receiver control register
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  the value read from the control register.
	*/
	UINT8 getRxCtrlReg(UINT8 chNbr);

	//! Set a receiver control register
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@param  value the value to write in the control register.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status setRxCtrlReg(UINT8 chNbr, UINT8 value);

	//! Set the priority label registers
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@param lblReversed A pointer to a 3 element byte array to write in the priority label registers.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status setPriLabels(UINT8 chNbr, UINT8 *lblReversed);

	//! Readback the priority label registers
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@param lblReversed A pointer to a 3 element byte array that will be overwritten with the priority label registers content.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status getPriLabels(UINT8 chNbr, UINT8 *lblReversed);

	//! Check if the receive FIFO is full 
	/*!
		@return  true if the FIFO is full.
	*/
	bool isRxFifoFull(UINT8 chNbr);

	//! Check if the receive FIFO is half full 
	/*!
		@return  true if the FIFO is at least half full.
	*/
	bool isRxFifoHalfFull(UINT8 chNbr);

	//! Check if the receive FIFO is empty 
	/*!
		@return  true if the FIFO is at empty.
	*/
	bool isRxFifoEmpty(UINT8 chNbr);


	// transmitter only

	//! Read back the configuration of a transmit channel 
	/*!
		\param config A pointer to where the structure will be stored.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status getTxConfig(Tx_configuration *config);

	//! Send an ARINC word to the transmit FIFO 
	/*!
		\param w A pointer from where the ARINC429 word will be retrieved.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status transmitWord(UINT32 w);

	//! Get the status register of a transmit channel 
	/*!
		\return  Returns the 8-bit status register value.
	*/
	UINT8 getTxStatusReg();

	//! Read the transmitter control register
	/*!
		@return  the value read from the control register.
	*/
	UINT8 getTxCtrlReg();

	//! Set the transmitter control register
	/*!
		@param  value the value to write in the control register.
		@return  ARINC429_NO_ERROR if the operation was successful.
	*/
	ARINC_status setTxCtrlReg(UINT8 value);

	// status/control registers
	//! Check if the transmit FIFO is full 
	/*!
		@return  true if the FIFO is full.
	*/
	bool isTxFifoFull();

	//! Check if the transmit FIFO is half full 
	/*!
		@return  true if the FIFO is at least half full.
	*/
	bool isTxFifoHalfFull();

	//! Check if the transmit FIFO is empty 
	/*!
		@return  true if the FIFO is at empty.
	*/
	bool isTxFifoEmpty();	


private:
	AlteraSpiDrv	*dev;

};