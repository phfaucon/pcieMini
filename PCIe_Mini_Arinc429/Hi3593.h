#ifndef _HI3593_NIOS_H
#define _HI3593_NIOS_H

#include "AlphiDll.h"
#include "AlteraSpiDrv.h"
#include "arinc429error.h"
#include "commandMb.h"

//! This structure contains the information to configure a transmitter channel.
typedef struct {
	UINT8 hiZ;			//!<  Setting this bit puts the on-chip line driver outputs to a high-impedance state.
	UINT8 tFlip;		//!<  Setting this bit reverses the bit order of the first 8 bits of each ARINC 429 message transmitted.
	UINT8 tMode;		/*!<  IfTMODE is "0", data in the transmit FIFO is sent to theARINC 429 bus only upon receipt of a
							transmit enable command. If TMODE is a "1", data is sent as soon as it is available.*/
	UINT8 selftest;		/*!<  Setting SELFTEST causes an internal connection to be made looping-back the transmitter
						  outputs to both receiver inputs for self-test purposes. When in self-test mode, the HI-3593
						  ignores data received on the twoARINC 429 receive channels and holds the on-chip line driver
						  outputs in the NULL state to prevent self-test data being transmitted to other receivers on the bus.
						  oddEven = "1". */
	UINT8 oddEven;		/*!<  If tParity is true, the transmitter inserts an odd parity bit if oddEven = "0", or an even if
						  oddEven = "1".*/
	UINT8 tParity;		/*!<  If tParity = "0", no parity bit is inserted and the 32nd transmitted bit is data. When tParity is
						  "1" a parity bit is substituted for bit 32 according to the oddEven value.*/
	UINT8 rate;			/*!<  If RATE is "0", ARINC 429 high-speed data rate is selected. RATE = "1" selects low-speed
						  ARINC 429 data rate (high-speed / 8).*/
} Tx_configuration;

//! This structure contains the information to configure a receiver channel.
typedef struct {
	// labels
	UINT8 labels[32];	//!<  bit map of which label is allowed
	// priority registers
	UINT8 priLabels[3];	//!< Receiver 1 Priority-Label Match Registers.
	// control register
	UINT8 rFlip;		//!<  Setting this bit reverses the bit order of the first 8 bits of each ARINC 429 message received.
	UINT8 sd9;			/*!<  If the receiver decoder is enable by setting the sdOn bit to a "1", then ARINC 429 message
							bit 9 must match this bit for the message to be accepted.*/
	UINT8 sd10;			/*!<  If the receiver decoder is enable by setting the sdOn bit to a "1", then ARINC 429 message
							bit 10 must match this bit for the message to be accepted.*/
	UINT8 sdOn;       	//!< enable/disable SD-bits filtering
						//!<  If this bit is set, bits 9 and 10 of the received ARINC 429 message must match SD9 and SD10
	UINT8 parity;		//!< enable/disable parity checking
						/*!<  Received word parity checking is enabled when this bit is set. If "0", all 32 bits of the received
							ARINC 429 word are stored without parity checking.*/
	UINT8 labRec;		//!< enable/disable label filtering
						/*!<  When "0", all received messages are stored. If this bit is set, incoming ARINC message label
							filtering is enabled. Only messages whose corresponding label filter table entry is set to a "1"
							will be stored in the Receive FIFO.*/
	UINT8 plOn;			/*!<  Priority-Label Register enable. If PLON = "1" the three Priority-Label Registers are enabled
							and received ARINC 429 messages with labels that match one of the three pre-programmed
							values will be capured and stored in the corresponding Prioty-Label Mail Boxes. If PLON = "0"
							the Priority-Label matching feature is turned off and no words are placed in the mail boxes.*/
	UINT8 rate;			//!<  "0": ARINC 429 high-speed data rate, "1" low-speed ARINC 429 data rate (high-speed / 8).
						/*!<  If RATE is "0", ARINC 429 high-speed data rate is selected. RATE = "1" selects low-speed
						 ARINC 429 data rate (high-speed / 8).*/
} Rx_configuration;

