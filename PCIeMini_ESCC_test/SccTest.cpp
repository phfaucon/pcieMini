/***************************************************************************/
/*                                                                         */
/*     SerialFIFO.C                                                        */
/*                                                                         */
/*     Confidence test of the PCIe-Mini-ESCC                               */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
//
// Copyright (c) 2020 Alphi Technology Corporation.  All Rights Reserved
//
// You are hereby granted a copyright license to use, modify and
// distribute this SOFTWARE so long as the entire notice is retained
// without alteration in any modified and/or redistributed versions,
// and that such modified versions are clearly identified as such.
// No licenses are granted by implication, estopple or otherwise under
// any patents or trademarks of Alphi Technology Corporation (Alphi).
//
// The SOFTWARE is provided on an "AS IS" basis and without warranty,
// to the maximum extent permitted by applicable law.
//
// ALPHI DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED, INCLUDING
// WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
// AND ANY WARRANTY AGAINST INFRINGEMENT WITH REGARD TO THE SOFTWARE
// (INCLUDING ANY MODIFIED VERSIONS THEREOF) AND ANY ACCOMPANYING
// WRITTEN MATERIAL.
//
// To the maximum extent permitted by applicable law, IN NO EVENT SHALL
// ALPHI BE LIABLE FOR ANY DAMAGE WHATSOEVER (INCLUDING WITHOUT LIMITATION,
// DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
// BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM THE USE
// OR INABILITY TO USE THE SOFTWARE.  GMS assumes no responsibility for
// for the maintenance or support of the SOFTWARE
//


#include <stdio.h>
#include <string.h>
#include "escc_defines.h"
#include "SccChannel.h"
#include "SccTest.h"
using namespace std;

int init_scc_chan(PCIeMini_ESCC* brd, int chan);

void SccTest::setVerbose(int verbose)
{
	dut->verbose = verbose;
}

// other test globals
#define SERIAL_TEST_LOOP 100 
static int loopNbr8530;
static int timeout8530;
extern int totalCharSent;
int errNbr[4] = { 0, 0, 0, 0 };
uint8_t TxENAB;

#define BISYNC_LOW	0x16
#define BISYNC_HIGH	0x16

#define SCC04_SCC_PCLK 16000000
#define SYNC_SPEED 100000
#define LOW_BR_DIV_SYNC   ((SCC04_SCC_PCLK/(2*SYNC_SPEED))-2)

uint8_t z8530_sync_1M[] =
{
	R9,		FHWRES,
	R4,		SYNC_ENAB | BISYNC | X1CLK/*|PAR_ENA*/,
	R10,	NRZ,
	R6,		BISYNC_LOW,
	R7,		BISYNC_HIGH,
	R2_IVR,		0,	/* No vector */
	R11,	RCRTxCP | TCBR | TRxCBR | TRxCOI,
	//	R12_BRGL, 	255,
		R12_BRGL, 	LOW_BR_DIV_SYNC,
		R13_BRGH, 	0,
		R14,	DTRREQ | DISDPLL | BRENABL | BRSRC,		// DTRREQ is needed by the transmit FIFO
		R15,	0,
		R5,		(uint8_t)(RTS | TxENAB | Tx8 | DTR),
		R3,		RxENABLE | Rx8,
		R1,		0,
		R7 + 16, 	0,								/* AUTOMATIC RTS RE/DEASSERT */
		R9,		NV | NORESET,
		255
};

#define LOW_BR_DIV   ((SCC04_SCC_PCLK/(32*19200))-2)

int pseudorandom(int j)
{
	int i = (j % 28) + 0x41;
	// if you want it to output A-Z \n
	if (j == 91)
		return 0xA;
	if (j == 92)
		return 0xD;
	return i; // 0x41 == 'A' +25 == 'Z'

#if 0 // to output more random characters enable this section
	return (char)j * 17;
#endif
}

