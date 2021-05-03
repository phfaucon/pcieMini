#include <string>
#include <iostream>
#include <time.h>
#include "Hi3593.h"

/* command mailbox 
*/
#define SPI_IDLE 			0x00
#define SPI_BRD_RESET		0x01
#define SPI_WRITE_1 		0x10
#define SPI_WRITE_0 		0x11
#define SPI_READ_1	 		0x12
#define SPI_STORE_LBL_0 	0x13
#define SPI_STORE_LBL_1 	0x14
#define SPI_WRITE_LBL_0 	0x15
#define SPI_WRITE_LBL_1 	0x16
#define SPI_READ_LBL_0		0x17
#define SPI_READ_LBL_1		0x18
#define SPI_WRITE_PRI_LBL	0x19
#define SPI_READ_PRI_LBL	0x1a
#define SPI_READ_RXFIFO_LVL	0x1b
#define SPI_READ_TXFIFO_LVL	0x1c
#define SPI_READ_RXFIFO_SZ	0x1d
#define SPI_READ_TXFIFO_SZ	0x1e

// IRQs
#define IRQ_WRITE_MASK	0x20
#define IRQ_READ_MASK	0x21
#define IRQ_SET_RX_LEVEL	0x24
#define IRQ_REENABLE_IRQ	0x25


