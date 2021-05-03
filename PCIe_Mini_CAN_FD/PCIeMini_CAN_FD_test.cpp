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
#include <conio.h>
#include "CanFdTest.h"

using namespace std;

int main(int argc, char* argv[])
{
	int verbose = 0;
	int brdNbr = 0;
	bool executeLoopback = false;
	int i;
	CanFdTest *tst = CanFdTest::getInstance();

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
		case 't':
			executeLoopback = true;
			break;
		case '?':
			std::cout << endl << "Possible options: <brd nbr>, v: verbose, t: starts the test" << endl;
			exit(0);
		}
	}

	std::cout << "Starting test" << endl;
	std::cout << "=============" << endl;
	tst->verbose = verbose; 
	tst->dut->verbose = verbose;

	tst->mainTest(brdNbr, executeLoopback);


	return 0;

}

/*
struct TxMsg {
	uint32_t txRate;
	TCAN4x5x_MCAN_TX_Header msgHeader;
	int numBytes;
	uint8_t dataPayload[100];
	uint8_t port;
};
TxMsg txData;
*/

int CanFdTest::sendCanMesg()
{
	uint8_t portNumber;
	TCAN4x5x_MCAN_TX_Header header = { 0 };			// Remember to initialize to 0, or you'll get random garbage!
	uint32_t rate;

	uint8_t data[64] = { 0 };						// Define the data payload
	header.DLCode = MCAN_DLC_4B;					// Set the DLC to be equal to or less than the data payload (it is ok to pass a 64 byte data array into the WriteTXFIFO function if your DLC is 8 bytes, only the first 8 bytes will be read)
	header.ID = 280;								// Set the ID
	header.EFC = 0;
	header.MM = 0;
	header.RTR = 0;
	header.XTD = 0;									// We are not using an extended ID in this example
	header.ESI = 0;									// Error state indicator

	if (isCanFd) {
		header.FDF = 1;									// CAN FD frame enabled
		header.BRS = 1;									// Bit rate switch enabled
	}
	else {
		header.FDF = 0;									// CAN FD frame disabled
		header.BRS = 0;									// Bit rate switch disabled
	}

	int j, k;
	char c;
	printf("rate in mS (0 for once): ");
	scanf("%d", &j);
	rate = j;

	printf("id (hexadecimal): 0x");
	scanf("%x", &j);
	header.ID = j;

	printf("port (0-3): ");
	scanf("%d", &j);
	if (j > 3 || j < 0 ) j = 0;

	cyclical[j] = rate;

	portNumber = j;
	while (_kbhit())
		_getch();   // Empties buffer
	printf("will this be a remote transmit message ? ( N/y)\n");
	c = _getch();
	if (c == 'y' || c == 'Y')
		header.RTR = 1;
	else
		header.RTR = 0;

	while (_kbhit())
		_getch();   // Empties buffer

	printf("will this be an extended frame message ? ( Y/n)\n");
	c = _getch();
	if (c != 'y' && c != 'Y')
		header.XTD = 1;

	printf("number of bytes (0..8): ");
	scanf("%x", &j);
	if (j > 8) j = 8;
	if (j < 0) j = 0;
	header.DLCode = j;
	if (!header.RTR)
		for (int i = 1; i <= j; i++) {
			printf("byte #%d (hexadecimal): 0x", i);
			scanf("%x", &k);
			data[i - 1] = k;
		}
	printf("Msg sent:");
	printTxMsg(&header, data);
	dut->can[portNumber]->MCAN_WriteTXBuffer(0, &header, data);	
	dut->can[portNumber]->can->AHB_WRITE_32(REG_MCAN_TXBAR, 1);
	/*if (txData.txRate == 0)
		CAN_write(&canDevice, &txData);

	else
		CAN_writeCyclic(&canDevice, &txData);*/
	return 0;
}

void CanFdTest::printChannelStatus(uint8_t channelNbr)
{
	printf("________________________________\n");
	printf("Channel #%d status\n", channelNbr);
	printf("________________________________\n");
	dut->can[channelNbr]->printStatusRegister();
	dut->can[channelNbr]->printControlRegister();
	// check SPI access
	printf("Testing the SPI access:\n");
	int errNbr = testSpiRead32(channelNbr);
	if (errNbr == 0) {
		checkSpiErrorBit(dut->can[channelNbr], 1);
	}
}

