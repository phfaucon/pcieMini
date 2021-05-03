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
#include "PCIeMini_DTMS.h"
#include "TestProgram.h"


class DtmsTest : public TestProgram
{
public:
	PCIeMini_DTMS* dut;

	static DtmsTest* getInstance()
	{
		if (testInstance == NULL) {
			testInstance = new DtmsTest();
		}
		return testInstance;
	}


	int mainTestLoop();

	int dacSpiTest();
	int check8429ThresholdRegister(uint32_t newVal, uint32_t oldVal);
	int check8429ControlRegisterBIT(bool verbose, uint32_t newVal, uint32_t oldVal);

	PCIeMini_status LTC2984Setup();
	PCIeMini_status LTC2984AdcSetup();
	PCIeMini_status configure_channels();
	PCIeMini_status configure_channels_as_adc();
	PCIeMini_status configure_global_parameters();
	void LTC2984MeasurementLoop();
	void LTC2984AdcMeasurementLoop();

private:
	inline DtmsTest()
	{
		dut = new PCIeMini_DTMS();
	}
	static DtmsTest* testInstance;

};