// constants
	static const UINT8 HI3593_masterResetCmd	= 0x04;		//!<  Software controlled Master Reset
	static const UINT8 HI3593_writeTxCtrlCmd	= 0x08;		//!<  Write Transmit Control Register
	static const UINT8 HI3593_writeTxMsgCmd		= 0x0c;		//!<  Write ARINC 429 message to Transmit FIFO
	static const UINT8 HI3593_writeRx1CtrlCmd	= 0x10;		//!<  Write Receiver 1 Control Register
	static const UINT8 HI3593_writeLblRx1Cmd	= 0x14;		/*!<  Write label values to Receiver 1 label memory. Starting with label 0xFF, consecutively set or reset each
				label in descending order. For example, if the first data byte is programmed to 10110010 then labels FF, FD FC and F9 will be set and FE, FB, FA and F8 will be reset.*/
	static const UINT8 HI3593_writePriRx1Cmd	= 0x18;		/*!<  Write Receiver 1 Priority-Label Match Registers. The data field consists of three eight-bit labels. The first data
				byte is written to P-L filter #3, the second to P-L filter #2, and the last byte to filter #1*/
	static const UINT8 HI3593_writeRx2CtrlCmd	= 0x24;		//!<  Write Receiver 2 Control Register
	static const UINT8 HI3593_writeLblRx2Cmd	= 0x28;		/*!<  Write label values to Receiver 2 label memory. Starting with label 0xFF, consecutively set or reset each
				label in descending order. For example, if the first data byte is programmed to 10110010 then labels FF, FD FC and F9 will be set and FE, FB, FA and F8 will be reset.*/
	static const UINT8 HI3593_writePriRx2Cmd	= 0x2c;		/*!<  Write Receiver 2 Priority-Label Match Registers. The data field consists of three eight-bit labels. The first data
				byte is written to P-L filter #3, the second to P-L filter #2, and the last byte to filter #1*/
	static const UINT8 HI3593_writeIrqRegCmd	= 0x34;		//!<  Write Flag / Interrupt Assignment Register
	static const UINT8 HI3593_writeAclkRegCmd	= 0x38;		//!<  Write ACLK Division Register
	static const UINT8 HI3593_transmitCurrCmd	= 0x40;		//!<  Transmit current contents of Transmit FIFO if Transmit Control Register bit 5 (TMODE) is a "0"
	static const UINT8 HI3593_softResetCmd		= 0x44;		//!<  Software Reset. Clears the Transmit and Receive FIFOs and the Priority-Label Registers
	static const UINT8 HI3593_resetLblMemRx1Cmd = 0x48;		//!<  Set all bits in Receiver 1 label memory to a "1"
	static const UINT8 HI3593_resetLblMemRx2Cmd = 0x4c;		//!<  Set all bits in Receiver 2 label memory to a "1"
	static const UINT8 HI3593_readTxStatusCmd	= 0x80;		//!<  Read Transmit Status Register
	static const UINT8 HI3593_readTxCtrlCmd		= 0x84;		//!<  Read Transmit Control Register
	static const UINT8 HI3593_readRx1StatusCmd	= 0x90;		//!<  Read Receiver 1 Status Register
	static const UINT8 HI3593_readRx1CtrlCmd	= 0x94;		//!<  Read Receiver 1 Control Register
	static const UINT8 HI3593_readRx1LblCmd		= 0x98;		//!<  Read label values from Receiver 1 label memory.
	static const UINT8 HI3593_readRx1PriLblCmd	= 0x9c;		//!<  Read Receiver 1 Priority-Label Match Registers.
	static const UINT8 HI3593_readRx1MsgCmd		= 0xa0;		//!<  Read one ARINC 429 message from the Receiver 1 FIFO
	static const UINT8 HI3593_readRx1PriReg1Cmd	= 0xa4;		//!<  Read Receiver 1 Priority-Label Register #1, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx1PriReg2Cmd	= 0xa8;		//!<  Read Receiver 1 Priority-Label Register #2, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx1PriReg3Cmd	= 0xac;		//!<  Read Receiver 1 Priority-Label Register #3, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx2StatusCmd	= 0xb0;		//!<  Read Receiver 2 Status Register
	static const UINT8 HI3593_readRx2CtrlCmd	= 0xb4;		//!<  Read Receiver 2 Control Register
	static const UINT8 HI3593_readRx2LblCmd		= 0xb8;		//!<  Read label values from Receiver 2 label memory
	static const UINT8 HI3593_readRx2PriLblCmd	= 0xbc;		//!<  Read Receiver 2 Priority-Label Match Registers.
	static const UINT8 HI3593_readRx2MsgCmd		= 0xc0;		//!<  Read one ARINC 429 message from the Receiver 2 FIFO
	static const UINT8 HI3593_readRx2PriReg1Cmd	= 0xc4;		//!<  Read Receiver 2 Priority-Label Register #1, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx2PriReg2Cmd	= 0xc8;		//!<  Read Receiver 2 Priority-Label Register #2, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readRx2PriReg3Cmd	= 0xcc;		//!<  Read Receiver 2 Priority-Label Register #3, ARINC429 bytes 2,3 & 4 (bits 9 - 32)
	static const UINT8 HI3593_readFlagRegCmd	= 0xd0;		//!<  Read Flag / Interrupt Assignment Register
	static const UINT8 HI3593_readAclkDivRegCmd	= 0xd4;		//!<  Read ACLK Division Register

Hi3593::Hi3593(UINT8 cs, volatile void *bar2address, volatile void *dpram, CommandMb *mb)
{
	// This is called when the board is opened so all the resources are valid
	dpr = (UINT32 *)dpram;
	bar2 = (UINT32*)bar2address;
	this->cs = cs;
	cmdMb = mb;

	// initialize the transmitter mailbox
	txMb = (volatile mBox *)(dpr + 0x20);
	rxMb = (volatile mBoxDouble *)(dpr);
	txMb->status = 0;
	txMb->data = 0xdeadbeef;
	rxMb[0]->status = 0;
	rxMb[1]->status = 0;

	// initialize FIFO
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_RXFIFO_SZ, 0, 0, 0, 0));
	rxFifoSize = cmdMb->getData();
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_TXFIFO_SZ, 0, 0, 0, 0));
	txFifoSize = cmdMb->getData();

	// initialize the NIOS structures
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_BRD_RESET, 0, 0, 0, 0));

}

/// mailboxes
#define MB_DATA_READBACK (0x30)			//! offset of the area dedicated to allow the host to read back Hi3593 data