int CanFdTest::mainTest(int brdNbr, bool executeLoopback)
{
	int loopCntr = 0;
	int totalError = 0;
	int spiError;
	bool runProgram = true;
	int errorCode;
	char c;
	uint8_t receiveBuffer[1024];
	int bytesRead;
	clock_t currentTime, oneSecondLater;
	int outputToScreen;
	int receiveHigh = 0, transmitHigh = 0;
	UINT32		status_high;
	UINT32		status_low;
	int i, j, k;
	char buff[20];
	time_t ts;
	uint32_t id;

	dut->setVerbose(0);
	PCIeMini_status st = dut->open(brdNbr);

	std::cout << "Opening the PCIeMini_Sync: " << getAlphiErrorMsg(st) << endl;
	if (st != ERRCODE_NO_ERROR) {
		std::cout << "Exiting: Press <Enter> to exit." << endl;
		getc(stdin);
		exit(0);
	}

	id = dut->getFpgaID();
	std::cout << "FPGA ID: 0x0" << hex << id << endl;
	if (id == 0xffffffff || id == 0) {
		std::cout << "Cannot read the board!" << endl;
		std::cout << "Exiting: Press <Enter> to exit." << endl;
		getc(stdin);
		exit(0);
	}

	ts = dut->getFpgaTimeStamp();
	std::cout << "FPGA Time Stamp: 0x" << ts << endl;
	strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
	printf("%s\n", buff);

	std::cout << endl << "---------------------------------" << endl;
	std::cout << "PCIeMini-CAN_FD Confidence Test" << endl;
	std::cout << "Using board number " << brdNbr << endl;
	if (verbose) std::cout << "verbose mode " << endl;
	std::cout << "---------------------------------" << endl;

	dut->reset();
	Sleep(100);

	// initialize the CAN chips
	for (int i = 0; i < dut->nbrOfCanInterfaces; i++) {
		Init_CAN(dut->can[i], verbose);
		dut->input0->resetIrq();
		if (i == 3) {
			dut->can[i]->setCanTermination(true);
			if (verbose) printf("Termination is on!\n");
		}
		else {
			dut->can[i]->setCanTermination(false);
			if (verbose) printf("Termination is off!\n");
		}
	}

	printf("press <enter> for command menu\n");
	while (runProgram)
	{
		// check for user input
		if (_kbhit() || executeLoopback) {
			// starts the loopback test as needed
			if (executeLoopback) {
				executeLoopback = false;
				c = '6';
			}
			else {
				// enter new message
				_getch();   // Empties buffer
				printf("Command input mode: \n");
				printf("0: send message\n");
				printf("1: cyclical messages\n");
				printf("2: on-line the board\n");
				printf("3: get status\n");
				printf("4: set baud rate\n");
				//			printf("5: wipe firmware\n");
				printf("6: quickTest\n");
				printf("t: update terminations\n");
				printf("v: toggle verbose mode\n");
				printf("x: exit the application\n");
				printf("any other character to display received messages\n");
				printf("Enter command: >");

				c = _getch();
				printf("%c\n", c);
			}
			switch (c) {
				// exit the program
			case 'x':
			case 'X':
				printf("exiting program and clearing cyclic messages...\n");
				runProgram = FALSE;
			case 'v':
			case 'V':
				verbose = !verbose;
				dut->verbose = verbose;
				printf("verbose is now %s\n", verbose ? "on" : "off");
				break;
			case 't':
			case 'T':
				for (int chnNbr = 0; chnNbr < dut->nbrOfCanInterfaces; chnNbr++) {
					printf("Channel #%d termination is %s.\n", chnNbr, dut->can[chnNbr]->isCanTerminationEnabled()?"on":"off");
				}
				printf("Enter the channel number you want to change or return to skip ?\n");
				c = _getch(); 
				printf("%c\n", c);
				i = c - '0';
				if (i < 0 || i > dut->nbrOfCanInterfaces) {
					break;
				}
				else {
					dut->can[i]->setCanTermination(!dut->can[i]->isCanTerminationEnabled());
				}
				for (int chnNbr = 0; chnNbr < dut->nbrOfCanInterfaces; chnNbr++) {
					printf("Channel #%d termination is %s.\n", chnNbr, dut->can[chnNbr]->isCanTerminationEnabled() ? "on" : "off");
				}
				break;
			case '1':		 // remove all cyclic commands
				for (int chnNbr = 0; chnNbr < dut->nbrOfCanInterfaces; chnNbr++) {
					if (cyclical[chnNbr] == 0) continue;
					printf("Channel #%d every %d ms\n", chnNbr, cyclical[chnNbr]);
				}
				printf("Do you want to remove the cyclical messages ? ( y/N)\n");
				c = _getch();
				if (c == 'y' || c == 'Y') {
					for (int chnNbr = 0; chnNbr < dut->nbrOfCanInterfaces; chnNbr++) {
						cyclical[chnNbr] = 0;
					}
				}

				break; 
/*
			case '2':
				errorCode = CAN_onLine(&canDevice);
				break;
*/
			case '3':
//				errorCode = CAN_status(&canDevice);
				printf("Enter the channel number you want to display: ");
				c = _getch();
				printf("%c\n", c);
				i = c - '0';
				id = dut->getFpgaID();
				std::cout << "FPGA ID: 0x0" << hex << id << endl;
				if (id == 0xffffffff || id == 0) {
					std::cout << "Cannot read the board!" << endl;
					std::cout << "Exiting: Press <Enter> to exit." << endl;
					getc(stdin);
					exit(0);
				}

				ts = dut->getFpgaTimeStamp();
				std::cout << "FPGA Time Stamp: 0x" << ts << endl;
				char buff[20];
				strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
				printf("%s\n", buff);
				if (i < 0 || i > dut->nbrOfCanInterfaces) {
					for (int chnNbr = 0; chnNbr < dut->nbrOfCanInterfaces; chnNbr++) {
						printChannelStatus(chnNbr);
					}
				}
				else {
					printChannelStatus(i);
				}
				break;
			case '4':
				while (_kbhit())
					_getch();   // Empties buffer
				for (int i = 0; i < NOMINAL_RATE_PRESETS; i++)
					printf("%d: %s\n", i, nominalBaudRates[i]);
				printf("Please enter the number of the nominal baud rate you want:\n");
				scanf("%d", &i);
				nominalSpeed = (MCAN_Nominal_Speed)i;

				for (int i = 0; i < DATA_RATE_PRESETS; i++)
					printf("%d: %s\n", i, dataBaudRates[i]);
				printf("Please enter the number of the data baud rate you want:\n");
				scanf("%d", &i);
				dataSpeed = (MCAN_Data_Speed)i;

				printf("Do you want to allow CAN-FD ? ( y/N)\n");
				c = _getch();
				if (c == 'y' || c == 'Y') {
					isCanFd = true;
				}
				else {
					isCanFd = false;
				}

				for (int i = 0; i < dut->nbrOfCanInterfaces; i++) {
					Init_CAN(dut->can[i]);
				}
				while (_kbhit())
					_getch();   // clear the last line feed on linux

				break;
				// run the default test messages
			case '6':
				while (_kbhit())
					_getch();   // Empties buffer
				printf("Starting the loopback test. The screen should be update every 10 seconds or less\n");
				printf("Press any key to exit:\n");
				totalError = 0;
				loopCntr = 0;
				while (!_kbhit()) {
					loopCntr++;
					if (testTiLib(10000)) {
						totalError++;
						ts = time(&ts);
						strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
						printf("%s - ", buff);

						printf("Test #%d failed: %d failure%s out of %d loops\n", loopCntr, totalError, (totalError<2)?"":"s", loopCntr);
					}
					else {
						ts = time(&ts);
						strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
						printf("%s - ", buff);
						printf("Test #%d passed: %d failure%s out of %d loops\n", loopCntr, totalError, (totalError < 2) ? "" : "s", loopCntr);
					}
				}
				break;

				// enter new message
			case '0':
				while (_kbhit())
					_getch();   // Empties buffer
				sendCanMesg();
				break;
			case 'P':
			case 'p':
				while (_kbhit())
					_getch();   // Empties buffer
				testPCIeSpeed();
				break;
			}
		}
		Sleep(1);
		// check for incoming messages
		ULONGLONG now = GetTickCount64();
		for (int chnNbr = 0; chnNbr < dut->nbrOfCanInterfaces; chnNbr++) {
			TCAN4550* can = dut->can[chnNbr];
			uint8_t numBytes = 0;

			while (!isRxFifo0Empty(can->can)) {
				uint8_t dataPayload[64] = { 0 };
				TCAN4x5x_MCAN_RX_Header MsgHeader = { 0 };		// Initialize to 0 or you'll get garbage
				numBytes = can->MCAN_ReadNextFIFO(RXFIFO0, &MsgHeader, dataPayload);	// This will read the next element in the RX FIFO 0
				printf("Channel #%d: ", chnNbr);
				printRxMsg(&MsgHeader, numBytes, dataPayload);
			}
			// check for outgoing messages
			if (cyclical[chnNbr] == 0) continue;
			int m = now % cyclical[chnNbr];
			if (m == 0) {
				dut->can[chnNbr]->can->AHB_WRITE_32(REG_MCAN_TXBAR, 1);
			}
		}
	}

	dut->disableInterrupts();
	dut->close();

	return 0;
	 
}