#include <string>
#include <iostream>
#include <iomanip>
#include <time.h>
#include "PCIeMini_Arinc429.h"
#include "Hi3593Status.h"
#include "arinc429test.h"

Hi3593Status *irqStat;
int irqNbr = 0;
int rxCharCnt1;
int rxCharCnt2;
int intEnabled = 0;

#ifdef WIN32
#else
void Sleep(int ms)
{
	struct timespec t1;
	int sec = ms / 1000;
	ms = ms - sec * 1000;
	t1.tv_nsec = ms * 1000000;
	t1.tv_sec = sec;
	nanosleep(&t1, NULL);
}

UINT32 GetTickCount()
{
	struct timespec ts;
	clock_gettime((clockid_t)CLOCK_REALTIME, &ts);
	return ts.tv_nsec/1000000;
}

#endif

time_t calculateDelayMs(struct timespec *stime)
{
	struct timespec etime;
	clock_gettime(CLOCK_REALTIME, &etime);
	return (etime.tv_sec - stime->tv_sec)*1000 + (etime.tv_nsec - stime->tv_nsec) / 1000000;
}

int checkPCIeIrqStatus(volatile UINT32 *PCIeIrqStatus, UINT32 value)
{
	return checkValue("PCIeIrqStatus: ", *PCIeIrqStatus, value);
}

void isrReceiver(void *brd, UINT32 data)
{
	PCIeMini_Arinc429_Rx *rx1 = dut->rx[0];
	PCIeMini_Arinc429_Rx *rx2 = dut->rx[1];
	PCIeMini_Arinc429_Rx *rx3 = dut->rx[2];
	PCIeMini_Arinc429_Rx *rx4 = dut->rx[3];
	UINT32 irqst;
	UINT32 buff[100];
	int tfrd;
	ARINC_status st;
	int keepLooping = 0;
	if (!intEnabled)
		return;

	while(((irqst = irqStat->getIrqEnable()) & 0x3140) && intEnabled) {
		if ((irqst & 0x3000) == 0)
			break;
		keepLooping = 0;
		st = rx1->receive(buff, 100, &tfrd, 0);
 		if (tfrd>0) {
			rxCharCnt1 += tfrd;
		}
		if (tfrd == 100)
			keepLooping = 1;
		tfrd = 0;

		st = rx2->receive(buff, 100, &tfrd, 0);
 		if (tfrd>0) {
			rxCharCnt2 += tfrd;
		}
 		if (tfrd == 100)
			keepLooping = 1;
		tfrd = 0;

/*		if (rx3->receive(buff, 100, &tfrd, 0) == ARINC429_NO_ERROR) {
			rxCharCnt1 += tfrd;
		}
 		if (tfrd == 100)
			keepLooping = 1;
		tfrd = 0;

		if (rx4->receive(buff, 100, &tfrd, 0) == ARINC429_NO_ERROR) {
			rxCharCnt2 += tfrd;
		}
		if (tfrd == 100)
			keepLooping = 1;
		tfrd = 0;*/

		if (keepLooping == 0) 
			break;
	}
	if (intEnabled) {
		irqStat->setIrqEnable(irqst);
	}
	irqNbr++;
}

