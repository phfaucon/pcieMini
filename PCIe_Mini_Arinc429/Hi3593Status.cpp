
#include "parallelInput.h"
#include "Hi3593Status.h"


Hi3593Status::Hi3593Status(volatile void *gpio) : ParallelInput(gpio)
{
	ParallelInput::reset();
}


Hi3593Status::~Hi3593Status(void)
{
	ParallelInput::reset();
}

bool Hi3593Status::checkPriMb(UINT8 channel, UINT8 nbr)
{
	if (channel>1 || nbr>3) 
		return false;
	UINT32 stat = getData();
	return ((stat & ( 1<<(channel*3 + nbr))) != 0);
}

bool Hi3593Status::checkRxFlag(UINT8 channel)
{
	if (channel>1) 
		return false;
	UINT32 stat = getData();
	return ((stat & ( 1<<(7 + channel*2))) != 0);
}

bool Hi3593Status::checkRxInt(UINT8 channel)
{
	if (channel>1) 
		return false;
	UINT32 stat = getData();
	return ((stat & ( 1<<(6 + channel*2))) != 0);
}

ARINC_status Hi3593Status::setRxIntEnable(UINT8 channel, bool irqEnable)
{
	// update the pio
	UINT32 data = 0;
	if (channel == 0)
		data |= 1 << 6;
	else
		data |= 1 << 8;
	if (irqEnable) {
		setIrqEnable(data);
	}
	else {
		setIrqDisable(data);
	}
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593Status::setRxFlagEnable(UINT8 channel, bool irqEnable)
{
	// update the pio
	UINT32 data = 0;
	if (channel == 0)
		data |= 1 << 7;
	else
		data |= 1 << 9;
	if (irqEnable) {
		setIrqEnable(data);
	}
	else {
		setIrqDisable(data);
	}
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593Status::setRxMbIrqEnable(UINT8 channel, UINT8 mbMask)
{
	// Let's check if there is anything in the mask that shouldn't be there
	if ((mbMask & ~0x7) != 0)
		return ARINC429_INVALID_VALUE;

	UINT32 data = getIrqEnable();
	if (channel == 0) {
		data &= 0x7;
		data |= mbMask;
	}
	else {
		data &= 0x7 << 3;
		data |= mbMask << 3;
	}
	// update the pio
	base[irqEnable_index] = data;
	return ARINC429_NO_ERROR;
}

ARINC_status Hi3593Status::setTxIntEnable(UINT8 fifoLevel, bool irqEnable)
{
	// update the pio
	UINT32 data = 0;
	data &= ~(3 << 10);				// mask the Tx FIFO bits
	if (irqEnable) {
		data |= fifoLevel << 10;	// if interrupts are enabled, set the relevant bits
	}
	if (irqEnable) {
		setIrqEnable(data);
	}
	else {
		setIrqDisable(data);
	}
	return ARINC429_NO_ERROR;
}
