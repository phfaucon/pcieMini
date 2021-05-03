#include "PCIeMini_Arinc429_Tx.h"
#include "Hi3593.h"

PCIeMini_Arinc429_Tx::PCIeMini_Arinc429_Tx(Hi3593 *controller)
{
	dev = controller;
}

ARINC_status PCIeMini_Arinc429_Tx::setConfig(Tx_configuration *config)
{
	return dev->setTxConfig(config);
}

ARINC_status PCIeMini_Arinc429_Tx::transmit(UINT32 *w, int nbr_requested, int *nbr_transmitted)
{
	int i;
	ARINC_status st;
	*nbr_transmitted = 0;

	for (i = 0; i<nbr_requested; i++) 
	{
		st = transmitWord(w[i]);
		if (st != ARINC429_NO_ERROR)
			return st;
		*nbr_transmitted = i+1;
	}
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429_Tx::getConfig(Tx_configuration *config)
{
	return dev->getTxConfig(config);
}

ARINC_status PCIeMini_Arinc429_Tx::transmitWord(UINT32 w)
{
	return dev->transmitWord(w);
}

bool PCIeMini_Arinc429_Tx::isFifoFull()
{
	return dev->isTxFifoFull();
}
bool PCIeMini_Arinc429_Tx::isFifoHalfFull()
{
	return dev->isTxFifoHalfFull();
}
bool PCIeMini_Arinc429_Tx::isFifoEmpty()
{
	return dev->isTxFifoEmpty();
}	

// register access
UINT8 PCIeMini_Arinc429_Tx::getStatusReg()
{
	return dev->getTxStatusReg();
}
UINT8 PCIeMini_Arinc429_Tx::getCtrlReg()
{
	return dev->getTxCtrlReg();
}
void PCIeMini_Arinc429_Tx::setCtrlReg(UINT8 value)
{
	dev->setTxCtrlReg(value);
}