int invPseudorandom(int j)
{
	// if you want it to output A-Z \n
	if (j == 0xA)
		return 26;
	if (j == 0xD)
		return 27;
	return j - 0x41;

}

int SccTest::checkFifoEmpty(SccChannel* scc, const char* s)
{
	int errNbr = 0;

	// check that the FIFOs are empty
	if (!scc->rxFifo->isFifoEmpty())
	{
		buff_cout << "ERROR: Rx FIFO is not empty (" << scc->rxFifo->usage() << " locations used)" << endl;
		errNbr++;
	}
	if (!scc->txFifo->isFifoEmpty())
	{
		buff_cout << "ERROR: Tx FIFO is not empty (" << scc->txFifo->usage() << " locations used)" << endl;
		errNbr++;
	}
	return errNbr;
}

int SccTest::loopbackTest(int trmtChannel, int rcvChannel, int nbrChar, int trmtDMA, int rcvDMA, bool useRs232)
{
	int charRcvd0 = 0;
	int charTrmtd0 = 0;
	int errOfst0 = 0;
	int charRcvd1 = 0;
	int charTrmtd1 = 0;
	int errOfst1 = 0;		// used to get only one error if a character has been lost
	int timeout = 0;
	int c1, c2;
	int data;
	int errNbr = 0;
	const char* buff1;
	const char* buff2;
	int errNbr0 = 0;
	int errNbr1 = 0;

	SccChannel* trmtScc = getScc(trmtChannel);
	SccChannel* rcvScc = getScc(rcvChannel);

	init_scc_chan(dut, trmtChannel);
	init_scc_chan(dut, rcvChannel);

	if (useRs232) {
		trmtScc->scc_config.bufferConfig = LTC2872::RS_232_buffers;
		rcvScc->scc_config.bufferConfig = LTC2872::RS_232_buffers;
	}


	if (trmtDMA) {
		buff1 = "DMA";
		trmtScc->enableTxDma();
	}
	else {
		buff1 = "direct";
		trmtScc->disableTxDma();
	}
	if (rcvDMA) {
		buff2 = "DMA";
		rcvScc->enableTxDma();
	}
	else {
		buff2 = "direct";
		rcvScc->disableTxDma();
	}

	buff_cout << "Testing " << (useRs232?"RS232":"RS422") << " loopback, channel #" << trmtChannel << "(" << buff1 << ")<->#" << rcvChannel << " (" << buff2 << ")";
	
	errNbr0 += checkFifoEmpty(rcvScc, "receiver");
	errNbr0 += checkFifoEmpty(trmtScc, "transmitter");

	while ((charRcvd0 < nbrChar || charRcvd1 < nbrChar) && timeout < 10000) {
		// output pseudo-random number t->r
		c1 = pseudorandom(charTrmtd0);
		if (charTrmtd0 < nbrChar)
			if (trmtScc->outchnw(c1) == 0) charTrmtd0++;

		// output pseudo-random number r->t
		c1 = pseudorandom(charTrmtd1);
		if (charTrmtd1 < nbrChar)
			if (rcvScc->outchnw(c1) == 0) charTrmtd1++;

		// receiving on r if there is anything
		data = rcvScc->inchnw();
		if (data >= 0) {		// we received a character
			c2 = (uint8_t)data;
			c1 = pseudorandom(charRcvd0 + errOfst0);
			if (c1 != c2) {
				// we are probably off by a character or 2
				errOfst0 = invPseudorandom(c2) - charRcvd0 % 28;
				if (errOfst0 < 0) errOfst0 += 28;
				if (errNbr0 <= 10) buff_cout << "ERROR #" << trmtChannel << "->#" << rcvChannel <<
					": char #" << charRcvd0 << ": sent 0x" << hex << pseudorandom(charRcvd0) << " (0x " << c1 << 
					"), received 0x" << c2 << ", offset " << dec << errOfst0 << endl;
				errNbr0++;
			}
			charRcvd0++;
			timeout = 0;
		}

		// receiving on t if there is anything
		data = trmtScc->inchnw();
		if (data >= 0) {		// we received a character
			c2 = (uint8_t)data;
			c1 = pseudorandom(charRcvd1 + errOfst1);
			if (c1 != c2) {
				// we are probably off by a character or 2
				errOfst1 = invPseudorandom(c2) - charRcvd1 % 28;
				if (errOfst1 < 0) errOfst1 += 28;
				if (errNbr1 <= 10) buff_cout << "ERROR #" << rcvChannel << "->#" << trmtChannel <<
					": char #" << charRcvd1 << ": sent 0x" << hex << c1 << ", received 0x" << c2 << ", offset " << dec << errOfst1 << endl;
				errNbr1++;
			}
			charRcvd1++;
			timeout = 0;
		}

		timeout++;
	}

	errNbr0 += nbrChar - charRcvd0;
	errNbr1 += nbrChar - charRcvd1;

	if (errNbr + errNbr0 + errNbr1 == 0) {
		buff_cout << " PASSED" << endl;
	}
	else {
		if (errNbr0) {
			buff_cout << "#" << trmtChannel << "->#" << rcvChannel << ": received " << charRcvd0 << " (" 
				<< rcvScc->rxFifo->bytesReceived << ") characters out of " << nbrChar <<
				", " << errNbr0 << " errors" << endl;
		}
		if (errNbr1) {
			buff_cout << "#" << rcvChannel << "->#" << trmtChannel << ": received " << charRcvd1 << " (" 
				<< trmtScc->rxFifo->bytesReceived << ") characters out of " << nbrChar <<
				", " << errNbr1 << " errors" << endl;
		}
	}

	// check that the FIFOs are empty
	errNbr += checkFifoEmpty(rcvScc, "receiver");
	errNbr += checkFifoEmpty(trmtScc, "transmitter");
	trmtScc->disableTxDma();
	rcvScc->disableRxDma(); 

	return !!(errNbr + errNbr0 + errNbr1);
}

