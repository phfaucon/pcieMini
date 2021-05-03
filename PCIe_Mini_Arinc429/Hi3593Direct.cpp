#include <windows.h>
#include "Hi3593.h"
#include "Hi3593direct.h"

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


Hi3593direct::Hi3593direct(UINT32 *spiController, UINT8 cs)
{
	dev = new AlteraSpiDrv(spiController, cs);
}

ARINC_status Hi3593direct::setPriLabels(UINT8 chNbr, UINT8 *lblReversed)
{
	if (chNbr<0 || chNbr>1)
	return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_writePriRx2Cmd : HI3593_writePriRx1Cmd;
	dev->MultiByteWrite(cmd, 3, (UINT8 *)lblReversed);
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593direct::getPriLabels(UINT8 chNbr, UINT8 *lblReversed)
{
	if (chNbr<0 || chNbr>1)
	return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_writePriRx2Cmd : HI3593_writePriRx1Cmd;
	dev->MultiByteWrite(cmd, 3, (UINT8 *)lblReversed);
	return ARINC429_NO_ERROR;
}

// receiver only
ARINC_status Hi3593direct::receiveWord(UINT8 chNbr, UINT32 *w)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (isRxFifoEmpty(chNbr))
		return ARINC429_RX_UNDERFLOW;
	UINT8 cmd = chNbr? HI3593_readRx2MsgCmd : HI3593_readRx1MsgCmd;
	dev->MultiByteRead(cmd, 4, (UINT8 *)w);
	return ARINC429_NO_ERROR;
}

// receiver only
ARINC_status Hi3593direct::receiveWord(UINT8 chNbr, UINT32 *w, int timeout)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (isRxFifoEmpty(chNbr))
		return ARINC429_RX_UNDERFLOW;
	UINT8 cmd = chNbr? HI3593_readRx2MsgCmd : HI3593_readRx1MsgCmd;
	dev->MultiByteRead(cmd, 4, (UINT8 *)w);
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593direct::setLabel(UINT8 chNbr, UINT8 labelNbr, UINT8 onOff)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (labelNbr<0 || labelNbr>255)
		return ARINC429_INVALID_LABELNUMBER;
	UINT8 labels[32];
	getLabels(chNbr, labels);
	UINT8 lbl = 1 << (labelNbr % 8);
	if (!onOff) 
		labels[labelNbr/8] &= ~lbl;		// Label filtering disable 
	else
		labels[labelNbr/8] |= lbl;		// Label filtering enable 
	setLabels(chNbr, labels);
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593direct::getLabels(UINT8 chNbr, UINT8 *labels)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (labels == NULL)
		return ARINC429_INVALID_HANDLE;
	UINT8 cmd = chNbr? HI3593_readRx2LblCmd : HI3593_readRx1LblCmd;
	dev->MultiByteRead(cmd, 32, (UINT8 *)labels);
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593direct::setLabels(UINT8 chNbr, UINT8 *labels)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	if (labels == NULL)
		return ARINC429_INVALID_HANDLE;
	UINT8 cmd = chNbr? HI3593_readRx2LblCmd : HI3593_readRx1LblCmd;
	dev->MultiByteWrite(cmd, 32, (UINT8 *)labels);
	return ARINC429_NO_ERROR;
}

bool Hi3593direct::isLabelEnable(UINT8 chNbr, UINT8 labelNbr)
{
	UINT8 labels[32];
	getLabels(chNbr, labels);
	UINT8 lbl = labels[labelNbr/8];
	return ((lbl & (1 << (labelNbr % 8))) != 0);
}

UINT8 Hi3593direct::getRxStatusReg(UINT8 chNbr)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_readRx2StatusCmd : HI3593_readRx1StatusCmd;
	return dev->getRegister(cmd);
}

UINT8 Hi3593direct::getRxCtrlReg(UINT8 chNbr)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_readRx2CtrlCmd : HI3593_readRx1CtrlCmd;
	return dev->getRegister(cmd);
}

ARINC_status Hi3593direct::setRxCtrlReg(UINT8 chNbr, UINT8 value)
{
	if (chNbr<0 || chNbr>1)
		return ARINC429_INVALID_CHANNEL_NUM;
	UINT8 cmd = chNbr? HI3593_writeRx2CtrlCmd : HI3593_writeRx1CtrlCmd;
	dev->setRegister(cmd, value);
	return ARINC429_NO_ERROR;
}

bool Hi3593direct::isRxFifoFull(UINT8 chNbr)
{
	return ((getRxStatusReg(chNbr) & HI3593_RX_STATUS_FFFULL) != 0);
}

bool Hi3593direct::isRxFifoHalfFull(UINT8 chNbr)
{
	return ((getRxStatusReg(chNbr) & HI3593_RX_STATUS_FFHALF) != 0);
}

bool Hi3593direct::isRxFifoEmpty(UINT8 chNbr)
{
	return ((getRxStatusReg(chNbr) & HI3593_RX_STATUS_FFEMPTY) != 0);
}