// DPR_BASE
UINT32 Hi3593::getRxMbStatus(UINT8 recvNbr)
{
	if (recvNbr > 1)
		return 0;
	return rxMb[cs][recvNbr].status;
}

void Hi3593::setRxMbStatus(UINT8 recvNbr, UINT32 val)
{
	if (recvNbr > 1)
		return;
	rxMb[cs][recvNbr].status = val;
}

int Hi3593::isRxMbEmpty(UINT8 recvNbr) {
	UINT32 stat = getRxMbStatus(recvNbr);
	return (stat == 0);
}

ARINC_status Hi3593::receiveWord(UINT8 chNbr, UINT32 *w)
{
	if (rxMb[cs][chNbr].status != 0) {
		*w = rxMb[cs][chNbr].data;
		rxMb[cs][chNbr].status = 0;
		return ARINC429_SUCCESS;
	}
	else return ARINC429_RX_UNDERFLOW;
}


ARINC_status Hi3593::receiveWord(UINT8 chNbr, UINT32 *w, UINT32 timeout)
{
	struct timespec t1;

	if(timeout == 0) return receiveWord(chNbr, w);
	if (rxMb[cs][chNbr].status != 0) {
		*w = rxMb[cs][chNbr].data;
		rxMb[cs][chNbr].status = 0;
		return ARINC429_SUCCESS;
	}
	t1.tv_nsec = 1000000;
	t1.tv_sec = 0;
	DWORD timeoutMs = GetTickCount() + timeout;
	while ((rxMb[cs][chNbr].status == 0) && GetTickCount() <= timeoutMs)
		Sleep(1);
	if (rxMb[cs][chNbr].status != 0) {
		*w = rxMb[cs][chNbr].data;
		rxMb[cs][chNbr].status = 0;
		return ARINC429_SUCCESS;
	}
	else return ARINC429_RX_UNDERFLOW;
}

UINT32 Hi3593::getTxMbStatus()
{
	return txMb[cs].status;
}

void Hi3593::setTxMbStatus(UINT32 val)
{
	txMb[cs].status = val;
}

int Hi3593::isTxMbEmpty() {
	return (txMb[cs].status == 0);
}

/*ARINC_status  Hi3593::transmitWord(UINT32 val)
{
	if (txMb->status == 0) {
		txMb->data = val;
		txMb->status == 1;
		return ARINC429_SUCCESS;
	}
	else
		return ARINC429_TX_OVERFLOW;
}*/

ARINC_status  Hi3593::transmitWord(UINT32 val)
{
	if (txMb[cs].status == 0) {
		txMb[cs].data = val;
		txMb[cs].status = 1;
		return ARINC429_SUCCESS;
	}
	int timeout = 1;
	DWORD timeoutMs = GetTickCount() + timeout + 100;
	while ((txMb[cs].status != 0) && GetTickCount() <= timeoutMs);
	if (txMb[cs].status == 0) {
		txMb[cs].data = val;
		txMb[cs].status = 1;
		return ARINC429_SUCCESS;
	}
	else
		return ARINC429_TX_OVERFLOW;
}


#define CHANNEL_0			(0 << 16)
#define SUBCHANNEL_0		(0 << 20)
#define SUBCHANNEL_1		(1 << 20)

UINT8 Hi3593::getRxStatusReg(UINT8 chNbr)
{
	if (chNbr<0 || chNbr>1)
		return 0;
	UINT8 cmd = chNbr? HI3593_readRx2StatusCmd : HI3593_readRx1StatusCmd;
	ARINC_status st = cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, chNbr, cmd, 0));
	if (st != ARINC429_SUCCESS) cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, chNbr, cmd, 0));
	return cmdMb->getData() & 0xff;
}

