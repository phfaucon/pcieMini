#if 0
void enableIrq(SccChannel* pIp, void(*irqHandler)(void))
{
	carrier_irqEnableIp(pIp->pDevice, pIp->slot, TRUE, irqHandler);	// enable serial interrupts
}

void disableIrq(SccChannel* pIp)
{
	carrier_irqEnableIp(pIp->pDevice, pIp->slot, FALSE, NULL);	// disable serial interrupts
}

// globals used for testing the interrupts
static char testMsg8530[] = "Testing the 85233 under interrupt\r";
static int intrIndex8530;
static int intChannel;
static int intTimeout;

// method to catch the interrupts
void SCCintReceived(void)
{
	timeout8530 = 0;
	if (intrIndex8530 > sizeof(testMsg8530)) intrIndex8530 = 0;
	while (SCC04_outchnw(&ip, intChannel, testMsg8530[intrIndex8530++]) >= 0 && loopNbr8530 >= 0) {
		loopNbr8530--;
		if (!testMsg8530[intrIndex8530])
			intrIndex8530 = 0;
	}
	// acknowledge the interrupt
	rd85233reg(&ip.channel[intChannel], R2_IVR);
	Sleep(1);
}

// test that we receive transmit interrupts
// since it is just transmission, it will work without loopback
// provided that the programmation of the chip and handshake signals allow
// for transmission.

int TestSerialSCCinterrupts(SccChannel* pIp, int chNbr)
{
	int errNbr = 0;
	int totalTestTime = 0;

	if (verbose) printf("Testing SCC interrupts Channel #%d: ", chNbr);
	SCC04_DmaEnable(pIp, chNbr, FALSE, SCC04_RX | SCC04_TX);	// not using the FIFO
	intrIndex8530 = 0;
	loopNbr8530 = SERIAL_TEST_LOOP;
	intChannel = chNbr;

	// allow the interrupts
	enableIrq(pIp, SCCintReceived);

	SCC04_setIRQflags(pIp, SCC04_MCR_ENABLE_IRQ0);
	SCC04_enInt85233(pIp, chNbr, TxINT_ENAB);

	timeout8530 = 1200;
	while (SCC04_outchnw(pIp, chNbr, '\n') && timeout8530) {
		timeout8530--;
		Sleep(1);
	}

	// set the 8530 to interrupt on transmitter ready
	for (timeout8530 = 0;
		(timeout8530 < SERIAL_TEST_LOOP * 100)
		&& loopNbr8530 > 0 && totalTestTime < 1000;
		timeout8530++) {
		totalTestTime++;
		Sleep(1);
	}
	SCC04_disInt85233(pIp, chNbr, 0x1f);	// make sure that we don't keep writing at the same time

	if (verbose && loopNbr8530 <= 0) {
		printf("PASSED! (%d milliseconds)\n\r", totalTestTime);
	}
	if (loopNbr8530 > 0) {
		errNbr = 1;
		printf("ERROR: SCC interrupt test: received %d out of %d interrupts\n\r", SERIAL_TEST_LOOP - loopNbr8530, SERIAL_TEST_LOOP);
	}


	disableIrq(pIp);

	return errNbr;
}

void intReceivedFIFO(void)
{
	loopNbr8530++;
	Sleep(10);
}

// at this point the ability to write interrupts will have been enabled, and the pre-conditions
// for the interrupts will be met.  All that should remain is to enable the bit which tells an
// individual channel to throw an interrupt
int TestSerialFIFOinterrupts(SccChannel* pIp, int chNbr, int bitNumber)
{
	uint8_t tmp;
	int timeout;

	loopNbr8530 = 0;		// loop counter

	// allow the interrupts
	enableIrq(pIp, intReceivedFIFO);
	tmp = SCC04_getIntCtrlReg(pIp, chNbr);
	SCC04_setIntCtrlReg(pIp, chNbr, tmp | 1 << bitNumber);

	// allow the FIFO interrupt
	for (timeout = 0; timeout < 500 && loopNbr8530 < 1; timeout++) {
		SCC04_setIRQflags(pIp, SCC04_MCR_ENABLE_IRQ0 | SCC04_MCR_DIS_SCC_IRQ);
		Sleep(10);
	}
	SCC04_setIntCtrlReg(pIp, chNbr, SCC04_getIntCtrlReg(pIp, chNbr) & ~(1 << bitNumber));
	disableIrq(pIp);
	SCC04_setIRQflags(pIp, 0);

	return loopNbr8530;
}

/*******************************************************************************************************************
 * checkInt0FIFO - this subroutine tests the transmit interrupts of outChNbr and the receive interrupts of inChNbr */
