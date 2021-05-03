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

// Maintenance Log
//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "Windows.h"
#include <time.h>
#include <string.h>

#include "escc_defines.h"
#include "PCIeMini_ESCC.h"


uint8_t cachedRegs[64];
 UartChannelConfig scc_config[2];

#define CHAN0 0
#define CHAN1 1


void print_rr0(uint8_t chan, uint8_t val)
{
	std::cout << "Ch " << chan << " RR0 - ";
	if(val & 0x80)
		std::cout << "Break ";
	if(val & 0x40)
		std::cout << "EOM ";
	if(~(val & 0x20))
		std::cout << "CTS ";
	if(val & 0x10)
		std::cout << "Hunt ";
	if(val & 0x08)
		std::cout << "DCD-IE ";
	if(val & 0x04)
		std::cout << "TX-MT ";
	if(val & 0x02)
		std::cout << "Zero ";
	if(val & 0x01)
		std::cout << "RX Avail ";
		std::cout << std::endl;
}

void print_brg(PCIeMini_ESCC* brd)
{
	uint8_t regl,regh;

	brd->sccRegisterRead(CHAN0, R12_BRGL, &regl);
	brd->sccRegisterRead(CHAN0, R13_BRGH, &regh);
	std::cout << "A BRG " << regh << ", " << regl << std::endl;
	brd->sccRegisterRead(CHAN1, R12_BRGL, &regl);
	brd->sccRegisterRead(CHAN1, R13_BRGH, &regh);
	std::cout << "B BRG " << regh << ", " << regl << std::endl;
}

int quickTest(PCIeMini_ESCC* brd)
{
	uint8_t patt, res, r2_vec;
	int i, fail;

	fail = 0;

	brd->sccRegisterRead(0, R2_IVR, &r2_vec);
	for(i=0; i<8; i++){
		patt = 1 << i;
		brd->sccRegisterWrite(0, R2_IVR, patt);
		brd->sccRegisterRead(0, R0_PTR, &res);
		brd->sccRegisterRead(1, R0_PTR, &res);
		brd->sccRegisterRead(0, R2_IVR, &res);
		if(res != patt){
			std::cout << "Quick check fail chan A IVR rec " << res << " expect " << patt << std::endl;
			fail = 1;
		}
		brd->sccRegisterRead(0, R0_PTR, &res);
		brd->sccRegisterRead(1, R0_PTR, &res);
		brd->sccRegisterRead(0, R2_IVR, &res);
		if((res&0xf1) != (patt&0xf1)){
			std::cout << "Quick check fail chan B IVR rec " << (res & 0xf1) << " expect " << (patt & 0xf1) << std::endl;
			fail = 1;
		}

	}
	brd->sccRegisterWrite(0, R2_IVR, r2_vec);

	return fail;
}

void set_serialBuffers(PCIeMini_ESCC *brd, int chan, UINT16 val)
{
	brd->getScc(chan)->set_serialBuffers(val);
	val = brd->getScc(chan)->buffer->getBuffer();
}

UINT8 brgDivLow_bob(int SCC_clocksource, int div, int baudRate)
{
	return (UINT8)((SCC_clocksource/(2*div*baudRate)-2) & 0xff);
}

int SCC_clocksource = 16000000;		///< PCLK frequency used for baud rate calculations
int RTxcFrequency = 14745600;       ///< TxC input, should be 14.745600 MHz. Used to calculate the value for the divider register.

uint32_t	scc_chip_reset(PCIeMini_ESCC* brd, int chan)
{
	return brd->getScc(chan)->reset();
}

void	scc_chan_reset(PCIeMini_ESCC* brd, int chan)
{
	return brd->getScc(chan)->resetChannel();
}

uint8_t Reg3 = Rx8;
uint8_t Reg4 = X16CLK;
uint8_t Reg5 = Tx8;
uint8_t divider;
uint8_t Reg14;