#define getTxFifoStatus() trmtScc->txFifo->getCtrlReg()
#define getRxFifoStatus() rxFifo->getCtrlReg()

int SccTest::checkFifo(int chNbr, int verbose)
{
	int errNbr = 0;
	int rcvChBnr = chNbr ? 0 : 1;
	SccChannel* trmtScc = getScc(chNbr);
	SccChannel* rcvScc = getScc(rcvChBnr);
	SccFifo* rxFifo = rcvScc->rxFifo;

	trmtScc->reset();
	rcvScc->reset();
	uint32_t prevValueTx = 0;
	uint32_t prevValueRx = 0;
	if (verbose)
		rcvScc->dump_regs();
	trmtScc->scc_config.baudRate = 115200;
	trmtScc->scc_config.bufferConfig = LTC2872::RS_422_buffers;
	rcvScc->scc_config.baudRate = 115200;
	rcvScc->scc_config.bufferConfig = LTC2872::RS_422_buffers;

	buff_cout << "Communication Test from channel Channel #" << chNbr << " to channel #" << rcvChBnr << ": ";

	init_scc_chan(dut, chNbr);
	init_scc_chan(dut, rcvChBnr);

	for (int i = 0; i < 2052; i++)
	{
		trmtScc->outch('0'+ (i%75));
	}

	prevValueTx = 0;
	prevValueRx = 0;
	// wait a little to make sure all the bytes have arrived
	int timeout;
	for (timeout = 0; (timeout <= 1024) && (!rxFifo->isFifoFull()); timeout++)
	{
		Sleep(1);
	}

	// read the characters from the receive FIFO
	int c;
	bool prevEmpty = false;
	bool prevAlmostEmpty = false;
	bool prevAlmostFull = true;
	bool prevFull = true;


	for (int i = 0; i < 2052; i++)
	{
		int usage = rcvScc->rxFifo->usage();
		if (usage != 2052 - i)
		{
			errNbr++;
			buff_cout << "Error: Wrong number of character in FIFO. should be " << 2052 - i << ", is " << usage << endl;
		}
		c = rcvScc->inchnw();
		if (c < 0) {
			// we ran out of characters early
			errNbr++;
			buff_cout << "Error: we ran out of characters early!\n";
			break;
		}
		if (c != '0' + (i % 75)) {
			buff_cout << "Error char#" << i << ",wrote " << '0' + (i % 75) << " read " << c << endl;
			errNbr++;
		}

		if (verbose) {
			bool b = rcvScc->rxFifo->isFifoEmpty();
			if (b != prevEmpty) {
				buff_cout << "Info: usage=" << dec << usage << ", FIFO became " << (b ? "" : "not") << " empty" << endl;
				prevEmpty = b;
			}
			b = rcvScc->rxFifo->isFifoAlmostEmpty();
			if (b != prevAlmostEmpty) {
				buff_cout << "Info: usage=" << dec << usage << ", FIFO became " << (b ? "" : "not") << " almost empty" << endl;
				prevAlmostEmpty = b;
			}
			b = rcvScc->rxFifo->isFifoAlmostFull();
			if (b != prevAlmostFull) {
				buff_cout << "Info: usage=" << dec << usage << ", FIFO became " << (b ? "" : "not") << " almost full" << endl;
				prevAlmostFull = b;
			}
			b = rcvScc->rxFifo->isFifoFull();
			if (b != prevFull) {
				buff_cout << "Info: usage=" << dec << usage << ", FIFO became " << (b ? "" : "not") << " full" << endl;
				prevFull = b;
			}
		}
	}
	if (!rcvScc->rxFifo->isFifoEmpty()) {
		buff_cout << "Error: Fifo is not empty, and it contains " << rcvScc->rxFifo->usage() << " bytes" << endl;
		errNbr++;
	}
	if (errNbr == 0)
	{
		buff_cout << " PASSED" << endl;
	}
	else {
		buff_cout << " FAILED " << errNbr << " errors" << endl;
	}
	return errNbr;
}

