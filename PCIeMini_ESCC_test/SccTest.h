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
// v1.0		7/23/2020	phf	Written
//---------------------------------------------------------------------

#pragma once

#include "stdint.h"
#include "PCIeMini_ESCC.h"
#include "TestProgram.h"

class SccTest : public TestProgram
{
public:
	static const int READ_TIMEOUT = 100;
	static const int MAX_PACKET_LENGTH = 100;
	char packet[MAX_PACKET_LENGTH];
	int packetLength;
	PCIeMini_ESCC* dut;

	inline SccTest() :
		packetLength(0)
	{
		dut = new PCIeMini_ESCC();
		memset(packet, 0, sizeof packet);
	}

	int mainTest(int brdNbr);
	void setVerbose(int verbose);

	int loopbackTest(int trmtChannel, int rcvChannel, int nbrChar, int trmtDMA, int rcvDMA, bool useRs232 = false);
	int syncLoopbackTest(int trmtChannel, int rcvChannel, int nbrPackets, int baudRate, int useFifo);
	int isPacketValid(char* packetIn, int length);

	int checkFIFO(int fifoNum, int nbrChar);

	inline SccChannel* getScc(int nbr) {
		return dut->getScc(nbr);
	}
	int checkFifoEmpty(SccChannel* scc, const char* s);
	int TestSerialFIFO();
	int checkIrqVectorRegister();
	int check_ab(int verbose);
	int checkFifo(int sccNum, int verbose);
	int TestSerialSCCinterrupts(int chNbr, int verbose);

};