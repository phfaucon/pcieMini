//
// Copyright (c) 2020 Alphi Technology Corporation, Inc.  All Rights Reserved
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
/** @file PCIe_Mini_AVIO.cpp
 * @brief Implementation of the PCIeMini_AVIO board class.
 */

 // Maintenance Log
//---------------------------------------------------------------------
// v1.0		2/23/2020	phf	Written
//---------------------------------------------------------------------

#include <stdio.h>
#include "PCIeMini_DTMS.h"
#include "DtmsTest.h"
#include "AvioAddressSpace.h"
#include "hostbug.h"

DtmsTest* DtmsTest::testInstance = NULL;
static int irqNbr;

void int_function(void* p_uio_board)
{
	uint8_t val = 0;
	PCIeMini_DTMS* dut = (PCIeMini_DTMS*)p_uio_board;

	printf("Int!! %x\n", val & 0x3f);
	irqNbr++;
	dut->statusReg->clearIrqStatus(0xff);
	return;
}

int DtmsTest::dacSpiTest()
{
	DAC161S997 *dac = dut->dac;
	SpiOpenCore *spi = dac->spi;
	uint32_t errNbr = 0;
	uint32_t err = 0;

	uint32_t ctrl = spi->getSpiControl();
	for (int i = 0; i<0x7f; i++) {
		uint32_t data = 0x2000 + i;
		spi->setSpiControl(data);
		uint32_t readback = spi->getSpiControl();
		if (readback != data) {
			if (err < 5)
				printf("Error: SPI Controller Control Register: wrote 0x%04x, read 0x%04x\n", data, readback);
			err++;
		}
	}
	spi->setSpiControl(ctrl);
	if (err == 0) {
		printf("Success: SPI Controller Control Register test passed.\n");
	}
	else {
		printf("Failed: SPI Controller Control Register test.\n");
		errNbr += err;
	}

	uint32_t divider = spi->getSpiDivider();
	err = 0;
	for (uint32_t i = 0; i<0x7f; i++) {
		//hi->spi_width(i);
		spi->setSpiDivider(i);
//		Sleep(1);
		uint32_t readback = spi->getSpiDivider();
		if (i != readback) {
			if (err < 5)
				printf("error: SPI Controller Divider register wrote 0x%04x, read 0x%04x\n", i, spi->getSpiDivider());
			err++;
		}
	}
	spi->setSpiDivider(divider);
	if (err == 0) {
		printf("Success: SPI Controller Divider register test passed.\n");
	}
	else {
		printf("Failed: SPI Controller Divider Register test.\n");
		errNbr += err;
	}

	return errNbr;
}

#if 0
int DtmsTest::check8429ThresholdRegister(uint32_t newVal, uint32_t oldVal)
{
	uint32_t readBack;
	int errNbr = 0;

	readBack = dut->hi8429->setHoltThresholdReg(newVal);
	if (readBack == oldVal) {
		printf("Success: ");
	}
	else {
		printf("Failure: ");
		errNbr++;
	}
    printf("read 0x%08x, wrote 0x%08x, spi_control=0x%04x\n", readBack, newVal, dut->hi8429->spi->getSpiControl());
    return errNbr;
}

int DtmsTest::check8429ControlRegisterBIT(bool verbose, uint32_t newVal, uint32_t oldVal)
{
	uint32_t readBack;
	int errNbr = 0;

	readBack = dut->hi8429->setHoltConfigReg(0x100 + newVal);
	uint8_t sense = dut->hi8429->getHoltSenseReg();
	if ((readBack == 0x100 + oldVal) && sense == newVal) {
		if (!verbose) {
			return errNbr;
		}
		printf("Success: ");
	}
	else {
		printf("Failure: ");
		errNbr++;
	}
	printf("Config Reg = 0x%04x, Readback config reg 0x%04x, Sense inputs register 0x%04x,\n", 0x100 + newVal, readBack, sense);
    return errNbr;
}
#endif