typedef struct _mBox 
{
	volatile UINT32 data;
	volatile UINT32 status;
} mBox;

typedef mBox mBoxDouble[2];


class DLL Hi3593{
public:
	//! Select which ARINC word receive condition will generate an interrupt
	enum rxIntSelect {
		FifoMsg,	//!<  A valid message has been received and placed in the Receiver 1 FIFO or any of the Receiver 1 Priority-Label mail boxes
		MB1Msg,		//!<  A message has been received and placed in Priority-Label mail box #1
		MB2Msg,		//!<  A message has been received and placed in Priority-Label mail box #2
		MB3Msg		//!<  A message has been received and placed in Priority-Label mail box #3
	};

	//! Select which receive FIFO level will generate an interrupt
	enum rxFifoLevel {
		FifoEmpty,		//!<  Flag goes high when FIFO is empty
		FifoFull,		//!<  Flag goes high when FIFO contains 32 ARINC-429 words (FIFO is full)
		FifoHalfFull,	//!<  Flag goes high when FIFO contains at least sixteen ARINC-429 words (FIFO is half-full)
		FifoNotEmpty	//!<  Flag goes high when FIFO is not empty
	};

	/*! This constructor only takes address information to access the chip and does not do any any initialization of the chip itself
	@param spiController Pointer to the SPI controller in the user address space
	@param cs chip select number to access the desired chip on the SPI bus, since several chips can share the same controller
	 */
	Hi3593(UINT8 cs, volatile void *bar2address, volatile void *dpr, CommandMb *mb);

	// receiver only
	//! Read back a receiver configuration
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param[out] config A pointer to a Rx_configuration structure containing the configuration parameters.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status getRxConfig(UINT8 chNbr, Rx_configuration *config);

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
	ARINC_status receiveWord(UINT8 chNbr, UINT32 *w, UINT32 timeout);

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

	// receiver only
	//! Configure a receiver
	/*!
		@param chNbr "0" for receiver 1 and "1" for receiver 2.
		@param config A pointer to a Rx_configuration structure containing the configuration parameters.
		@return  ARINC429_NO_ERROR if the operation was successful.
		@return  ARINC429_INVALID_CHANNEL_NUM if the channel number is invalid.
	*/
	ARINC_status setRxConfig(UINT8 chNbr, Rx_configuration *config);

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

	// status/control registers
	//! Check if the receive FIFO is full 
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  true if the FIFO is full.
	*/
	bool isRxFifoFull(UINT8 chNbr);

	//! Check if the receive FIFO is half full 
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  true if the FIFO is at least half full.
	*/
	bool isRxFifoHalfFull(UINT8 chNbr);

	//! Check if the receive FIFO is empty 
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  true if the FIFO is at empty.
	*/
	bool isRxFifoEmpty(UINT8 chNbr);

	//! Check how many words are in the receive FIFO
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  number of words.
	*/
	UINT32 RxFifoLevel(UINT8 chNbr);

	// transmitter only
	//! Configure a transmit channel 
	/*!
		\param config The structure containing the transmitter configuration information.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setTxConfig(Tx_configuration *config);

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

	//! Check how many words are in the receive FIFO
	/*!
		@param chNbr "0" for receiver 1 and any other value for receiver 2.
		@return  number of words.
	*/
	UINT32 TxFifoLevel();

