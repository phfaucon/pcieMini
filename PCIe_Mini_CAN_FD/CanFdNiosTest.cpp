#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include "CanFdNiosComm.h"
#include "CanFdTest.h"

using namespace std;



static bool irqReceived;
void CanNiosIntReceived(void* brd, UINT32 data)
{
	// acknowledge the interrupt
	irqReceived = true;

	Sleep(1);
}

int CanFdTest::niosObjectTest()
{
	int nbrErrors = 0;
	return nbrErrors;
}

int CanFdTest::canFdNiosTest()
{
	uint8_t chan;
	struct canfd_frame* cf = (struct canfd_frame*)malloc(sizeof(struct canfd_frame));

	// check that there is not interrupt already being requested
	uint16_t irqStatus = dut->cra->getIrqStatus();
	uint16_t irqEnable = dut->cra->getIrqEnableMask();
	if (irqStatus != 0 || irqEnable != 0) {
		cout << "Warning: while entering, interrupter not initialized: pcieIrqStatus = 0x"
			<< hex << irqStatus << ", pcieIrqEnable = 0x" << irqEnable << endl;
	}

	int nbrErrors = 0;
	bool	irqReceived = false;

	// allow the interrupts
	PCIeMini_status st = dut->hookInterruptServiceRoutine((MINIPCIE_INT_HANDLER)CanNiosIntReceived);
	if (st != WD_STATUS_SUCCESS)
		cout << "hookInterruptServiceRoutine failed, error #" << st << endl;

	dut->canNios->fifo_status();
	dut->canNios->ask_for_board_id();
	dut->canNios->fifo_status();
	dut->canNios->ask_for_timestamp();
	dut->canNios->fifo_status();

	dut->canNios->ask_for_CAN_ID(0);
	dut->canNios->fifo_status();
	dut->canNios->ask_for_CAN_ID(1);
	dut->canNios->fifo_status();
	dut->canNios->ask_for_CAN_ID(2);
	dut->canNios->fifo_status();
	dut->canNios->ask_for_CAN_ID(3);

	chan = 0;
	cf->can_id = 0x1234 | CAN_EFF_FLAG;
	cf->len = 4;
	cf->data[0] = 0x11;
	cf->data[1] = 0x22;
	cf->data[2] = 0x33;
	cf->data[3] = 0x44;
	dut->canNios->send_CAN_message(chan, cf);
	dut->canNios->fifo_status();

	chan = 1;
	cf->can_id = 0x2341 | CAN_EFF_FLAG;
	cf->len = 4;
	cf->data[0] = 0x22;
	cf->data[1] = 0x33;
	cf->data[2] = 0x44;
	cf->data[3] = 0x11;
	dut->canNios->send_CAN_message(chan, cf);
	dut->canNios->fifo_status();

	chan = 2;
	cf->can_id = 0x3412 | CAN_EFF_FLAG;
	cf->flags = CANFD_ESI;
	cf->len = 4;
	cf->data[0] = 0x33;
	cf->data[1] = 0x44;
	cf->data[2] = 0x11;
	cf->data[3] = 0x22;
	dut->canNios->send_CANFD_message(chan, cf);
	dut->canNios->fifo_status();

	chan = 3;
	cf->can_id = 0x4123 | CAN_EFF_FLAG;
	cf->flags = CANFD_BRS;
	cf->len = 4;
	cf->data[0] = 0x44;
	cf->data[1] = 0x11;
	cf->data[2] = 0x22;
	cf->data[3] = 0x33;
	dut->canNios->send_CANFD_message(chan, cf);
	dut->canNios->fifo_status();

	Sleep(100);
	irqStatus = dut->cra->getIrqStatus();
	dut->canNios->int_function();

	if (irqReceived) {
		cout << "Success, interrupt received" << endl;
	}
	else {
		cout << "Interrupt was not received." << endl;
		nbrErrors++;
	}

	st = dut->disableInterrupts();
	return nbrErrors;


}