int SccTest::isPacketValid(char* packetIn, int length)
{
	int i, j = 0;

	if (length < 26) return FALSE; // we are missing data
//	for(i=0; i<length; i++){
//		if(packetIn[i] ==22); // it is Synchronous idle byte
//		else{
//			packetIn[j] = packetIn[i];
//			j++;
//			packetIn[i]=22; // we've recovered the important data, clear so we don't fail here
//		}
//	}
	for (i = 0; i < length; i++)
	{
		if (packetIn[i] == 22); // it's a synchronous idle byte
		else if (packetIn[i] != packet[i + 3]) return FALSE;
	}
	return TRUE;
}

int SccTest::syncLoopbackTest(int trmtChannel, int rcvChannel, int nbrPackets, int baudRate, int useFifo)
{
	// stats
	int charRcvd = 0;
	int charRejected = 0;
	int charTrmtd = 0;

	int pcktRcvd = 0;
	int pcktTrmtd = 0;
	int pcktInError = 0;
	int packetSize[100];
	char packetIn[10][MAX_PACKET_LENGTH];
	int verbose = 1;
	SccChannel* trmtScc = getScc(trmtChannel);
	SccChannel* rcvScc = getScc(rcvChannel);
	for (int channelNbr = 0; channelNbr < 2; channelNbr++) {
//		getScc(channelNbr)->config(&trmtScc->scc_config);
		init_scc_chan(dut, channelNbr);
	}
	for (int channelNbr = 0; channelNbr < 2; channelNbr++) {
		getScc(channelNbr)->channelLoad(z8530_sync_1M);
	}

	int data;
	int receiving = 0;
	uint8_t reg0Tx;
	uint8_t reg0Rx;
	const char* temp;
	int packetPos = 0;
	int i = 0, j;
	int packetInLength = 0;
	int timeout;

	//	initialize packet buffer
	packet[i++] = BISYNC_LOW;
	packet[i++] = BISYNC_HIGH;
	packet[i++] = 0x02;
	j = i;
	for (; i < j + 26; i++)
		packet[i] = 'A' + i - j;
	packet[i++] = 0x04;
	packetLength = i;

//	SCC04_SetReg(pIp, SCC04_LOAD_RES_ENABLE_120, 0x00);
	trmtScc->sccRegisterWrite(R12_BRGL, trmtScc->brgDivLow(SCC04_SCC_PCLK, 1, baudRate));

	if (useFifo) {
		trmtScc->enableTxDma();
		rcvScc->enableRxDma();
	}
	else {
		trmtScc->disableTxDma();
		rcvScc->disableRxDma();
	}

	//	wr85233reg( ptrRx, R3, RxENABLE|Rx8|ENT_HM);
	trmtScc->enableRTS();
	rcvScc->enableRTS();
	if (!useFifo) temp = "NO ";
	else temp = "";
	cout << "Loopback access " << temp << "FIFO, Bisync, " << (float)baudRate / 1000000 << " Mbps, channel #" << trmtChannel 
		<< "/#" << rcvChannel << endl;

	trmtScc->outch(BISYNC_LOW);
	trmtScc->outch(BISYNC_HIGH);
	trmtScc->outch(BISYNC_LOW);
	trmtScc->outch(BISYNC_HIGH);

	trmtScc->sccRegisterRead(0, &reg0Tx);
	if (reg0Tx & TxEOM) {
		trmtScc->sccRegisterWrite(0, RES_EOM_L);	/* Reset error */
	}

	rcvScc->sccRegisterRead(0, &reg0Rx);
	if (reg0Rx & TxEOM) {
		rcvScc->sccRegisterWrite(0, RES_EOM_L);	/* Reset error */
	}

	packetPos = 0;
	pcktTrmtd = 0;
	timeout = 0;
	// starts sending the packets
	while (pcktRcvd < nbrPackets && timeout < 10000) {
		// transmit
		if (pcktTrmtd < nbrPackets) {
			if (packetPos < packetLength) {
				if (trmtScc->outchnw(packet[packetPos]) == 0) {
					packetPos++;
					charTrmtd++;
				}
			}
			else {
				pcktTrmtd++;
				packetPos = 0;
			}
		}

		// receive
		data = rcvScc->inchnw();
		if (data >= 0) {
			switch (data) {
			case 0x02: receiving = 1;
				packetInLength = 0;
				break;
			case 0x04: receiving = 0;
				packetSize[pcktRcvd] = packetInLength;
				if (!isPacketValid(packetIn[pcktRcvd], packetInLength))
					pcktInError++;
				pcktRcvd++;
				break;
			case 0x16: break; // sync idle character
			default:
				if (!receiving) {
					charRejected++;
					break;
				}
				if (packetInLength < MAX_PACKET_LENGTH && pcktRcvd < 10)
					packetIn[pcktRcvd][packetInLength] = data;
				packetInLength++;
				charRcvd++;
				break;
			}
		}
		timeout++;
	}

	if (verbose > 0 || pcktInError) {
		buff_cout << "char transmitted " << charTrmtd << ", received " << charRcvd << ", rejected " << charRejected << endl;
		buff_cout << "packets transmitted " << pcktTrmtd << ", received " << pcktRcvd << ", error " << pcktInError << endl;
		if (pcktInError) {
			buff_cout << "Packets received:\n";
			for (data = 0; data < pcktRcvd; data++) {
				packetIn[data][packetSize[data]] = 0;
				buff_cout << "#" << data << ": length=" << packetSize[data] << " '" << packetIn[data] << "'" << endl;
			}
		}
		buff_cout << endl;
	}

	trmtScc->disableTxDma();
	rcvScc->disableRxDma();

	return !!(pcktInError + abs(pcktTrmtd - pcktRcvd));
}