int init_scc_chan(PCIeMini_ESCC* brd, int chan, UartChannelConfig *config)
{
	scc_config[chan].baudRate = 115200;
	scc_config[chan].dataBits = 8;
	scc_config[chan].eolChar = '\n';
	scc_config[chan].parity = 1;
	scc_config[chan].parityEnable = 0;
	scc_config[chan].stopBits = 1;
	scc_config[chan].bufferConfig = LTC2872::RS_422_buffers;
	scc_config[chan].useRxFifo = true;
	scc_config[chan].useTxFifo = true;

	// receive data bit configuration
	switch (scc_config[chan].dataBits) {
	case 5: Reg3 = Rx5;
		break;
	case 6: Reg3 = Rx6;
		break;
	case 7: Reg3 = Rx7;
		break;
	default: Reg3 = Rx8;
		break;
	}

	// transmit data bit configuration
	switch (scc_config[chan].dataBits) {
	case 5: Reg5 = Tx5;
		break;
	case 6: Reg5 = Tx6;
		break;
	case 7: Reg5 = Tx7;
		break;
	default: Reg5 = Tx8;
		break;
	}

	if (scc_config[chan].parityEnable)
		Reg4 |= PAR_ENA;
	if (!scc_config[chan].parity)
		Reg4 |= PAR_EVEN;
	if (scc_config[chan].stopBits == 2)
		Reg4 |= SB2;
	else
		Reg4 |= SB1;

	if (scc_config[chan].baudRate <= 38400) {
		divider = brgDivLow_bob(SCC_clocksource, 16, scc_config[chan].baudRate);
		Reg14 = DTRREQ | BRSRC | BRENABL;		// BRSRC indicates that the source of the baud rate generator is PCLK
	}
	else {
		divider = brgDivLow_bob(RTxcFrequency, 16, scc_config[chan].baudRate);
		Reg14 = DTRREQ | BRENABL;				// BRSRC not asserted indicates that the source of the baud rate generator is the RTxC input
	}


	uint8_t z85233_initList[] =
	{
		4,		Reg4,				/* 0  ASYNC, NO PARITY, 1 STOP BIT */
		6,		0,					/* 1  no sync char */
		7,		0,					/* 2  no sync char */
		2,		0,					/* 3  vector */
		1,		0,
		3,		Reg3,				/* 5  8 BIT, Rx disabled */
		5,		(uint8_t)(Reg5 | DTR),			/* 6  8 BIT, Tx disabled */
		9,		0,					/* 7  interrupts disabled */
		10,		NRZ,				/* 8  Encoding */
		11,		(uint8_t)(RCBR | TCBR),			/* 9  Clocks */
		13,		0,					// a
		12,		divider,            // b
		14,		Reg14,				/* c baud rate enabled, DMA signals enabled */
		15,		0,
		7 + 16,	0,					/* 14 automatic RTS de-assert */
		9,		0,
		3,		(uint8_t)(Reg3 | RxENABLE),
		5,		(uint8_t)(Reg5 | TxENABLE),      // 17
		255
	};

	scc_chan_reset(brd, chan);

	// set UART speed
	brd->getScc(chan)->channelLoad(z85233_initList);
	brd->getScc(chan)->channelLoad(z85233_initList);

	// set other configurations
	set_serialBuffers(brd, chan, scc_config[chan].bufferConfig);

	if (scc_config[chan].useRxFifo)
		brd->enableRxDma(chan);
	else
		brd->disableRxDma(chan);

	if (scc_config[chan].useTxFifo)
		brd->enableTxDma(chan);
	else
		brd->disableTxDma(chan);

	return 0;
}

int init_scc_chan(PCIeMini_ESCC* brd, int chan)
{


    scc_config[chan].baudRate = 115200;
    scc_config[chan].dataBits = 8;
    scc_config[chan].eolChar = '\n';
    scc_config[chan].parity = 1;
    scc_config[chan].parityEnable = 0;
    scc_config[chan].stopBits = 1;
    scc_config[chan].bufferConfig = LTC2872::RS_422_buffers ;
    scc_config[chan].useRxFifo = true;
    scc_config[chan].useTxFifo = true;


	// receive data bit configuration
	switch (scc_config[chan].dataBits) {
	case 5: Reg3 = Rx5;
		break;
	case 6: Reg3 = Rx6;
		break;
	case 7: Reg3 = Rx7;
		break;
	default: Reg3 = Rx8;
		break;
	}

	// transmit data bit configuration
	switch (scc_config[chan].dataBits) {
	case 5: Reg5 = Tx5;
		break;
	case 6: Reg5 = Tx6;
		break;
	case 7: Reg5 = Tx7;
		break;
	default: Reg5 = Tx8;
		break;
	}

	if (scc_config[chan].parityEnable)
		Reg4 |= PAR_ENA;
	if (!scc_config[chan].parity)
		Reg4 |= PAR_EVEN;
	if (scc_config[chan].stopBits == 2)
		Reg4 |= SB2;
	else
		Reg4 |= SB1;

	if (scc_config[chan].baudRate <= 38400) {
		divider = brgDivLow_bob(SCC_clocksource, 16, scc_config[chan].baudRate);
		Reg14 = DTRREQ|BRSRC|BRENABL;		// BRSRC indicates that the source of the baud rate generator is PCLK
	}
	else {
		divider = brgDivLow_bob(RTxcFrequency, 16, scc_config[chan].baudRate);
		Reg14 = DTRREQ|BRENABL;				// BRSRC not asserted indicates that the source of the baud rate generator is the RTxC input
	}


	uint8_t z85233_initList[]=
	{
		4,		Reg4,				/* 0  ASYNC, NO PARITY, 1 STOP BIT */
		6,		0,					/* 1  no sync char */
		7,		0,					/* 2  no sync char */
		2,		0,					/* 3  vector */
		1,		0,
		3,		Reg3,				/* 5  8 BIT, Rx disabled */
		5,		(uint8_t)(Reg5|DTR),			/* 6  8 BIT, Tx disabled */
		9,		0,					/* 7  interrupts disabled */
		10,		NRZ,				/* 8  Encoding */
		11,		RCBR|TCBR,			/* 9  Clocks */
		13,		0,					// a
		12,		divider,            // b
		14,		Reg14,				/* c baud rate enabled, DMA signals enabled */
		15,		0,
		7+16,	0,					/* 14 automatic RTS de-assert */
		9,		0,
		3,		(uint8_t)(Reg3|RxENABLE),
		5,		(uint8_t)(Reg5|TxENABLE),      // 17
		255
	};

	scc_chan_reset(brd, chan);

	// set UART speed
	brd->getScc(chan)->channelLoad(z85233_initList);
	brd->getScc(chan)->channelLoad(z85233_initList);

	// set other configurations
	set_serialBuffers(brd, chan, scc_config[chan].bufferConfig);

	if (scc_config[chan].useRxFifo)
		brd->enableRxDma(chan);
	else
		brd->disableRxDma(chan);

	if (scc_config[chan].useTxFifo)
		brd->enableTxDma(chan);
	else
		brd->disableTxDma(chan);

	return 0;
}

