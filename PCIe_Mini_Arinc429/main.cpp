#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "PCIeMini_Arinc429.h"
#include "Hi3593Status.h"
#include "arinc429test.h"
#include "arinc429data.h"

PCIeMini_Arinc429 *dut;

// version changes
// version 02062018: added command line options to specify what external loopback test to run
// version 02082018: buffered the output of each pass to avoid being sensitive to the console output
// version 02122018: removed the output buffering and modified the communication timeout to make it less sensitive slowness.

std::stringstream buff_cout;

UINT8 configureRx(PCIeMini_Arinc429_Rx *rx)
{
	Rx_configuration conf;
	conf.parity = false;
	conf.plOn = false;
	conf.labRec = false;
	conf.rFlip = false;
	conf.rate = 0;
	conf.sdOn = false;
	conf.sd10 = true;
	conf.sd9 = true;
	for (int i = 0; i <32; i++)
		conf.labels[i] = 0xff;
	rx->setConfig(&conf);

	// control
	UINT8 ctrl = 0;
	ctrl |= conf.rFlip ? HI3593_RX_CTRL_RFLIP : 0;
	ctrl |= conf.sd9 ? HI3593_RX_CTRL_SD9 : 0;
	ctrl |= conf.sd10 ? HI3593_RX_CTRL_SD10 : 0;
	ctrl |= conf.sdOn ? HI3593_RX_CTRL_SDON : 0;
	ctrl |= conf.labRec ? HI3593_RX_CTRL_LABREC : 0;
	ctrl |= conf.plOn ? HI3593_RX_CTRL_PLON : 0;
	ctrl |= conf.rate ? HI3593_RX_CTRL_RATE : 0;

	return ctrl;
}

void configureTx(PCIeMini_Arinc429_Tx *tx, bool loopback)
{
	Tx_configuration conf;
	conf.hiZ = false;
	conf.tFlip = false;
	conf.tMode = true;
	conf.rate = 0;
	conf.tParity = false;
	conf.oddEven = false;
	conf.selftest = loopback;
	tx->setConfig(&conf);
}

void printTxFifoStatus(PCIeMini_Arinc429_Tx *tx)
{
	buff_cout << "Tx FIFO status: ";
	if (tx->isFifoEmpty())
		buff_cout << " empty";
	if (tx->isFifoHalfFull())
		buff_cout << " half-full";
	if (tx->isFifoFull())
		buff_cout << " full";
	buff_cout << endl;
}

int checkRx1Ctrl(PCIeMini_Arinc429_Rx *rx1, UINT32 value)
{
	return checkValue("Rx1Ctrl: ", rx1->getCtrlReg(), value);
}

int checkRx2Ctrl(PCIeMini_Arinc429_Rx *rx, UINT32 value)
{
	return checkValue("Rx2Ctrl: ", rx->getCtrlReg(), value);
}

int checkTxCtrl(PCIeMini_Arinc429_Tx *tx, UINT32 value)
{
	return checkValue("TxCtrl: ", tx->getCtrlReg(), value);
}

int checkTxFifoEmpty(PCIeMini_Arinc429_Tx *tx, char *msg)
{
	if (!tx->isFifoEmpty()) {
		buff_cout << "Tx FIFO is not empty " << msg << endl;
		return 1;
	}
	else
		return 0;
}

int checkTxFifoEmpty(PCIeMini_Arinc429_Tx *tx) {
	return checkTxFifoEmpty(tx, "");
}

int checkGPIOvalue(int chipNbr, UINT32 value)
{
	return checkValue("HI3593 GPIO: ", dut->getGpioInputValues(chipNbr), value);
}

void displayIrqStatus(int cs)
{
	UINT32 val = dut->getGpioInputValues(cs);
	buff_cout << "GPIO: " << hex << "0x" << val << 
		endl;
	if (val & 0x800)
		buff_cout << "Tx full, ";
	if (val & 0x400)
		buff_cout << "Tx empty, ";
	else
		buff_cout << "Tx not empty, ";
	if (val & 0x200)
		buff_cout << "Rx2 Flag on, ";
	else
		buff_cout << "Rx2 Flag off, ";
	if (val & 0x80)
		buff_cout << "Rx1 Flag on, ";
	else
		buff_cout << "Rx1 Flag off, ";
	if (val & 0x1000)
		buff_cout << "NIOS Rx1 Flag on, ";
	else
		buff_cout << "NIOS Rx1 Flag 0ff, ";
	if (val & 0x2000)
		buff_cout << "NIOS Rx2 Flag on, ";
	else
		buff_cout << "NIOS Rx2 Flag 0ff, ";
	 buff_cout << endl << endl;
}