int rxIrqTest(int chipNbr, int verbose) {
	ARINC_status st;
	int errNbr = 0;
	int totalWords = 2000;
	UINT32 tw = 0x100000;
	int tfrd;
	UINT32 buff[5000];
	int i;

	irqStat = dut->gpio[0];
	// PCIe controller registers
	volatile UINT32 * PCIeIrqStatus = (volatile UINT32*)dut->getBar0Address(0x40);
	volatile UINT32 * PCIeIrqEnable = (volatile UINT32*)dut->getBar0Address(0x50);
	// hook the ISR
	dut->hookInterruptServiceRoutine((MINIPCIE_INT_HANDLER)isrReceiver);
	irqStat->setInterruptEnableMask(0x3140);
	intEnabled = 1;

	// initialize for internal loopback
	dut->reset();
	Sleep(1);
	PCIeMini_Arinc429_Rx *rx1 = dut->rx[chipNbr * 2];
	PCIeMini_Arinc429_Rx *rx2 = dut->rx[chipNbr * 2 + 1];
	PCIeMini_Arinc429_Tx *tx = dut->tx[chipNbr];

	UINT8 rxConf = configureRx(rx1);
	configureRx(rx2);
	configureTx(tx, 1);
	rx1->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
	rx2->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
//	cout << "after initial reset \n";
	errNbr += checkPCIeIrqStatus(PCIeIrqStatus, 0x00);

	// check the registers
	checkRx1Ctrl(rx1, rxConf);
	errNbr += checkRx2Ctrl(rx2, rxConf);
	errNbr += checkTxCtrl(tx, 0x30);
	checkTxFifoEmpty(tx, "after reset");
	errNbr += checkGPIOvalue(chipNbr, 0x680);

	// initialize the interrupts
	irqNbr = 0;
	rxCharCnt1 = 0;
	rxCharCnt2 = 0;
	irqStat->setInterruptEnableMask(0x3000);
	*PCIeIrqEnable = 0x0020;					// enable the PIO irq

	UINT32 *txBuff;
	txBuff = (UINT32 *)malloc(totalWords * sizeof(UINT32));
	for (i = 0; i<totalWords; i++) {
		txBuff[i] = tw + i;
	}

	i = 0;
	while (i<totalWords) {
		st = tx->transmit(&txBuff[i], totalWords-i, &tfrd);
//		Sleep(1);
		if (st != ARINC429_NO_ERROR) {
			errNbr++;
			cout << "Transmitting word:" << tw + i << " status=" << getArinc429ErrorMsg(st) << endl;
			if (st == ARINC429_TX_OVERFLOW)
				break;
		}
		i += tfrd;
	}
	if (st != ARINC429_NO_ERROR) {
		errNbr++;
		cout << "Transmitting word:" << tw << " status=" << getArinc429ErrorMsg(st) << endl;
	}
	Sleep(300);
	st = rx1->receive(buff, 100, &tfrd, 0);
 	if (tfrd>0) {
		rxCharCnt1 += tfrd;
	}
 	rx2->receive(buff, 100, &tfrd, 0);
 	if (tfrd>0) {
		rxCharCnt2 += tfrd;
	}
	errNbr += checkGPIOvalue(chipNbr, 0x680);

	dut->unhookInterruptServiceRoutine();
//	dut->disableInterrupts();
	irqStat->setInterruptEnableMask(0);
	*PCIeIrqEnable = 0;
	dut->reset();
	rx1->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
	rx2->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
	errNbr += checkPCIeIrqStatus(PCIeIrqStatus, 0);			// everything should be disabled

	if (irqNbr >= 1)
	{
		if (verbose)
			cout << "*** Receive PCIe Interrupt test: PASSED (" << dec << rxCharCnt1 << ", " << rxCharCnt2 << " words, out of " << totalWords << ", " << irqNbr << " interrupts)" << endl;
		else
			cout << "*** Receive PCIe Interrupt test: PASSED" << endl;
	}
	else {
		cout << "*** Receive PCIe Interrupt test: FAILED" << endl;
	}
	free (txBuff);
	intEnabled = 0;
	return (irqNbr == 0);
}

void isr429(void *brd, UINT32 data)
{
//	if (irqNbr == 0) 
//		cout << "Received Irq!\n";
	Sleep(100);
	irqNbr++;
	irqStat->setInterruptEnableMask(0x0);
	irqStat->setInterruptEnableMask(0xffff);
}
int staticIrqTest() {
	// SPI status address
	irqStat = dut->gpio[0];
	irqNbr = 0;

	// PCIe controller registers
	// volatile UINT32 * PCIeIrqStatus = (volatile UINT32*)dut->getBar0Address(0x40);
	volatile UINT32 * PCIeIrqEnable = (volatile UINT32*)dut->getBar0Address(0x50);
	//	cout << "PCIe IRQ status: " << hex << "0x" << *PCIeIrqStatus << endl;

	// enable the interrupts in the PCIe controller
	// hook the ISR
	dut->hookInterruptServiceRoutine((MINIPCIE_INT_HANDLER)isr429);
	*PCIeIrqEnable = 0xffff;

	//	irqStat->reset();
	irqStat->setInterruptEnableMask(0xffff);
	//	irqStat->setDirection(0xffff);
	irqStat->setDataOut(0xffff);
	irqStat->setDataOut(0x0);

	Sleep(10);

	//	cout << "PCIe IRQ status: " << hex << "0x" << *PCIeIrqStatus << endl;
	//	cout << "PCIe IRQ enable: " << hex << "0x" << *PCIeIrqEnable << endl;
	Sleep(1000);
	irqStat->setDataOut(0xffff);
	irqStat->setDataOut(0x0);
	//	cout << "PCIe IRQ status: " << hex << "0x" << *PCIeIrqStatus << endl;
	//	cout << "PCIe IRQ enable: " << hex << "0x" << *PCIeIrqEnable << endl;
	dut->reset();

	// test the ARINC receive interrupt
	// program the receiver transmitter
	// program the SPIStatus to receive just the IRQ
	irqStat->setEdgeCapture(Hi3593Status::r1IntMask | Hi3593Status::r2IntMask);
	// edge only
	// send a character on the loopback
	// have we received the interrupt?

	//	cout << "PCIe IRQ status: " << hex << "0x" << *PCIeIrqStatus << endl;
	dut->unhookInterruptServiceRoutine();
	irqStat->setInterruptEnableMask(0);
	*PCIeIrqEnable = 0;

	if (irqNbr >= 1)
	{
		cout << "*** Static PCIe Interrupt test: PASSED" << endl;
	}
	else {
		cout << "*** Static PCIe Interrupt test: FAILED" << endl;
	}
	return (irqNbr == 0);
}

