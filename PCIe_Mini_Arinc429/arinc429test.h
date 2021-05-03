#ifndef _ARINC_429_TEST_H
#define _ARINC_429_TEST_H
#include <iostream>
#include <sstream>
#include "PCIeMini_Arinc429.h"

static const int busWidth = 1;
static const int data_reg = 0 * busWidth;
static const int polarity_reg = 1 * busWidth;
static const int edge_reg = 2 * busWidth;
static const int irq_status_reg = 3 * busWidth;
static const int irq_enable_reg = 4 * busWidth;
static const int direction_reg = 5 * busWidth;
static const int data_out_reg = 6 * busWidth;
static const int irq_delay = 7 * busWidth;

extern PCIeMini_Arinc429 *dut;
extern std::stringstream buff_cout;

UINT8 configureRx(PCIeMini_Arinc429_Rx *rx);
void configureTx(PCIeMini_Arinc429_Tx *tx, bool loopback);

// display routines
inline int checkValue(const char *msg, UINT32 actual, UINT32 target)
{
	if (actual != target) {
		buff_cout << msg << hex << "0x" << actual << " Should be " << target << endl;
		return 1;
	}
	return 0;
}

void printTxFifoStatus(PCIeMini_Arinc429_Tx *tx);
int checkRx1Ctrl(PCIeMini_Arinc429_Rx *rx1, UINT32 value);
int checkRx2Ctrl(PCIeMini_Arinc429_Rx *rx1, UINT32 value);
int checkTxCtrl(PCIeMini_Arinc429_Tx *tx, UINT32 value);
int checkTxFifoEmpty(PCIeMini_Arinc429_Tx *tx, char *msg);
int checkTxFifoEmpty(PCIeMini_Arinc429_Tx *tx);
int checkGPIOvalue(int chipNbr, UINT32 value);
void displayIrqStatus(int cs);

int staticIrqTest();
int rxIrqTest(int chipNbr, int verbose);
void isr429(void *brd, UINT32 data);

time_t calculateDelayMs(struct timespec *stime);

#endif