/** @brief Check interrupt vector access
 *
 * Please note that the IVR must be accessed from the channel A side.
 * Please refer to the Z85230 datasheet for more info.
 * @retval number of errors.
 */
int SccTest::checkIrqVectorRegister()
{
	int errNbr = 0;
	int testNbr = 0;
	SccChannel* scc = getScc(0);
	uint8_t c1;
	buff_cout << "85233 channel #" << 0 << " register access: ";
	for (int j = 0; j < 256; j++) {
		testNbr++;
		scc->sccRegisterWrite(R2_IVR, j);
		scc->sccRegisterRead(R2_IVR, &c1);
		//	c1 = data;
		if (j != c1) {
			if (errNbr < 8) {
				buff_cout << "ERROR: SCC register access test: IRQ vector, wrote 0x" << hex << j << ", read 0x" << c1 << endl;
				Sleep(1);
			}
			errNbr++;

		}
	}

	if (errNbr == 0)
	{
		buff_cout << " PASSED" << endl;
	}
	else {
		buff_cout << " FAILED " << errNbr << " errors" << endl;
	}

	return errNbr;
}

int SccTest::check_ab(int verbose)
{
	int i;
	int errNbr = 0;
	SccChannel* scc_A = getScc(0);
	SccChannel* scc_B = getScc(1);

	uint8_t patt, res, r12a_tcl, r12b_tcl, r13a_tch, r13b_tch;

	scc_A->sccRegisterRead(R12_BRGL, &r12a_tcl);
	scc_B->sccRegisterRead(R12_BRGL, &r12b_tcl);
	scc_A->sccRegisterRead(R13_BRGH, &r13a_tch);
	scc_B->sccRegisterRead(R13_BRGH, &r13b_tch);

	buff_cout << "85233 check A/B register access: ";
	for (i = 0; i < 8; i++) {
		patt = 1 << i;
		scc_A->sccRegisterWrite(R12_BRGL, patt);
		scc_B->sccRegisterWrite(R12_BRGL, ~patt);
		scc_A->sccRegisterWrite(R13_BRGH, ~patt);
		scc_B->sccRegisterWrite(R13_BRGH, patt);

		scc_A->sccRegisterRead(R12_BRGL, &res);
		if (res != patt) {
			buff_cout << "A/B chan A rec " << hex << res << " expect " << patt << endl;
			errNbr++;
		}
		scc_B->sccRegisterRead(R12_BRGL, &res);
		if (res != ((~patt) & 0xff)) {
			buff_cout << "A/B chan B rec " << hex << res << " expect " << ((~patt) & 0xff) << endl;
			errNbr++;
		}
		scc_A->sccRegisterRead(R13_BRGH, &res);
		if (res != ((~patt) & 0xff)){
			buff_cout << "A/B chan A rec " << hex << res << " expect " << ((~patt) & 0xff) << endl;
			errNbr++;
		}
		scc_B->sccRegisterRead(R13_BRGH, &res);
		if (res != patt) {
			buff_cout << "A/B chan B rec " << hex << res << " expect " << patt << endl;
			errNbr++;
		}
	}

	scc_A->sccRegisterWrite(R12_BRGL, r12a_tcl);
	scc_B->sccRegisterWrite(R12_BRGL, r12b_tcl);
	scc_A->sccRegisterWrite(R13_BRGH, r13a_tch);
	scc_B->sccRegisterWrite(R13_BRGH, r13b_tch);

	if (errNbr == 0)
	{
		buff_cout << " PASSED" << endl;
	}
	else {
		buff_cout << " FAILED " << errNbr << " errors" << endl;
	}
	return errNbr;
}