int verifyBuffer(UINT32 *buf, UINT32 startValue, int length, int complement, int channel)
{
	// buff_cout << " verifying channel #" << channel << " length :" << dec << length << endl;
	int errNbr = 0;
	UINT32 val;
	for (int i = 0; i<length; i++) {
		val = startValue + i;
		if (complement) val =  ~val;
		if (buf[i] != val){
			errNbr++;
			if (errNbr<10)
				buff_cout << dec << "receiving word #" <<  i << " ch " << channel << ": " << buf[i] << " should be: " << val << endl;
		}
	}
	return errNbr;
}

int ExternalLoopbackTest(int verbose, int txNbr, int rxNbr1, int rxNbr2, int loopNbr){
	ARINC_status st;
	int errNbr = 0;
	int errAlreadyReported = 0;
	int totalWords = 400;
	UINT32 tw = 0x12345670;
	UINT32 buff[500];
	int tfrd;
	int i;

	// initialize for internal loopback
	dut->reset();
	buff_cout << dec << "*** External Loopback Tx #" << txNbr << " Rx #" << rxNbr1 << " and #" << rxNbr2 << ": ";
	Sleep(1);
	PCIeMini_Arinc429_Rx *rx1 = dut->rx[rxNbr1];
	PCIeMini_Arinc429_Rx *rx2 = dut->rx[rxNbr2];
	PCIeMini_Arinc429_Tx *tx = dut->tx[txNbr];

	UINT8 rxConf = configureRx(rx1);
	configureRx(rx2);
	configureTx(tx, 0);
	rx1->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
	rx2->receive(buff, 500, &tfrd, 0);		// flush the mailboxes

	Sleep(1);

	// check the control registers if in verbose mode
	if (verbose) {
		errNbr += checkRx1Ctrl(rx1, rxConf);
		errNbr += checkRx2Ctrl(rx2, rxConf);
		errNbr += checkTxCtrl(tx, 0x20);
		errNbr += checkTxFifoEmpty(tx, "after reset");
	}

	errNbr += checkGPIOvalue(txNbr, 0x680);

	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
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
			buff_cout << "Transmitting word:" << tw + i << " status=" << getArinc429ErrorMsg(st) << endl;
			if (st == ARINC429_TX_OVERFLOW)
				break;
		}
		i += tfrd;
	}
	double elapsed = calculateDelayMs(&start);
	if (verbose || (loopNbr == 694))
		buff_cout << dec << totalWords << " words in " << (int)elapsed << " ms = " << ((double)totalWords * 38000) / elapsed << " bps";

	Sleep(100);			// wait for the data to be transmitted
	errNbr += checkTxFifoEmpty(tx, " 300 ms after finishing transmitting");			// where we die
	errNbr += checkGPIOvalue(rxNbr1 / 2, 0x3680);			// check that we have received data

	// check the received data
	UINT32 *rx1Buff;
	rx1Buff = (UINT32 *)malloc(totalWords * sizeof(UINT32));
	UINT32 *rx2Buff;
	rx2Buff = (UINT32 *)malloc(totalWords * sizeof(UINT32));
	for (i = 0; i < totalWords; i++) {
		rx1Buff[i] = 0;
		rx2Buff[i] = 0;
	}
	clock_gettime(CLOCK_REALTIME, &start);
	// check the received data
	struct timespec timeoutMs;
	clock_gettime(CLOCK_REALTIME, &timeoutMs);
	int rcvd1 = 0;
	int rcvd2 = 0;
	while ((rcvd1<totalWords || rcvd2<totalWords) && calculateDelayMs(&timeoutMs) < 30) {
		st = rx2->receive(&rx2Buff[rcvd2], totalWords - rcvd2, &tfrd);
		if (tfrd) {
			rcvd2 += tfrd;
			clock_gettime(CLOCK_REALTIME, &timeoutMs);
		}
		st = rx1->receive(&rx1Buff[rcvd1], totalWords - rcvd1, &tfrd);
		if (tfrd) {
			rcvd1 += tfrd;
			clock_gettime(CLOCK_REALTIME, &timeoutMs);
		}
	}

	elapsed = calculateDelayMs(&start);
	if (elapsed == 0) elapsed = 1;
	if (rcvd1!=totalWords || rcvd2!=totalWords) errNbr++;
	verifyBuffer(rx1Buff, tw, rcvd1, 0, rxNbr1);
	verifyBuffer(rx2Buff, tw, rcvd2, 0, rxNbr2);

	clock_gettime(CLOCK_REALTIME, &timeoutMs);
	if (verbose)
		buff_cout << dec << endl << (rcvd1 + rcvd2) << " words in " << (int)elapsed << " ms = " << (int)(((double)(rcvd1 + rcvd2) * 38000 * 2) / elapsed) << " bps";

	errNbr += checkTxFifoEmpty(tx, "after receiving");

	if (errNbr == 0)
	{
		buff_cout << " PASSED" << endl;
	}
	else {
		buff_cout << " FAILED" << endl;
	}
	free (txBuff);
	free (rx1Buff);
	free (rx2Buff);
	return !!errNbr;
}