	//! Program the Int output type and Enable/Disable receiver interrupts
	/*!
			@param channel "0" is channel 1, else channel 2
			@param intType Select the type of word that will generate an interrupt when received
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setRxIntEnable(UINT8 channel, rxIntSelect intType);

	//! Program the Flag output type and Enable/Disable FIFO level interrupts
	/*!
			@param channel "0" is channel 1, else channel 2
			@param flagType Select FIFO level that will generate an interrupt
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setRxFlagEnable(UINT8 channel, rxFifoLevel flagType);

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

	//! Program the interrupt mask
	/*!
			@param mask  16-bit mask indicating which interrupts are enabled
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setIrqMask(UINT16 mask);

	//! Read back the interrupt mask
	/*!
			@param mask  16-bit mask indicating which interrupts are enabled
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status getIrqMask(UINT16 *mask);

	//! Set the FIFO level for the interrupt
	/*!
			@param level  16-bit indicating at what 
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setIrqRxFifoLevel(UINT16 level);

	//! Revalidate the interrupts for the PCI express 
	/*!
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status reenableIrq();
	UINT8 cs;

	// interrupt mask setup

	protected:

#define HI3593_RX_CTRL_RATE			0x01
#define HI3593_RX_CTRL_PLON			0x02
#define HI3593_RX_CTRL_LABREC		0x04
#define HI3593_RX_CTRL_PARITY		0x08
#define HI3593_RX_CTRL_SDON			0x10
#define HI3593_RX_CTRL_SD10			0x20
#define HI3593_RX_CTRL_SD9			0x40
#define HI3593_RX_CTRL_RFLIP		0x80

#define HI3593_RX_STATUS_FFEMPTY	0x01
#define HI3593_RX_STATUS_FFHALF		0x02
#define HI3593_RX_STATUS_FFFULL		0x04
#define HI3593_RX_STATUS_PL1		0x08
#define HI3593_RX_STATUS_PL2		0x10
#define HI3593_RX_STATUS_PL3		0x20

#define HI3593_TX_CTRL_RATE			0x01
#define HI3593_TX_CTRL_TPARITY		0x04
#define HI3593_TX_CTRL_ODDEVEN		0x08
#define HI3593_TX_CTRL_SELFTEST		0x10
#define HI3593_TX_CTRL_TMODE		0x20
#define HI3593_TX_CTRL_TFLIP		0x40
#define HI3593_TX_CTRL_HIZ			0x80

#define HI3593_TX_STATUS_FFEMPTY	0x01
#define HI3593_TX_STATUS_FFHALF		0x02
#define HI3593_TX_STATUS_FFFULL		0x04

private:
	UINT32 getRxMbStatus(UINT8 recvNbr);
	void setRxMbStatus(UINT8 recvNbr, UINT32 val);
	int isRxMbEmpty(UINT8 recvNbr);
	UINT32 getTxMbStatus();
	void setTxMbStatus(UINT32 val);
	int isTxMbEmpty();
	int WriteTxMailbox(int channel, UINT32 val);

	// command FIFO
	int commandFifoReadStatus(UINT32 mask);
	int commandFifoReadLevel();
	ARINC_status commandFifoWrite(UINT32 data);

	volatile UINT32 * bar2;
	volatile UINT32 * dpr;
	volatile mBoxDouble * rxMb;
	volatile mBox * txMb;
	unsigned int rxFifoSize;
	unsigned int txFifoSize;
	CommandMb *cmdMb;
	UINT8 labels[2][32];
#if 0
	// constants
	static const UINT8 HI3593_masterResetCmd;		//!<  Software controlled Master Reset
	static const UINT8 HI3593_writeTxCtrlCmd;		//!<  Write Transmit Control Register
	static const UINT8 HI3593_writeTxMsgCmd;		//!<  Write ARINC 429 message to Transmit FIFO
	static const UINT8 HI3593_writeRx1CtrlCmd;		//!<  Write Receiver 1 Control Register
	static const UINT8 HI3593_writeLblRx1Cmd;		/*!<  Write label values to Receiver 1 label memory. Starting with label 0xFF, consecutively set or reset each
				label in descending order. For example, if the first data byte is programmed to 10110010 then labels FF, FD FC and F9 will be set and FE, FB, FA and F8 will be reset.*/
	static const UINT8 HI3593_writePriRx1Cmd;		/*!<  Write Receiver 1 Priority-Label Match Registers. The data field consists of three eight-bit labels. The first data
				byte is written to P-L filter #3, the second to P-L filter #2, and the last byte to filter #1*/
	static const UINT8 HI3593_writeRx2CtrlCmd;		//!<  Write Receiver 2 Control Register
	static const UINT8 HI3593_writeLblRx2Cmd;		/*!<  Write label values to Receiver 2 label memory. Starting with label 0xFF, consecutively set or reset each
				label in descending order. For example, if the first data byte is programmed to 10110010 then labels FF, FD FC and F9 will be set and FE, FB, FA and F8 will be reset.*/
	static const UINT8 HI3593_writePriRx2Cmd;		/*!<  Write Receiver 2 Priority-Label Match Registers. The data field consists of three eight-bit labels. The first data
				byte is written to P-L filter #3, the second to P-L filter #2, and the last byte to filter #1*/
	static const UINT8 HI3593_writeIrqRegCmd;		//!<  Write Flag / Interrupt Assignment Register
	static const UINT8 HI3593_writeAclkRegCmd;		//!<  Write ACLK Division Register
	static const UINT8 HI3593_transmitCurrCmd;		//!<  Transmit current contents of Transmit FIFO if Transmit Control Register bit 5 (TMODE) is a "0"
	static const UINT8 HI3593_softResetCmd;		//!<  Software Reset. Clears the Transmit and Receive FIFOs and the Priority-Label Registers
	static const UINT8 HI3593_resetLblMemRx1Cmd;		//!<  Set all bits in Receiver 1 label memory to a "1"
	static const UINT8 HI3593_resetLblMemRx2Cmd;		//!<  Set all bits in Receiver 2 label memory to a "1"
	static const UINT8 HI3593_readTxStatusCmd;		//!<  Read Transmit Status Register
	static const UINT8 HI3593_readTxCtrlCmd;		//!<  Read Transmit Control Register
	static const UINT8 HI3593_readRx1StatusCmd;		//!<  Read Receiver 1 Status Register
	static const UINT8 HI3593_readRx1CtrlCmd;		//!<  Read Receiver 1 Control Register
	static const UINT8 HI3593_readRx1LblCmd;		//!<  Read label values from Receiver 1 label memory.
	static const UINT8 HI3593_readRx1PriLblCmd;		//!<  Read Receiver 1 Priority-Label Match Registers.
	static const UINT8 HI3593_readRx1MsgCmd;		//!<  Read one ARINC 429 message from the Receiver 1 FIFO
	static const UINT8 HI3593_readRx1PriReg1Cmd;		//!<  Read Receiver 1 Priority-Label Register #1, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx1PriReg2Cmd;		//!<  Read Receiver 1 Priority-Label Register #2, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx1PriReg3Cmd;		//!<  Read Receiver 1 Priority-Label Register #3, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx2StatusCmd;		//!<  Read Receiver 2 Status Register
	static const UINT8 HI3593_readRx2CtrlCmd;		//!<  Read Receiver 2 Control Register
	static const UINT8 HI3593_readRx2LblCmd;		//!<  Read label values from Receiver 2 label memory
	static const UINT8 HI3593_readRx2PriLblCmd;		//!<  Read Receiver 2 Priority-Label Match Registers.
	static const UINT8 HI3593_readRx2MsgCmd;		//!<  Read one ARINC 429 message from the Receiver 2 FIFO
	static const UINT8 HI3593_readRx2PriReg1Cmd;		//!<  Read Receiver 2 Priority-Label Register #1, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx2PriReg2Cmd;		//!<  Read Receiver 2 Priority-Label Register #2, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx2PriReg3Cmd;		//!<  Read Receiver 2 Priority-Label Register #3, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readFlagRegCmd;		//!<  Read Flag / Interrupt Assignment Register
	static const UINT8 HI3593_readAclkDivRegCmd;		//!<  Read ACLK Division Register
#endif
};

#endif