// transmitter only
ARINC_status Hi3593direct::transmitWord(UINT32 w)
{
	dev->MultiByteWrite(HI3593_writeTxMsgCmd, 4, (UINT8 *)&w);
	return ARINC429_NO_ERROR;
}

// register access
UINT8 Hi3593direct::getTxStatusReg()
{
	return dev->getRegister(HI3593_readTxStatusCmd);
}

UINT8 Hi3593direct::getTxCtrlReg()
{
	return dev->getRegister(HI3593_readTxCtrlCmd);
}

ARINC_status Hi3593direct::setTxCtrlReg(UINT8 value)
{
	dev->setRegister(HI3593_writeTxCtrlCmd, value);
	return ARINC429_NO_ERROR;
}



ARINC_status Hi3593direct::sendCommand(UINT8 cmd, UINT8 *readBuffer, UINT8 *writeBuffer)
{
	UINT8 w_buffer[1000];
	w_buffer[0] = cmd;
	switch (cmd)
	{
	// command, no parameter
	case HI3593_masterResetCmd:
	case HI3593_transmitCurrCmd:
	case HI3593_softResetCmd:
	case HI3593_resetLblMemRx1Cmd:
	case HI3593_resetLblMemRx2Cmd:
		dev->spiCommand(
			1, w_buffer,
			0, NULL,
			0);
		break;
	// command, 1 byte long write parameter
	case HI3593_writeTxCtrlCmd:
	case HI3593_writeRx1CtrlCmd:
	case HI3593_writeRx2CtrlCmd:
	case HI3593_writeIrqRegCmd:
	case HI3593_writeAclkRegCmd:
		if (writeBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		w_buffer[1] = writeBuffer[0];
		dev->spiCommand(
			2, w_buffer,
			0, NULL,
			0);
		break;
	// command, 3 byte long write parameter
	case HI3593_writePriRx1Cmd:
	case HI3593_writePriRx2Cmd:
		if (writeBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		w_buffer[1] = writeBuffer[0];
		for (int i=0; i<3; i++)
			w_buffer[i+1] = writeBuffer[i];
		dev->spiCommand(
			4, w_buffer,
			0, NULL,
			0);
		break;
	// command, 4 byte long write parameter
	case HI3593_writeTxMsgCmd:
		if (writeBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		w_buffer[1] = writeBuffer[0];
		for (int i=0; i<4; i++)
			w_buffer[i+1] = writeBuffer[i];
		dev->spiCommand(
			5, w_buffer,
			0, NULL,
			0);
		break;
	// command, 32 byte long write parameter
	case HI3593_writeLblRx1Cmd:
	case HI3593_writeLblRx2Cmd:
		if (writeBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		w_buffer[1] = writeBuffer[0];
		for (int i=0; i<32; i++)
			w_buffer[i+1] = writeBuffer[i];
		dev->spiCommand(
			33, w_buffer,
			0, NULL,
			0);
		break;
	// command, 1 byte long read parameter
	case HI3593_readTxStatusCmd:
	case HI3593_readTxCtrlCmd:
	case HI3593_readRx1StatusCmd:
	case HI3593_readRx1CtrlCmd:
	case HI3593_readRx2StatusCmd:
	case HI3593_readRx2CtrlCmd:
	case HI3593_readFlagRegCmd:
	case HI3593_readAclkDivRegCmd:
		if (readBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		dev->spiCommand(
			1, w_buffer,
			1, readBuffer,
			0);
		break;
	// command, 3 byte long read parameter
	case HI3593_readRx1PriLblCmd:
	case HI3593_readRx1PriReg1Cmd:
	case HI3593_readRx1PriReg2Cmd:
	case HI3593_readRx1PriReg3Cmd:
	case HI3593_readRx2PriLblCmd:
	case HI3593_readRx2PriReg1Cmd:
	case HI3593_readRx2PriReg2Cmd:
	case HI3593_readRx2PriReg3Cmd:
		if (readBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		dev->spiCommand(
			1, w_buffer,
			3, readBuffer,
			0);
		break;
	// command, 4 byte long read parameter
	case HI3593_readRx1MsgCmd:
	case HI3593_readRx2MsgCmd:
		if (readBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		dev->spiCommand(
			1, w_buffer,
			4, readBuffer,
			0);
		break;
	// command, 32 byte long read parameter
	case HI3593_readRx1LblCmd:
	case HI3593_readRx2LblCmd:
		if (readBuffer == NULL)
			return ARINC429_INVALID_HANDLE;
		dev->spiCommand(
			1, w_buffer,
			32, readBuffer,
			0);
		break;
	default:
		return ARINC429_INVALID_COMMAND;
	}
	return ARINC429_NO_ERROR;
}

bool Hi3593direct::isTxFifoFull()
{
	return ((getTxStatusReg() & HI3593_TX_STATUS_FFFULL) != 0);
}

bool Hi3593direct::isTxFifoHalfFull()
{
	return ((getTxStatusReg() & HI3593_TX_STATUS_FFHALF) != 0);
}

bool Hi3593direct::isTxFifoEmpty()
{
	return ((getTxStatusReg() & HI3593_TX_STATUS_FFEMPTY) != 0);
}