// method to catch the interrupts
static SccChannel* scc_int;
int intrIndex8530;
int intChannel;
static const char* testMsg8530 = "This is a test!";

void SCCintReceived(void* brd, UINT32 data)
{
	uint8_t ivr;

	timeout8530 = 0;
	if (intrIndex8530 > sizeof(testMsg8530)) intrIndex8530 = 0;
	while (scc_int->outchnw(testMsg8530[intrIndex8530++]) >= 0 && loopNbr8530 >= 0) {
		loopNbr8530--;
		if (!testMsg8530[intrIndex8530])
			intrIndex8530 = 0;
	}
	// acknowledge the interrupt
	scc_int->sccRegisterRead(R2_IVR, &ivr);
	Sleep(1);
}

// test that we receive transmit interrupts
// since it is just transmission, it will work without loopback
// provided that the programmation of the chip and handshake signals allow
// for transmission.

int SccTest::TestSerialSCCinterrupts(int chNbr, int verbose)
{
	int errNbr = 0;
	int totalTestTime = 0;
	scc_int = getScc(chNbr);

	SccChannel* scc_int = getScc(chNbr);

	uint32_t* scc = (uint32_t*)scc_int->baseAddress;
	uint32_t* sccTxFifoCtrl = &scc[4];
	uint32_t* txData = &scc[10];

	scc_int->reset();
	if (verbose) 
		buff_cout << "after resetFIFOs - TxFifoCtrl: 0x" << hex << *sccTxFifoCtrl << endl;
	uint32_t prevValueTx = 0;
	uint32_t prevValueRx = 0;
	scc_int->scc_config.baudRate = 115200;
	scc_int->scc_config.bufferConfig = LTC2872::RS_422_buffers;

	scc_int->config(&scc_int->scc_config);

	buff_cout << "Testing SCC interrupts Channel #" << chNbr << " : ";
	scc_int->disableRxDma();
	scc_int->disableTxDma();

	intrIndex8530 = 0;
	loopNbr8530 = SERIAL_TEST_LOOP;
	intChannel = chNbr;
	uint8_t irqStatus;

	// allow the interrupts
	PCIeMini_status st = dut->hookInterruptServiceRoutine((MINIPCIE_INT_HANDLER)SCCintReceived);
	if (st != WD_STATUS_SUCCESS)
		buff_cout << "hookInterruptServiceRoutine failed, error #" << st << endl;

	scc_int->setSccControlRegister(scc_int->sccControlIrqEnableMask);
	scc_int->enableIntSCC(TxINT_ENAB);
	scc_int->sccRegisterRead(R3, &irqStatus);

	st = dut->enableInterrupts();
	if (st != WD_STATUS_SUCCESS)
		buff_cout << "enableInterrupts failed, error #" << st << endl;

	if (verbose) {
		buff_cout << "REG 3: IRQ status 0x"<< hex << irqStatus << endl;
		buff_cout << "REG 1: 0x0x" << hex << scc_int->cachedRegs[1] << endl;
		buff_cout << "REG 9: 0x0x" << hex << scc_int->cachedRegs[9] << endl;
		buff_cout << "PCI Express Interrupt Status Register: 0x04x" << hex << dut->cra->getIrqStatus() << endl;
		buff_cout << "PCI Express Interrupt Control Register: 0x04x" << hex << dut->cra->getIrqEnableMask() << endl;
	}

	timeout8530 = 1200;
	while (scc_int->outchnw('\n') && timeout8530) {
		timeout8530--;
		Sleep(1);
	}
	if (timeout8530 == 0)
	{
		buff_cout << "timed out waiting to send\n";
	}

	// set the 8530 to interrupt on transmitter ready
	for (timeout8530 = 0;
		(timeout8530 < SERIAL_TEST_LOOP * 100)
		&& loopNbr8530 > 0 && totalTestTime < 1000;
		timeout8530++) {
		totalTestTime++;
		Sleep(1);
	}
	scc_int->sccRegisterRead(R3, &irqStatus);
	if (verbose) {
		buff_cout << "REG 3: IRQ status 0x" << hex << irqStatus << endl;
		buff_cout << "REG 1: 0x" << hex << scc_int->cachedRegs[1] << endl;
		buff_cout << "REG 9: 0x" << hex << scc_int->cachedRegs[9] << endl;
		buff_cout << "Control register: 0x" << hex << scc_int->getSccControlRegister() << endl;
		buff_cout << "PCI Express Interrupt Status Register: 0x04x" << hex << dut->cra->getIrqStatus() << endl;
		buff_cout << "PCI Express Interrupt Control Register: 0x04x" << hex << dut->cra->getIrqEnableMask() << endl;
	}
	scc_int->disableIntSCC(0x1f);	// make sure that we don't keep writing at the same time

	dut->disableInterrupts();
	dut->unhookInterruptServiceRoutine();

	if (errNbr == 0)
	{
		buff_cout << " PASSED (" << totalTestTime << " milliseconds)" << endl;
	}
	else {
		buff_cout << " FAILED SCC interrupt test: received " << SERIAL_TEST_LOOP - loopNbr8530 << " out of " << SERIAL_TEST_LOOP << " interrupts" << endl;
	}
	return errNbr;
}