UINT8 Hi3593::getRxCtrlReg(UINT8 chNbr)
{
	if (chNbr<0 || chNbr>1)
		return 0;
	UINT8 cmd = chNbr? HI3593_readRx2CtrlCmd : HI3593_readRx1CtrlCmd;
	ARINC_status st = cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, chNbr, cmd, 0));
	if (st != ARINC429_SUCCESS) cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, chNbr, cmd, 0));
//	std::cout << "getRxCtrlReg cmd=" << std::hex << (UINT32)cmd << " value=" << (cmdMb->data & 0xff) << "\n";
	return cmdMb->getData() & 0xff;
}

UINT8 Hi3593::getTxStatusReg()
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, 0, HI3593_readTxStatusCmd, 0));
	return cmdMb->getData() & 0xff;
}

UINT8 Hi3593::getTxCtrlReg()
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, 0, HI3593_readTxCtrlCmd, 0));
	return cmdMb->getData() & 0xff;
}

ARINC_status Hi3593::setRxCtrlReg(UINT8 chNbr, UINT8 value)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_writeRx2CtrlCmd : HI3593_writeRx1CtrlCmd;
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_1, cs, chNbr, cmd, value));
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::setTxCtrlReg(UINT8 value)
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_1, cs, 0, HI3593_writeTxCtrlCmd, value));
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::setLabel(UINT8 chNbr, UINT8 labelNbr, UINT8 onOff)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (labelNbr<0 || labelNbr>255)
		return ARINC429_INVALID_LABELNUMBER;
	UINT8 lbl = 1 << (labelNbr % 8);
	if (!onOff) 
		labels[chNbr][labelNbr/8] &= ~lbl;		// Label filtering disable 
	else
		labels[chNbr][labelNbr/8] |= lbl;		// Label filtering enable 
	setLabels(chNbr, labels[chNbr]);
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::getLabels(UINT8 chNbr, UINT8 *lbl)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (lbl == NULL)
		return ARINC429_INVALID_HANDLE;
	for (int i= 0; i<32; i++)
		lbl[i] = labels[chNbr][i];
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::setLabels(UINT8 chNbr, UINT8 *lbl)
{
	UINT32 commandWord;

	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (labels == NULL)
		return ARINC429_INVALID_HANDLE;
	for (int i= 0; i<32; i++)
		labels[chNbr][i] = lbl[i];
	// write to the board, 2 bytes at a time
	// send to the HI3593
	for (int i = 0; i<16; i++) {			// we are always sending for channel 0 receiver 1
		commandWord = ((SPI_STORE_LBL_0)<<24) ;
		commandWord |= i <<20;
		commandWord |= lbl[i*2 + 1] << 8;
		commandWord |= lbl[i*2];
		cmdMb->commandMbSendCommand(commandWord);
	}
	return cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_LBL_0 + chNbr, cs, chNbr, 0, 0));
}

bool Hi3593::isLabelEnable(UINT8 chNbr, UINT8 labelNbr)
{
	UINT8 labels[32];
	getLabels(chNbr, labels);
	UINT8 lbl = labels[labelNbr/8];
	return ((lbl & (1 << (labelNbr % 8))) != 0);
}