int ExternalLoopbackTest(int verbose, int txNbr, int rxNbr1, int loopNbr){
	ARINC_status st;
	int errNbr = 0;
	int errAlreadyReported = 0;
	int totalWords = 400;
	UINT32 tw = 0x12345670;
	UINT32 buff[500];
	int tfrd;
	int i;

	// initialize for external loopback
	dut->reset();
	buff_cout << dec << "*** External Loopback Tx #" << txNbr << " Rx #" << rxNbr1 << ": ";
	Sleep(1);
	PCIeMini_Arinc429_Rx *rx1 = dut->rx[rxNbr1];
	PCIeMini_Arinc429_Tx *tx = dut->tx[txNbr];

	UINT8 rxConf = configureRx(rx1);
	configureTx(tx, 0);
	rx1->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
	Sleep(1);

	// check the control registers if we have mysterious errors and we need additional diagnostics
	if (verbose) {
		errNbr += checkRx1Ctrl(rx1, rxConf);	// there is only one receiver used per test
		errNbr += checkTxCtrl(tx, 0x20);
		errNbr += checkTxFifoEmpty(tx, "after reset");
	}
	errNbr += checkGPIOvalue(txNbr, 0x680);

	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
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
			buff_cout << "Transmitting word:" << tw + i << " status=" << getArinc429ErrorMsg(st) << endl;
			if (st == ARINC429_TX_OVERFLOW)
				break;
		}
		i += tfrd;
	}
	double elapsed = calculateDelayMs(&start);
	if (verbose || (loopNbr == 694))
		buff_cout << dec << totalWords << " words in " << (int)elapsed << " ms = " << ((double)totalWords * 38000) / elapsed << " bps";

	Sleep(300);			// wait for the data to be transmitted
	errNbr += checkTxFifoEmpty(tx, " 300 ms after finishing transmitting");			// where we die

	// check the received data
	UINT32 *rx1Buff;
	rx1Buff = (UINT32 *)malloc(totalWords * sizeof(UINT32));
	if (rx1Buff == NULL) 
	{
		errNbr++;
		buff_cout << "ERROR: the process ran out of memory!" << endl;
	}
	for (i = 0; i < totalWords; i++) {
		rx1Buff[i] = 0;
	}
	clock_gettime(CLOCK_REALTIME, &start);
	// check the received data
	struct timespec timeoutMs;
	clock_gettime(CLOCK_REALTIME, &timeoutMs);
	int rcvd1 = 0;
	while ((rcvd1<totalWords) && calculateDelayMs(&timeoutMs) < 300) {
		st = rx1->receive(&rx1Buff[rcvd1], totalWords - rcvd1, &tfrd);
		if (tfrd) {
			rcvd1 += tfrd;
			clock_gettime(CLOCK_REALTIME, &timeoutMs);
		}
	}

	elapsed = calculateDelayMs(&start);
	if (elapsed == 0) elapsed = 1;
	if (rcvd1!=totalWords) errNbr++;
	verifyBuffer(rx1Buff, tw, rcvd1, 0, rxNbr1);

	clock_gettime(CLOCK_REALTIME, &timeoutMs);
	if (verbose)
		buff_cout << dec << endl << (rcvd1) << " words in " << (int)elapsed << " ms = " << (int)(((double)(rcvd1) * 38000 * 2) / elapsed) << " bps";

	if (verbose) errNbr += checkTxFifoEmpty(tx, "after receiving");

	if (errNbr == 0)
	{
		buff_cout << " PASSED" << endl;
	}
	else {
		buff_cout << " FAILED" << endl;
	}
	free (txBuff);
	free (rx1Buff);
	return !!errNbr;
}