int DtmsTest::mainTestLoop()
{
	uint32_t sys_id;
	uint32_t timestamp;
	int brdNbr = 0;
	DtmsCtrlReg *ctrlReg;
	int errNbr = 0;
	PCIeMini_status st;

	dut->open(brdNbr);
	ctrlReg = dut->ctrlReg;
    printf("CtrlReg: 0x%04x\n", ctrlReg->getData());

	sys_id = dut->getFpgaID();
	printf("System ID %x\n", sys_id);

	timestamp = (uint32_t)dut->getFpgaTimeStamp();
	printf("Timestamp %x\n", timestamp);
	time_t ts = (time_t)timestamp;
	char buff[20];
	strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
    printf("%s\n", buff);

//    AvioAddressSpace *CurrentAs = new AvioAddressSpace("BAR #2", dut->bar2.Address, dut->bar2.Length);
//	hostbug_main((AddrSpace*)CurrentAs);

    // DAC161S997 Test
    printf("\nDAC161S997 Test:\n");
    printf("------------------\n");
    dacSpiTest();
    dut->resetDac();
    Sleep(2);
    dut->dac->spi_init();
    uint32_t rb = dut->dac->getStatus();
    printf("Status: 0x%02x\n", rb);
    dut->dac->printStatus(rb);
    rb = dut->dac->SendNopCmd();
    printf("Nop returned: 0x%06x\n", rb);
    rb = dut->dac->SetErrorConfig(0x014f);
    printf("SetErrorConfig returned: 0x%06x\n", rb);
     {
		rb = dut->dac->SetDacCode(0x0000);
		printf("SetDacCode returned: 0x%06x\n", rb);
		rb = dut->dac->SetDacCode(0x2000);
		printf("SetDacCode returned: 0x%06x\n", rb);
		rb = dut->dac->SetDacCode(0x6000);
		printf("SetDacCode returned: 0x%06x\n", rb);
		rb = dut->dac->SetDacCode(0xa000);
		printf("SetDacCode returned: 0x%06x\n", rb);
		rb = dut->dac->SetDacCode(0xe000);
		printf("SetDacCode returned: 0x%06x\n", rb);
		rb = dut->dac->SetDacCode(0xffff);
		printf("SetDacCode returned: 0x%06x\n", rb);
		rb = dut->dac->SetDacCode(0x8000);
		printf("SetDacCode returned: 0x%06x\n", rb);
	    rb = dut->dac->getStatus();
	    dut->dac->printStatus(rb);
    }
    rb = dut->dac->getStatus();
    printf("Status: 0x%02x\n", rb);
    dut->dac->printStatus(rb);

    // LTC2984 Test
    printf("\nLTC2984 Test:\n");
    printf("------------------\n");
    dut->resetDtms();
    Sleep(300);
    while (dut->dtms->isLTC2984Busy());

    st = LTC2984AdcSetup();
    if (st == ERRCODE_NO_ERROR) {
    	printf("LTC2984 configured successfully!\n");
    }
    else {
    	printf("Error: LTC2984 configuration failed!\n");
    	errNbr++;
    }
	ctrlReg = dut->ctrlReg;
    printf("CtrlReg: 0x%04x\n", ctrlReg->getData());

    uint32_t checkvalue = 0xfedcba98;
    dut->dtms->write_four_bytes(0x250, checkvalue);
    rb = dut->dtms->read_four_bytes(0x250);
    uint32_t rb2 = 0;
    for (int i=0; i<4; i++)
    	rb2 = (rb2 << 8) | dut->dtms->read_byte(0x250 + i);
    printf("DTMS SPI check: wrote 0x%04x, 4-byte read: 0x%08x, 1-byte reads: 0x%08x\n\n", checkvalue, rb, rb2);

    printf("Status register before enabling Irq: 0x%04x edges: 0x%04x, status: 0x%04x\n", dut->statusReg->getData(), dut->statusReg->getEdgeCapture(), dut->statusReg->getIrqStatus());
    dut->enableLtc2984Irq(int_function, 0);
    irqNbr = 0;
    Sleep(20);
    printf("Status register before enabling Irq: 0x%04x edges: 0x%04x, status: 0x%04x\n", dut->statusReg->getData(), dut->statusReg->getEdgeCapture(), dut->statusReg->getIrqStatus());

	LTC2984AdcMeasurementLoop();

    printf("Status register after test - data: 0x%04x edges: 0x%04x, status: 0x%04x\n", dut->statusReg->getData(), dut->statusReg->getEdgeCapture(), dut->statusReg->getIrqStatus());
    printf("CRA after test - irq status: 0x%04x, irq enable: 0x%04x irq number: %d\n", dut->cra->getIrqStatus(), dut->cra->getIrqEnableMask(), irqNbr);

    Sleep(100);
    dut->disableLtc2984Irq();

    /* testing relays */
    printf("\ntesting relays\n");
    dut->setOutputDirection(0x0f);
    for (int i = 0; i < 100; i++)
    {
    	int index = i % 8;
    	dut->setOutputs(0xff);
    	dut->setRelays(0x03);
    	dut->setOcOutputs(0x03);

    	//
    	switch (index)
    	{
    	case 0:
    		dut->setOcOutputs(0x02);
    		break;
    	case 1:
    		dut->setOcOutputs(0x01);
    		break;
    	case 2:
    		dut->setOutputs(0x0e);
    		break;
    	case 3:
    		dut->setOutputs(0x0d);
    		break;
    	case 4:
    		dut->setOutputs(0x0b);
    		break;
    	case 5:
    		dut->setOutputs(0x07);
    		break;
    	case 6:
    		dut->setRelays(0x02);
    		break;
    	case 7:
    		dut->setRelays(0x01);
    		break;
    	}
    	Sleep(300);
    }

    /* testing IO loopback
     *
     */
    dut->setOutputDirection(0x05);
    for (int i = 0; i < 100; i++)
    {
    	uint8_t out = (i&1) ? 0x0c : 0x03;
    	dut->setOutputs(out);
    	Sleep(300);
    	printf("inputs: 0x%x\n", dut->getInputs());
    }

    for (int i = 0; i < 100; i++)
    {
     	dut->setPower5v(i & 1);
    	Sleep(300);
    }


	dut->close();

	return errNbr;

}

int main()
{
	DtmsTest::getInstance()->mainTestLoop();
	return 0;
}
