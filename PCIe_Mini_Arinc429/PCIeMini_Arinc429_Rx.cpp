#include "arinc429error.h"
#include "PCIeMini_Arinc429_Rx.h"

PCIeMini_Arinc429_Rx::PCIeMini_Arinc429_Rx(Hi3593 *controller, Hi3593Status *gpio, int channelNbr)
{
	dev = controller;
	chNbr = channelNbr;
	hi3593Status = gpio;
}

ARINC_status PCIeMini_Arinc429_Rx::setConfig(Rx_configuration *config)
{
	return dev->setRxConfig(chNbr, config);
}

ARINC_status PCIeMini_Arinc429_Rx::getConfig(Rx_configuration *config)
{
	return dev->setRxConfig(chNbr, config);
}

ARINC_status PCIeMini_Arinc429_Rx::receive(UINT32 *w, int nbr_requested, int *nbr_received)
{
	int i;
	ARINC_status st;
	*nbr_received = 0;

	for (i = 0; i<nbr_requested; i++) 
	{
		st = receiveWord(&w[i]);
		if (st != ARINC429_NO_ERROR) {
			if (st == ARINC429_RX_UNDERFLOW)
				return ARINC429_NO_ERROR;
			else 
				return st;
		}
		*nbr_received = i+1;
	}
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429_Rx::receive(UINT32 *w, int nbr_requested, int *nbr_received, int time_out)
{
	int i;
	ARINC_status st;
	*nbr_received = 0;

	for (i = 0; i<nbr_requested; i++) 
	{
		st = receiveWord(&w[i], time_out);
		if (st != ARINC429_NO_ERROR) {
			if (st == ARINC429_RX_UNDERFLOW)
				return ARINC429_NO_ERROR;
			else 
				return st;
		}
		*nbr_received = i+1;
	}
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429_Rx::receiveWord(UINT32 *w)
{
	return dev->receiveWord(chNbr, w);
}

ARINC_status PCIeMini_Arinc429_Rx::receiveWord(UINT32 *w, int timeout)
{
	return dev->receiveWord(chNbr, w, timeout);
}

ARINC_status PCIeMini_Arinc429_Rx::setLabel(UINT8 labelNbr, UINT8 onOff)
{
	return dev->setLabel(chNbr, labelNbr, onOff);
}

ARINC_status PCIeMini_Arinc429_Rx::setLabels(UINT8 labelNbr, UINT8 *onOff)
{
	return dev->setLabels(chNbr, onOff);
}


bool PCIeMini_Arinc429_Rx::isFifoFull()
{
	return dev->isRxFifoFull(chNbr);
}

bool PCIeMini_Arinc429_Rx::isFifoHalfFull()
{
	return dev->isRxFifoHalfFull(chNbr);
}

bool PCIeMini_Arinc429_Rx::isFifoEmpty()
{
	return dev->isRxFifoEmpty(chNbr);
}

// register access
UINT8 PCIeMini_Arinc429_Rx::getStatusReg()
{
	return dev->getRxStatusReg(chNbr);
}

UINT8 PCIeMini_Arinc429_Rx::getCtrlReg()
{
	return dev->getRxCtrlReg(chNbr);
}


void PCIeMini_Arinc429_Rx::setCtrlReg(UINT8 value)
{
	dev->setRxCtrlReg(chNbr, value);
}

bool PCIeMini_Arinc429_Rx::checkPriMb(UINT8 nbr)
{
	return hi3593Status->checkPriMb(chNbr, nbr);
}

bool PCIeMini_Arinc429_Rx::checkRxFlag()
{
	return hi3593Status->checkRxFlag(chNbr);
}

bool PCIeMini_Arinc429_Rx::checkRxInt()
{
	return hi3593Status->checkRxInt(chNbr);
}

ARINC_status PCIeMini_Arinc429_Rx::setRxIntEnable(UINT8 mask)
{
	ARINC_status st;
	if ((mask & 0x1)!= 0) {
		st = dev->setRxIntEnable(chNbr, Hi3593::FifoMsg);
		if (st != ARINC429_NO_ERROR)
			return st;
		// update the pio register
		st = hi3593Status->setRxIntEnable(chNbr, true);
		if (st != ARINC429_NO_ERROR)
			return st;
	}
	else {
		st = hi3593Status->setRxIntEnable(chNbr, false);
		if (st != ARINC429_NO_ERROR)
			return st;
	}
	hi3593Status->setRxMbIrqEnable(chNbr, mask>>1);
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429_Rx::setRxFlagEnable(Hi3593::rxFifoLevel flagType, bool irqEnabled)
{
	ARINC_status st;
	st = dev->setRxFlagEnable(chNbr, flagType);
	if (st != ARINC429_NO_ERROR)
		return st;
	// update the pio register
	return hi3593Status->setRxFlagEnable(chNbr, irqEnabled);
}

