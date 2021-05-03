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
/** @file PCIeMini_Synch_test.cpp : This file contains the 'main' function. Program execution begins and ends there. */
//
// Maintenance Log
//---------------------------------------------------------------------
// v1.0		7/23/2020	phf	Written
//---------------------------------------------------------------------
/** @file PCIeMini_Synch_test.cpp : This file contains the 'main' function for the self test. Program execution begins and ends there.
 */

#include <iostream>
#include "SyncTest.h"

using namespace std;

int main(int argc, char* argv[])
{
	int verbose = 0;
	int brdNbr = 0;
	int executeLoopback = 1;
	int i;
	SyncTest tst;

	for (i = 1; i < argc; i++) {
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
		case '?':
			cout << endl << "Possible options: <brd nbr>, v: verbose, x: external loopback disabled" << endl;
			cout << "	e <tx#> <rx#>: ext. loopback between the transmitter tx and receiver rx." << endl;
			cout << "for example: 'arinc429test 0 x v' means: board #0, verbose, no external loopback" << endl;
			cout << "    'arinc429test v e 0 0 e 1 1 e 1 2' means: verbose,  external loopback tx0 to rx0, tx1 to rx1, tx1 to rx2" << endl << endl;
			exit(0);
		}
	}

	cout << "Starting test" << endl;
	cout << "=============" << endl;
	tst.setVerbose(verbose);

	tst.mainTest(brdNbr);


	return 0;

}