int TxTest(int rate)
{
	Tx_configuration conf;
	PCIeMini_Arinc429_Tx* tx = dut->tx[0];
	ARINC_status st;
	UINT32 word = 0xdeadbeef;
	int sent = 0;
	int actual;
	int errors = 0;
	int overflows = 0;

	conf.hiZ = false;
	conf.tFlip = false;
	conf.tMode = true;
	conf.rate = rate;				// low speed
	conf.tParity = false;
	conf.oddEven = false;
	conf.selftest = false;
	tx->setConfig(&conf);
	UINT32 start = GetTickCount();
	while (sent <= 7000)
	{
		st = tx->transmit(&word, 1, &actual);
		sent++;

		if (st != ARINC429_NO_ERROR) {
			errors++;
			if (st == ARINC429_TX_OVERFLOW) {
				overflows++;
			}
		}
		if (sent % 1000 == 0) {
			UINT32 now = GetTickCount();
			UINT32 dly = now - start;
			double dlySec = (double)dly / 1000.0;
			start = now;
			printf("Sent full speed: %d, errors=%d, ovf=%d in %d ms (%f word/s, %f bits/s) Fifo level = %d\n", sent, errors, overflows, dly, 1000.0/ dlySec, 32000.0/ dlySec, tx->dev->TxFifoLevel());
		}
	}
	sent = 0;
	Sleep(1500); // let's the FIFO empty
	while (sent <= 7000)
	{
		st = tx->transmit(&word, 1, &actual);
		sent++;

		if (st != ARINC429_NO_ERROR) {
			errors++;
			if (st == ARINC429_TX_OVERFLOW) {
				overflows++;
			}
		}
		if (sent % 1000 == 0) {
			UINT32 now = GetTickCount();
			UINT32 dly = now - start;
			double dlySec = (double)dly / 1000.0;
			start = now;
			printf("2.5 ms wait: %d, errors=%d, ovf=%d in %d ms (%f word/s, %f bits/s) Fifo level = %d\n", sent, errors, overflows, dly, 1000.0 / dlySec, 32000.0 / dlySec, tx->dev->TxFifoLevel());
		}
		usleep(2500);
	}
	sent = 0;
	while (sent <= 7000)
	{
		st = tx->transmit(&word, 1, &actual);
		sent++;

		if (st != ARINC429_NO_ERROR) {
			errors++;
			if (st == ARINC429_TX_OVERFLOW) {
				overflows++;
			}
		}
		if (sent % 1000 == 0) {
			UINT32 now = GetTickCount();
			UINT32 dly = now - start;
			double dlySec = (double)dly / 1000.0;
			start = now;
			printf("max 2 word in FIFO: %d, errors=%d, ovf=%d in %d ms (%f word/s, %f bits/s) Fifo level = %d\n", sent, errors, overflows, dly, 1000.0 / dlySec, 32000.0 / dlySec, tx->dev->TxFifoLevel());
		}
		while(tx->dev->TxFifoLevel() > 1)
			usleep(500);
	}
	return 0;
}