int SccTest::mainTest(int brdNbr)
{
	int loopCntr = 0;
	int totalError = 0;
	dut->setVerbose(1);
	PCIeMini_status st = dut->open(brdNbr);

	cout << "Opening the PCIeMini_ESCC: " << getAlphiErrorMsg(st) << endl;
	if (st != ERRCODE_NO_ERROR) {
		cout << "Exiting: Press <Enter> to exit." << endl;
		getc(stdin);
		exit(0);
	}

	cout << "FPGA ID: 0x" << hex << dut->getFpgaID() << endl;
	cout << "FPGA Time Stamp: 0x" << dut->getFpgaTimeStamp() << endl;
	if (dut->getFpgaID() != 0x100) {
		cout << "Error: This board is not a PCIeMini-ESCC" << endl;
		cout << "Exiting: Press <Enter> to exit." << endl;
		getc(stdin);
		exit(0);
	}

	cout << endl << "---------------------------------" << endl;
	cout << "PCIeMini-ESCC Confidence Test" << endl;
	cout << "---------------------------------" << endl;
	cout << endl << "use ? as a parameter for list of options." << endl;
	cout << "Options selected" << endl;
	cout << "================" << endl;
	cout << "Using board number " << brdNbr << endl;
	if (verbose) cout << "verbose mode " << endl;	
	
	while (loopCntr <= 100) {
		buff_cout.str(std::string());		// clear the buffer
		buff_cout << dec << endl << endl << "ARINC429 Test Loop # " << loopCntr << " errors = " << totalError << endl;
		//		totalError += tst.TestSerialFIFO();
		totalError += checkIrqVectorRegister();
		totalError += check_ab(0);
		totalError += TestSerialSCCinterrupts(0,0);
//		totalError += syncLoopbackTest(0, 1, 10, 1000000, 1); // 1.0Mbps, FIFO, Tx =0, Rx =1
		totalError += checkFifo(0,0);
		totalError += loopbackTest(0, 1, 2000, 0, 0, true);
		totalError += loopbackTest(1, 0, 2000, 0, 0, true);
		totalError += loopbackTest(0, 1, 2000, 0, 0);
		totalError += loopbackTest(1, 0, 2000, 0, 0);
		totalError += loopbackTest(0, 1, 2000, 1, 1);
		totalError += loopbackTest(1, 0, 2000, 1, 1);
		loopCntr++;
		buff_cout << dec << endl << "Finished Loop #" << loopCntr << " errors = " << totalError << endl;
		Sleep(0000);
		cout << buff_cout.str();			// output the buffer content to stdout
	}
	dut->disableInterrupts();
	dut->close();

	return 0;

}