ARINC_status Hi3593::setPriLabels(UINT8 chNbr, UINT8 *lblReversed)
{
	if (chNbr<0 || chNbr>1)
	return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_writePriRx2Cmd : HI3593_writePriRx1Cmd;
	cmdMb->setData( lblReversed[0]<<16 | lblReversed[1]<<8 | lblReversed[2]);
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_PRI_LBL, cs, chNbr, cmd, 0));
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::getPriLabels(UINT8 chNbr, UINT8 *lblReversed)
{
	if (chNbr<0 || chNbr>1)
	return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_readRx2PriLblCmd : HI3593_readRx1PriLblCmd;
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_PRI_LBL, cs, chNbr, cmd, 0));
	lblReversed[0] = (cmdMb->getData()>> 16) & 0xff;
	lblReversed[1] = (cmdMb->getData()>> 8) & 0xff;
	lblReversed[2] = cmdMb->getData() & 0xff;
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::setRxIntEnable(UINT8 channel, rxIntSelect intType)
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, 0, HI3593_readFlagRegCmd, 0));
	UINT8 irqReg = cmdMb->getData() & 0xff;			// read the flag/interrupt assignment register 
	if (channel == 0) {
		irqReg &= 0xf3;			// mask the correct bits
		irqReg |= intType << 2;	// insert the new programing
	}
	else {
		irqReg &= 0x3f;			// mask the correct bits
		irqReg |= intType << 6;	// insert the new programing
	}
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_1, cs, 0, HI3593_writeIrqRegCmd, irqReg));	// write it back
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593::setRxFlagEnable(UINT8 channel, rxFifoLevel flagType)
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_1, cs, 0, HI3593_readFlagRegCmd, 0));
	UINT8 irqReg = cmdMb->getData() & 0xff;			// read the flag/interrupt assignment register 
	if (channel == 0) {
		irqReg &= 0xfc;			// mask the correct bits
		irqReg |= flagType;		// insert the new programing
	}
	else {
		irqReg &= 0xcf;			// mask the correct bits
		irqReg |= flagType << 4;	// insert the new programing
	}
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_1, cs, 0, HI3593_writeIrqRegCmd, irqReg));	// write it back
	return ARINC429_NO_ERROR;
}

bool Hi3593::isRxFifoFull(UINT8 chNbr)
{
	return (RxFifoLevel(chNbr) >= rxFifoSize);
}

bool Hi3593::isRxFifoHalfFull(UINT8 chNbr)
{
	return (RxFifoLevel(chNbr) >= rxFifoSize / 2);
}

bool Hi3593::isRxFifoEmpty(UINT8 chNbr)
{
	return (RxFifoLevel(chNbr) == 0);
}

UINT32 Hi3593::RxFifoLevel(UINT8 chNbr)
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_RXFIFO_LVL, cs, chNbr, 0, 0));
//	std::cout << "chip:" << (int)cs << " rx:" << (int)chNbr << " level:" << cmdMb->data << " \n";
	return cmdMb->getData();
}

bool Hi3593::isTxFifoFull()
{
	return (TxFifoLevel() >= txFifoSize);
}

bool Hi3593::isTxFifoHalfFull()
{
	return (TxFifoLevel() >= txFifoSize / 2);
}

bool Hi3593::isTxFifoEmpty()
{
	return (TxFifoLevel() == 0);
}

UINT32 Hi3593::TxFifoLevel()
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_TXFIFO_LVL, cs, 0, 0, 0));
//	std::cout << "chip:" << (int)cs << " tx FIFO level:" << cmdMb->data << " \n";
	return cmdMb->getData();
}