int InternalLoopbackTest(int verbose, int chipNbr){
	ARINC_status st;
	int errNbr = 0;
	int totalWords = 100;
	UINT32 tw = 1000000;
	UINT32 buff[500];
	int tfrd;
	int i;
	UINT8 rxConf;

	// initialize for internal loopback
	dut->reset();
	buff_cout << "*** Internal Loopback Tx #" << chipNbr << " Rx #" << 2 * chipNbr << " and #" << 2 * chipNbr + 1 << ": ";
	Sleep(1);
	PCIeMini_Arinc429_Rx *rx1 = dut->rx[chipNbr * 2];
	PCIeMini_Arinc429_Rx *rx2 = dut->rx[chipNbr * 2 + 1];
	PCIeMini_Arinc429_Tx *tx = dut->tx[chipNbr];

	rxConf = configureRx(rx1);
	configureRx(rx2);
	configureTx(tx, 1);
	rx1->receive(buff, 500, &tfrd, 0);		// flush the mailboxes
	rx2->receive(buff, 500, &tfrd, 0);		// flush the mailboxes

	// check the control register if we have mysterious errors and we need additional diagnostics
	if (verbose) {
		errNbr += checkRx1Ctrl(rx1, rxConf);
		errNbr += checkRx2Ctrl(rx2, rxConf);
		errNbr += checkTxCtrl(tx, 0x30);
		checkTxFifoEmpty(tx, "after reset");
	}

//	errNbr += checkGPIOvalue(chipNbr, 0x680);

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
			buff_cout << "Transmitting word:" << tw + i << " status=" << getArinc429ErrorMsg(st) << endl;
			if (st == ARINC429_TX_OVERFLOW)
				break;
		}
		i += tfrd;
	}

	Sleep(200);			// wait for the data to be transmitted
	if (verbose) errNbr += checkTxFifoEmpty(tx, " 300 ms after finishing transmitting");
	errNbr += checkGPIOvalue(chipNbr, 0x3680);
	UINT32 *rx1Buff;
	rx1Buff = (UINT32 *)malloc(totalWords * sizeof(UINT32));
	UINT32 *rx2Buff;
	rx2Buff = (UINT32 *)malloc(totalWords * sizeof(UINT32));
	if (rx1Buff == NULL || rx2Buff == NULL)
	{
		errNbr++;
		buff_cout << "ERROR: the process ran out of memory!" << endl;
	}

	for (i = 0; i < totalWords; i++) {
		rx1Buff[i] = 0;
		rx2Buff[i] = 0;
	}

	// check the received data
	struct timespec timeoutMs;
	clock_gettime(CLOCK_REALTIME, &timeoutMs);
	int rcvd1 = 0;
	int rcvd2 = 0;
	while ((rcvd1<totalWords || rcvd2<totalWords) && calculateDelayMs(&timeoutMs) < 20) {
		st = rx1->receive(&rx1Buff[rcvd1], totalWords - rcvd1, &tfrd);
		if (tfrd) {
			rcvd1 += tfrd;
			clock_gettime(CLOCK_REALTIME, &timeoutMs);
		}
		st = rx2->receive(&rx2Buff[rcvd2], totalWords - rcvd2 +100, &tfrd);
		if (tfrd) {
			rcvd2 += tfrd;
			clock_gettime(CLOCK_REALTIME, &timeoutMs);
		}
	}
	if (rcvd1!=totalWords) {
		buff_cout << " Transmitted " << totalWords << ", received " << rcvd1 << " words on rcv#" << 2 * chipNbr;
		errNbr++;
	}
	if (rcvd2!=totalWords) {
		buff_cout << " Transmitted " << totalWords << ", received " << rcvd1 << " words on rcv#" << 2 * chipNbr + 1;
		errNbr++;
	}
	errNbr += verifyBuffer(rx1Buff, tw, rcvd1, 0, 2 * chipNbr);
	errNbr += verifyBuffer(rx2Buff, tw, rcvd2, 1, 2 * chipNbr + 1);
	errNbr += checkTxFifoEmpty(tx);

	if (errNbr == 0)
	{
		buff_cout << " PASSED" << endl;
	}
	else {
		buff_cout << " FAILED" << endl;
	}
	free (txBuff);
	free (rx1Buff);
	free (rx2Buff);
	return !!errNbr;
}

int labelTesting()
{
	Arinc429word w;		// the constructor sets the word to 0

	w.setData(0x1234);
	return 0;
}

class ExtLoopBack 
{
	static const int maxLb = 100;
	UINT8 txs[maxLb];
	UINT8 rxs[maxLb];
	UINT8 nbrValid;

public:
	inline ExtLoopBack() {
		nbrValid = 0;
		for (int i = 0; i < maxLb; i++) {
			txs[i] = 0;
			rxs[i] = 0;
		}
	}

	inline void add(int tx, int rx)
	{
		if (nbrValid<maxLb) {
			txs[nbrValid] = tx;
			rxs[nbrValid] = rx;
			nbrValid++;
		}
	}

	inline UINT8 getTx(UINT8 nbr)
	{
		if (nbr>=nbrValid) nbr = 0;
		return txs[nbr];
	}

	inline UINT8 getRx(UINT8 nbr)
	{
		if (nbr>=nbrValid) nbr = 0;
		return rxs[nbr];
	}

