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
#include "PCIeMini_Synch.h"
#include "TestProgram.h"

class SyncTest : public TestProgram
{
public:
	PCIeMini_Synch* dut;

	inline SyncTest()
	{
		dut = new PCIeMini_Synch();
		gain1 = 0;
		gain2 = 0;
		gain3 = 0;
		for (int i = 0; i < spiDaNbrOfValues; i++) {
			synchRef[i] = 0;
			synchS1[i] = 0;
			synchS2[i] = 0;
			synchS3[i] = 0;
		}
	}

	int mainTest(int brdNbr);
	inline void setVerbose(int v) {
		dut->setVerbose(v);
	}

	int checkAngle(double angle, bool* bitFailed);

	int staticLoopbackTest();

	int dynamicLoopbackTest();
	int miscSoftwareVerifications();
	int checkAngleToRadConv(double rad);
	int checkAngleToGain(double test, int16_t gain1, int16_t gain2, int16_t gain3);

	static double normalizeAngle(double angle);
	static double getAngularError(double desired, double actual);

	int irqTest();

	// SPI DA test
	// we are sending sine waves on the DA converters and checking them with the synchro
	static const int spiDaNbrOfValues = 100;			///< number of values per sinewave cycle
	static const int spiDaStepTime = 1;				///< step time in millisecond: 63 ms cycle correspond to 16 Hz
#define degToRad(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)

	int16_t synchRef[spiDaNbrOfValues];		///< Reverence sine waves
	int16_t synchS1[spiDaNbrOfValues];					///< S1 sine waves
	int16_t synchS2[spiDaNbrOfValues];					///< S2 sine waves
	int16_t synchS3[spiDaNbrOfValues];					///< S3 sine waves
	double gain1;										///< S1 sine waves multiplier
	double gain2;										///< S2 sine waves multiplier
	double gain3;										///< S3 sine waves multiplier

	void spiDaInitSineTables(double angle);
	int tryAngle(double desired);

	int testSpiDa();
};