ARINC_status Hi3593::sendCommand(UINT8 cmd, UINT8 *readBuffer, UINT8 *writeBuffer)
{
	switch (cmd)
	{
	// command, no parameter
	case HI3593_masterResetCmd:
	case HI3593_transmitCurrCmd:
	case HI3593_softResetCmd:
	case HI3593_resetLblMemRx1Cmd:
	case HI3593_resetLblMemRx2Cmd:
		cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_0, cs, 0, cmd, 0));
		break;
	// command, 1 byte long write parameter
	case HI3593_writeTxCtrlCmd:
	case HI3593_writeRx1CtrlCmd:
	case HI3593_writeRx2CtrlCmd:
	case HI3593_writeIrqRegCmd:
	case HI3593_writeAclkRegCmd:
		if (writeBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_WRITE_1, cs, 0, cmd, writeBuffer[0]));
		break;

	default:
		return ARINC429_INVALID_COMMAND;
	}
	return ARINC429_NO_ERROR;
}

	ARINC_status Hi3593::setIrqMask(UINT16 mask)
	{
		cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(IRQ_WRITE_MASK, cs, 0, mask));
		return ARINC429_NO_ERROR;
	}

	//! Read back the interrupt mask
	/*!
			@param mask  16-bit mask indicating which interrupts are enabled
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status Hi3593::getIrqMask(UINT16 *mask)
	{
		cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(IRQ_READ_MASK, cs, 0, 0));
		*mask = cmdMb->getData();
		return ARINC429_NO_ERROR;
	}

	//! Set the FIFO level for the interrupt
	/*!
			@param level  16-bit indicating at what 
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status Hi3593::setIrqRxFifoLevel(UINT16 level)
	{
		cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(IRQ_SET_RX_LEVEL, cs, 0, level));
		return ARINC429_NO_ERROR;
	}

	//! Revalidate the interrupts for the PCI express 
	/*!
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status Hi3593::reenableIrq()
	{
		cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(IRQ_REENABLE_IRQ, cs, 0, 0));
		return ARINC429_NO_ERROR;
	}

	ARINC_status Hi3593::setRxConfig(UINT8 chNbr, Rx_configuration *config)
	{
		ARINC_status st;

		// labels
		st = setLabels(chNbr, config->labels);
		if (st != ARINC429_NO_ERROR) return ARINC429_NO_ERROR;

		// control
		UINT8 ctrl = 0;
		ctrl |= config->rFlip ? HI3593_RX_CTRL_RFLIP : 0;
		ctrl |= config->sd9 ? HI3593_RX_CTRL_SD9 : 0;
		ctrl |= config->sd10 ? HI3593_RX_CTRL_SD10 : 0;
		ctrl |= config->sdOn ? HI3593_RX_CTRL_SDON : 0;
		ctrl |= config->labRec ? HI3593_RX_CTRL_LABREC : 0;
		ctrl |= config->plOn ? HI3593_RX_CTRL_PLON : 0;
		ctrl |= config->rate ? HI3593_RX_CTRL_RATE : 0;
		st = setRxCtrlReg(chNbr, ctrl);
		if (st != ARINC429_NO_ERROR) return ARINC429_NO_ERROR;

		// priority labels
		UINT8 lblReversed[3];
		lblReversed[2] = config->priLabels[0];
		lblReversed[1] = config->priLabels[1];
		lblReversed[0] = config->priLabels[2];
		return setPriLabels(chNbr, lblReversed);
	}

	ARINC_status Hi3593::setTxConfig(Tx_configuration *config)
	{
		// control
		UINT8 ctrl = 0;
		ctrl |= config->hiZ ? HI3593_TX_CTRL_HIZ : 0;
		ctrl |= config->tFlip ? HI3593_TX_CTRL_TFLIP : 0;
		ctrl |= config->tMode ? HI3593_TX_CTRL_TMODE : 0;
		ctrl |= config->selftest ? HI3593_TX_CTRL_SELFTEST : 0;
		ctrl |= config->oddEven ? HI3593_TX_CTRL_ODDEVEN : 0;
		ctrl |= config->tParity ? HI3593_TX_CTRL_TPARITY : 0;
		ctrl |= config->rate ? HI3593_TX_CTRL_RATE : 0;
		return setTxCtrlReg(ctrl);
	}

	ARINC_status Hi3593::getTxConfig(Tx_configuration *config)
	{
		// control
		UINT8 ctrl = getTxCtrlReg();
		config->hiZ = ((ctrl & HI3593_TX_CTRL_HIZ) != 0);
		config->tFlip = ((ctrl & HI3593_TX_CTRL_TFLIP) != 0);
		config->tMode = ((ctrl & HI3593_TX_CTRL_TMODE) != 0);
		config->selftest = ((ctrl & HI3593_TX_CTRL_SELFTEST) != 0);
		config->oddEven = ((ctrl & HI3593_TX_CTRL_ODDEVEN) != 0);
		config->tParity = ((ctrl & HI3593_TX_CTRL_TPARITY) != 0);
		config->rate = ((ctrl & HI3593_TX_CTRL_RATE) != 0);

		return ARINC429_NO_ERROR;
	}