	inline UINT8 len()
	{
		return nbrValid;
	}
};

int main(int argc, char *argv[])
{
	ARINC_status st;
	char s[10];
	int verbose = 0;
	int brdNbr = 0;
	int executeLoopback = 1;
	int i;
	ExtLoopBack lpBckInfo;

	for (i = 1; i<argc; i++) {
		char t,r;
		char c = argv[i][0];
		switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			brdNbr = argv[1][0] - '0';
			break;
		case 'v':
			verbose = 1;
			break;
		case 'x':
			executeLoopback = 0;
			break;
		case 'e':
			// verify valid
			if (i+2>=argc) {
				cout << "Error: not enough parameter for the external loopback option\n";
				i+=2;
				break;
			}
			t = argv[i+1][0];
			r = argv[i+2][0];
			if (!isdigit(t) || !isdigit(r)) {
				cout << "Error: incorrect parameters for the external loopback option\n";
				i+=2;
				break;
			}
			lpBckInfo.add(t-'0', r-'0');
			i+=2;
			break;
		case '?':
			cout << endl << "Possible options: <brd nbr>, v: verbose, x: external loopback disabled" << endl;
			cout << "	e <tx#> <rx#>: ext. loopback between the transmitter tx and receiver rx." << endl;
			cout << "for example: 'arinc429test 0 x v' means: board #0, verbose, no external loopback" << endl;
			cout << "    'arinc429test v e 0 0 e 1 1 e 1 2' means: verbose,  external loopback tx0 to rx0, tx1 to rx1, tx1 to rx2" << endl << endl;
			exit (0);
		}
	}
	if ((lpBckInfo.len() == 0) && (executeLoopback == 1)){
		lpBckInfo.add(0, 0);
	}

	cout << endl << "---------------------------------" << endl;
	cout << "PCIeMini-ARINC429 Confidence Test" << endl;
	cout << "---------------------------------" << endl;
	cout << endl << "use ? as a parameter for list of options." << endl;
	cout << "Options selected" << endl;
	cout << "================" << endl;
	cout << "Using board number " << brdNbr << endl;
	if (verbose) cout << "verbose mode " << endl;
	if (executeLoopback) cout << "external loopback enabled " << endl << endl;

	cout << "Starting test" << endl;
	cout << "=============" << endl;
	dut = new PCIeMini_Arinc429();
	dut->setVerbose(verbose);
	st = dut->open(brdNbr);
	if (st != 0) {
		cout << "error returned by open: ";
		cout << getArinc429ErrorMsg(st) << endl;
		cout << "press <enter> to exit";
		cin >> s;
		return -1;
	}
	else {
		char buf[30];
		time_t now = (time_t)dut->sysid->getTimeStamp();
		strftime(buf, 30, "%m/%d/%Y %H:%M:%S", localtime(&now));
		cout << "Board opened successfully, Firmare date: " << buf << "\n";
	}

//	dut->stop();
//	dut->start();
	volatile UINT32 *rstCtl = (volatile UINT32 *)dut->getBar2Address(0x100);
//	*rstCtl = 0x1;
	*rstCtl = 0x00;
	dut->reset();
	Sleep(1);
	checkGPIOvalue(0, 0x680);
	st = dut->enableInterrupts();
	if (WD_STATUS_SUCCESS == st) {
		cout << "Interrupts enabled successfully!\n";
	}

	int loopCntr = 0;
	int totalError = 0;
	while (loopCntr<=1) {
		buff_cout.str(std::string());		// clear the buffer
		labelTesting();
		buff_cout << dec << endl << endl << "ARINC429 Test Loop # " << loopCntr << " errors = " << totalError << endl;
		totalError += InternalLoopbackTest(verbose, 0);
		totalError += InternalLoopbackTest(verbose, 1);
		TxTest(1);
		for (int i=0; i<lpBckInfo.len(); i++) {
			totalError += ExternalLoopbackTest(verbose, lpBckInfo.getTx(i), lpBckInfo.getRx(i), loopCntr);
		}
//		totalError += staticIrqTest();
//		rxIrqTest(0, verbose);
		loopCntr++;
		buff_cout << dec << endl << "Finished Loop #" << loopCntr << " errors = " << totalError << endl;
		Sleep(0000);
		cout << buff_cout.str();			// output the buffer content to stdout
	}
	dut->disableInterrupts();

	return 0;

}