int SccTest::checkInt0FIFO(int trmtChannel, int rcvChannel)
{
	int fifoSize = SCC04_FIFO_MAX_SIZE; // how many characters can we write before we're full ?
	int charTrmtd = 0;
	int timeout = 0;
	uint8_t fifoStatus;
	int intRcvd;
	int errNbr = 0;

	printf("Testing FIFO interrupt generation Tx#%d, Rx#%d\n", trmtChannel, rcvChannel);

	// reset the FIFO in case there were stragglers
	SccChannel* trmtScc = getScc(trmtChannel);
	SccChannel* rcvScc = getScc(rcvChannel);

	rcvScc->resetRxFIFO();		// reset our receiver FIFO
	trmtScc->enableTxDma();
	rcvScc->enableRxDma();
	SCC04_disableTx85233(pIp, trmtChannel);

	//////////// check empty and almost empty for transmitter //////////////////////////////////
	fifoStatus = SCC04_getCR_FIFO(pIp, trmtChannel);
	intRcvd = TestSerialFIFOinterrupts(pIp, trmtChannel, 2);
	if (intRcvd && (fifoStatus & 4) != 4) {
		printf("  ERROR: rcvd IRQ while transmit channel %d, almost empty bit is NOT set \n", trmtChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 4) == 4) {
		printf("  ERROR: rcvd no IRQ while transmit channel %d, almost empty bit is set \n", trmtChannel);
		errNbr++;
	}

	intRcvd = TestSerialFIFOinterrupts(pIp, trmtChannel, 3);
	if (intRcvd && (fifoStatus & 8) != 8) {
		printf("  ERROR: rcvd IRQ while transmit channel %d, empty bit is NOT set \n", trmtChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 8) == 8) {
		printf("  ERROR: rcvd no IRQ while transmit channel %d, empty bit is set \n", trmtChannel);
		errNbr++;
	}

	// load up the transmit fifo
	charTrmtd = 0;
	while (charTrmtd < fifoSize + 4)
	{
		if (SCC04_outchnw(pIp, trmtChannel, 0) == 0) charTrmtd++; // was the write successful ?
		else timeout++;
		if (timeout > (fifoSize * 2))
			break;
		Sleep(1);
	}
	if (charTrmtd < fifoSize)
		printf("    timed out transmitting on FIFO %d, wrote %d characters\n", trmtChannel, charTrmtd);

	///////////////////////// check almost full and full interrupts for transmitter ////////////
	fifoStatus = SCC04_getCR_FIFO(pIp, trmtChannel);
	intRcvd = TestSerialFIFOinterrupts(pIp, trmtChannel, 0);
	if (intRcvd && (fifoStatus & 1) != 1) {
		printf("  ERROR: rcvd IRQ while transmit channel %d, full bit is NOT set \n", trmtChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 1) == 1) {
		printf("  ERROR: rcvd no IRQ while transmit channel %d, full bit is set \n", trmtChannel);
		errNbr++;
	}

	// check almost full and full interrupts
	intRcvd = TestSerialFIFOinterrupts(pIp, trmtChannel, 1);
	if (intRcvd && (fifoStatus & 2) != 2) {
		printf("  ERROR: rcvd IRQ while transmit channel %d, almost full bit is NOT set \n", trmtChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 2) == 2) {
		printf("  ERROR: rcvd IRQ while transmit channel %d, almost full bit is NOT set \n", trmtChannel);
		errNbr++;
	}

	////////////////////////receiver ////////////////////////////////////////////
	fifoStatus = SCC04_getCR_FIFO(pIp, rcvChannel);
	intRcvd = TestSerialFIFOinterrupts(pIp, rcvChannel, 6);
	if (intRcvd && (fifoStatus & 0x40) != 0x40) {
		printf("  ERROR: rcvd IRQ while receive channel %d, almost empty bit is NOT set \n", rcvChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 0x40) == 0x40) {
		printf("  ERROR: rcvd NO IRQ while receive channel %d, almost empty bit is set \n", rcvChannel);
		errNbr++;
	}

	intRcvd = TestSerialFIFOinterrupts(pIp, rcvChannel, 7);
	if (intRcvd && (fifoStatus & 0x80) != 0x80) {
		printf("  ERROR: rcvd IRQ while receive channel %d, empty bit is NOT set \n", rcvChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 0x80) == 0x80) {
		printf("  ERROR: rcvd NO IRQ while receive channel %d, empty bit is set \n", rcvChannel);
		errNbr++;
	}


	// allow the SCC to move the data and check to ensure the receive FIFO is throwing interrupts
	fifoStatus = SCC04_getCR_FIFO(pIp, trmtChannel);
	SCC04_enableTx85233(pIp, trmtChannel);

	for (timeout = 0; timeout < fifoSize && (SCC04_getCR_FIFO(pIp, rcvChannel) & 0x10) == 0; timeout++)
		Sleep(1); // delay 5000 for each character in the FIFO to transfer
	fifoStatus = SCC04_getCR_FIFO(pIp, rcvChannel);
	if ((fifoStatus & 0x10) != 0x10)
		printf("    timed out receiving on FIFO %d\n", rcvChannel);

	// test receiver almost full and full
	fifoStatus = SCC04_getCR_FIFO(pIp, rcvChannel);
	intRcvd = TestSerialFIFOinterrupts(pIp, rcvChannel, 4);
	if (intRcvd && (fifoStatus & 0x10) != 0x10) {
		printf("  ERROR: rcvd IRQ while receive channel, full bit is NOT set \n %d", rcvChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 0x10) == 0x10) {
		printf("  ERROR: rcvd NO IRQ while receive channel, full bit is set \n %d", rcvChannel);
		errNbr++;
	}

	intRcvd = TestSerialFIFOinterrupts(pIp, rcvChannel, 5);
	if (intRcvd && (fifoStatus & 0x20) != 0x20) {
		printf("  ERROR: rcvd IRQ while receive channel, almost full bit is NOT set \n %d", rcvChannel);
		errNbr++;
	}
	if (!intRcvd && (fifoStatus & 0x20) == 0x20) {
		printf("  ERROR: rcvd NO IRQ while receive channel, almost full bit is set \n %d", rcvChannel);
		errNbr++;
	}

	// finished, put things back the way they were
	SCC04_setIRQflags(pIp, 0);
	SCC04_DmaEnable(pIp, trmtChannel, FALSE, SCC04_TX); // disable the transmit FIFO 
	SCC04_DmaEnable(pIp, rcvChannel, FALSE, SCC04_RX);	// disable the receive FIFO

	if (errNbr) printf("%d ERROR(s)\n", errNbr);
	else printf("NO ERRORS\n");
	return errNbr;
}



#endif