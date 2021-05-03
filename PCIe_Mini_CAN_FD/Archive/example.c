#include <stdio.h>
#ifdef linux
#include "TypeDefinitions.h"
#include "libusbLin.h"
#define _getch() getchar()
#define GetLastError() errno
#elif WIN32
// Include Windows headers for type definitions
#include <windows.h>
#include <conio.h>
#include "libusbWin.h"
#endif // lin/win

#include <errno.h>
#include "can_api.h"
#include "firmwareWipe.h"
#include "formulas.h"

#ifdef linux
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h> //for exit()
#include <pthread.h> // for threads

int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;


	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}
#endif

#ifdef linux

// Simulate Windows Sleep
void Sleep(int ms) 
{
	usleep(ms*1000); //convert to microseconds
	return;
}
#else
#	include <process.h> 
#endif


/*Function definitions */

int StartReadCANThread(int threadType);
/*global variables */


CAN_CONTROLLER_DESC canDevice;
// run means don't die, pause means don't print right now
int outputToScreen, runReceiveThread=0, pauseReceiveThread=0, runReceiveTestThread=0;
FILE *fp;

int interruptsReceived;
int errInTest;
int receiveHigh=0, transmitHigh=0;
UINT32		status_high;
UINT32		status_low;

int writeToScreen;


void printToScreen(stRX_MESSAGE * msgBuff)
{
	int lenToDisplay=0,j=0;

	printf("%d\t", (msgBuff->u32TimeStamp));
	printf("0x%08x\t", msgBuff->u32Id);
	printf("Port #%d\t", msgBuff->u8CANPort);
	if(EXTRACT_RTR(msgBuff->u32IERReg))
		printf("RTR\t");
	else 
		printf("---\t");
	printf("%d\t", EXTRACT_DLC(msgBuff->u32IERReg));
	lenToDisplay = EXTRACT_DLC(msgBuff->u32IERReg);
	if (lenToDisplay > sizeof(msgBuff->au8Data)) 
		printf("** invalid message length **\n");
	if (lenToDisplay > 10) {
		printf("msg length=%d, displaying first 10 characters\n", lenToDisplay);
		lenToDisplay = 10;
	}
	if(!EXTRACT_RTR(msgBuff->u32IERReg))
		for (j = 0; j < lenToDisplay; j++)
			printf("%02x ", msgBuff->au8Data[j]);
	else
		for (j = 0; j < lenToDisplay; j++)
			printf("-- ");
	printf("\n");
}

void printToFile(stRX_MESSAGE * msgBuff)
{
	int lenToDisplay=0,j=0;

	fprintf(fp,"%d\t", (msgBuff->u32TimeStamp));
	fprintf(fp,"0x%08x\t", msgBuff->u32Id);
	fprintf(fp,"Port #%d\t", msgBuff->u8CANPort);
	if(EXTRACT_RTR(msgBuff->u32IERReg))
		fprintf(fp,"RTR\t");
	else 
		fprintf(fp,"---\t");
	fprintf(fp,"%d\t", EXTRACT_DLC(msgBuff->u32IERReg));
	lenToDisplay = EXTRACT_DLC(msgBuff->u32IERReg);
	if (lenToDisplay > sizeof(msgBuff->au8Data)) 
		fprintf(fp,"** invalid message length **\n");
	if (lenToDisplay > 10) {
		fprintf(fp,"msg length=%d, displaying first 10 characters\n", lenToDisplay);
		lenToDisplay = 10;
	}
	if(!EXTRACT_RTR(msgBuff->u32IERReg))
		for (j = 0; j < lenToDisplay; j++)
			fprintf(fp,"%02x ", msgBuff->au8Data[j]);
	else
		for (j = 0; j < lenToDisplay; j++)
			fprintf(fp,"-- ");
	fprintf(fp,"\n");
}

void runSpecialTest()
{
	stTX_DATA		txData;

	while (_kbhit())
		_getch();   // Empties buffer

	while(!_kbhit())
	{
				txData.u32IERReg=0;
				txData.u32TxRate = 0;

				txData.u32Id = 0x1267f;

				txData.u8CANPort = 1;
				txData.u32IERReg |= PLACE_EXT(1);

				txData.u32IERReg |= PLACE_DLC(2);
				txData.au8Data[0]=0x49;
				txData.au8Data[1]=0x44;

				txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
				CAN_write(&canDevice, &txData);

		//		Sleep(100);
	}


}

void runDefaultMessages()
{

	int errorCode, i, errCount=0, messagesRead=0;
	stTX_DATA		txData;
	int loopCounter=0;

	// set both channels at 500k, we assume there is a loopback
	errorCode= CAN_setBaud(&canDevice,(UINT32)6,(UINT32)3);

	// write a message from channel 0 at 1ms transmit
	txData.u32TxRate = 1;
	txData.u32Id = 0x12345678;
	txData.u8CANPort = 0;
	txData.u32IERReg=0;
	txData.u32IERReg |= PLACE_EXT(1);
	txData.u32IERReg |= PLACE_DLC(8);
	for (i = 0; i<8; i++) {
		txData.au8Data[i] = 0x11*i;
	}
	txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
	CAN_writeCyclic(&canDevice, &txData );

	// write a message from channel 1 at 1ms transmit
	txData.u32TxRate = 1;
	txData.u32Id = 0x0fedcba9;
	txData.u8CANPort = 1;
	txData.u32IERReg=0;
	txData.u32IERReg |= PLACE_EXT(1);
	txData.u32IERReg |= PLACE_DLC(8);
	for (i = 0; i<8; i++) {
		txData.au8Data[i] = 0xFF-(0x11*i);
	}
	txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
	CAN_writeCyclic(&canDevice, &txData );
	txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
	CAN_writeCyclic(&canDevice, &txData );

	// write a message from channel 0 at 1ms transmit
	txData.u32TxRate = 1;
	txData.u32Id = 0x13579bd;
	txData.u8CANPort = 0;
	txData.u32IERReg=0;
	txData.u32IERReg |= PLACE_EXT(1);
	txData.u32IERReg |= PLACE_RTR(1);
	txData.u32IERReg |= PLACE_DLC(8);
	for (i = 0; i<8; i++) {
		txData.au8Data[i] = 0xFF-(0x11*i);
	}
	txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
	CAN_writeCyclic(&canDevice, &txData );

}

// this function will convert a libusb error number to a libusb error string 
char * errToString(int errNumber)
{
	switch(errNumber){
	case LIBUSB_SUCCESS: return "LIBUSB SUCCESS";
	case LIBUSB_ERROR_IO: return "LIBUSB IO ERROR";
	case LIBUSB_ERROR_INVALID_PARAM: return "LIBUSB INVALID PARAMETER";
	case LIBUSB_ERROR_ACCESS: return "LIBUSB INSUFFICIENT PERMISSION";
	case LIBUSB_ERROR_NO_DEVICE: return "LIBUSB NO DEVICE";
	case LIBUSB_ERROR_NOT_FOUND: return "LIBUSB ENTITY NOT FOUND";
	case LIBUSB_ERROR_BUSY: return "LIBUSB RESOURCE BUSY";
	case LIBUSB_ERROR_TIMEOUT: return "LIBUSB TIMEOUT";
	case LIBUSB_ERROR_OVERFLOW: return "LIBUSB OVERFLOW ERROR";
	case LIBUSB_ERROR_PIPE: return "LIBUSB PIPE ERROR";
	case LIBUSB_ERROR_INTERRUPTED: return "LIBUSB SYSCALL INTERRUPTED";
	case LIBUSB_ERROR_NO_MEM: return "LIBUSB NO MEM ERROR";
	case LIBUSB_ERROR_NOT_SUPPORTED: return "LIBUSB OPERATION NOT SUPPORTED";
	case LIBUSB_ERROR_OTHER: return "LIBUSB OTHER ERROR";	

	}
	return "ERROR CODE NOT RECOGNIZED";
}
// sendContinuous is another testing mode which will send or receive a wide variety of messages
// sendhigh is so you can have 2 boards looped back and sending different messages
void sendContinuous(BOOL send, BOOL receive, int msWait)
{
	{
		BOOL runProgram = TRUE;
		int currentByte=0,temp=0,i,j,errorCode,errCount=0, messagesRead=0;
		stRX_MESSAGE	msgBuff;
		stBULK_TX_DATA bulkBuffer;
		time_t t1,t2,t3;
		int loopCounter=0;
		//starting and ending message ID's
		UINT32 startValue = 0x4e2, endValue = 0x4e2+49;
		// we assume that we send each ID betxween start and end
		UINT32 numMessages = 50;
		UINT32 receivedMessageBucket[50];				// this could be malloced later, change once time permits
		// number of CAN messages in a USB message
		UINT32 usbPacketSize = 50, numMessagePacks=0, numPacketsSent=0, numErrors =0;

		clock_t currentTime = clock() / (CLOCKS_PER_SEC / 1000);
		clock_t sendTimer = clock() / (CLOCKS_PER_SEC / 1000);
		clock_t receiveStartTime = clock() / (CLOCKS_PER_SEC / 1000);

		time(&t1);
		time(&t3);

		if(transmitHigh)
		{
			startValue += 0x100;
			endValue += 0x100;
		}

		for(i=0; i<numMessages; i++)
			receivedMessageBucket[i]=0;  

		printf("enabling testing mode...\n");
		if(send)
			printf("I will be sending to the CAN board, \n");
		if(receive)
		{
			printf("i will be receiving and verifying data read from the board \n");
			runReceiveTestThread=1;
			StartReadCANThread(1);
		}

		while(runProgram)
		{
			currentTime = clock() / (CLOCKS_PER_SEC / 1000);
			// send once per mswait,  one has to be careful with the PCIe-Mini-CAN send buffer
			// it stores only 2 usb messages at a time, so if one wants to have dynamic messages
			// at a cyclic interval(say 100ms) it's better to send a fraction every 10ms
			// rather than 8 back-to-back messages ( in the current firmware 6/17)
			if(send && currentTime-sendTimer > msWait )
			{
				sendTimer = clock() / (CLOCKS_PER_SEC / 1000);
				numMessagePacks=0;

				// write from 4e0 to 5e7 inclusive
				for(i=0; (i+numMessagePacks*usbPacketSize)<numMessages; i++)
				{
					// send messages every 50
					if(i%usbPacketSize ==0 && i !=0)
					{
						bulkBuffer.messagesUsed=i;
						CAN_write_bulk(&canDevice,&bulkBuffer);
						i=0;
						numMessagePacks++; // usbPacketSize more
					}

					bulkBuffer.messages[i%50].u32IERReg=0;
					bulkBuffer.messages[i%50].u32TxRate = 100;
					bulkBuffer.messages[i%50].u8CANPort = 0;
					bulkBuffer.messages[i%50].u32IERReg |= PLACE_EXT(1);
					bulkBuffer.messages[i%50].u32IERReg |= PLACE_DLC(6);

					bulkBuffer.messages[i%50].u32Id = startValue + i + numMessagePacks*usbPacketSize;
					bulkBuffer.messages[i%50].au8Data[0] = bulkBuffer.messages[i%50].u32Id&(0xFF);

					for (j = 1; j<6; j++)
						bulkBuffer.messages[i%50].au8Data[j] = (bulkBuffer.messages[i%50].u32Id*(11+j*2))&(0xFF);
					bulkBuffer.messages[i%50].u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
					//			CAN_write(&canDevice, &txData );				
				}
				// the last few that didn't add to 50 should also be sent
				bulkBuffer.messagesUsed=i;
				errorCode = CAN_write_bulk(&canDevice,&bulkBuffer);
				if(errorCode==CAN_SUCCESS)
					numPacketsSent++;
				else
					printf(" failed to transmit with code: %s\n",errToString(errorCode));

				if(numPacketsSent%50 ==0)
					printf("Transmitted: %d message sets\n",numPacketsSent);
			}
			if(receive)
			{

			}

			// if we want to stop running allow the possibility to exit out
			if(_kbhit())
			{
				// some some reason if a single getch is done it will eat non-x
				// characters but will pause on x, this makes sure it will always pause
				while(_kbhit())
					_getch();

				printf("press 'x' to stop running or any other key to continue\n");
				while(!_kbhit());
				temp = _getch();
				if( temp =='x')
					runProgram = FALSE;
			}
		}
	}
	runReceiveTestThread=0;
	Sleep(100); // sleep 100 ms so that the thread has time to die
}

// runs a simple counter of messages looped, ignores the messages which are not of ID 0x12345
void testingMode()
{
	BOOL runProgram = TRUE;
	int currentByte=0,temp=0,i,j,errorCode,errCount=0;
	stTX_DATA		txData;
	stRX_MESSAGE	msgBuff;
	time_t t1,t2;
	time(&t1);

	printf("enabling testing mode...\n");
	txData.u32IERReg=0;
	txData.u32TxRate = 100;
	txData.u8CANPort = 0;
	txData.u32IERReg |= PLACE_EXT(1);
	txData.u32IERReg |= PLACE_DLC(8);
	for(i=0; i<0x107; i++)
	{
		txData.u32Id = 0x4e0 + i;
		//				txData.u32IERReg |= 0x00800; // auto-increment bit
		//				txData.u32IERReg |= 0x00400; // incrementing ID bit
		for (j = 0; j<8; j++) {
			txData.au8Data[j] = (txData.u32Id*(11+j*2))&(0xFF);
		}
		txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
		CAN_writeCyclic(&canDevice, &txData );

	}


	while(runProgram)
	{
		errorCode = CAN_read(&canDevice, &msgBuff );
		if(errorCode!= CAN_SUCCESS && errorCode != LIBUSB_ERROR_TIMEOUT)
			printf("nontrivial error %s received\n",errToString(errorCode));
		else
		{			

			// this is our only message
			if(msgBuff.u32Id != 0x00012345)
				continue;
			temp = (msgBuff.au8Data[0] <<8) + msgBuff.au8Data[1];
			if(temp != currentByte+1)
			{
				errCount++;
				printf("counter jump detected, expected message %d but received %d\n", currentByte+1, temp);
				currentByte = temp;
			}
			else
				currentByte++;

			if(currentByte ==0)
			{
				time(&t2);
				printf("\n\n\n******** received the last 0xFFFF messages in %d seconds with %d errors *********** \n\n",t2-t1, errCount);
				time(&t1); 
			}
			else if(currentByte %0xFF == 0)
				printf("received the next 255...\n");
		}

		if(_kbhit())
		{
			printf("press 'x' to stop running or any other key to continue\n");
			_getch();
			while(!_kbhit());
			if(_getch() =='x')
				runProgram = FALSE;

		}

	}



}
UINT8 receiveBuffer[1024];
UINT8 receiveBuffer[1024];
// specialized test read, checks for a sequence and data integrity
#ifdef linux
void *readCANTestThread(void *pV)
#else
static unsigned __stdcall readCANTestThread(void *pV)
#endif
{
	int currentByte=0x4e1, errCount=0, errorCode, loopCounter=0, temp=0, messagesRead=0;
	stRX_MESSAGE	msgBuff;
	UINT32 receivedMessageBucket[50];				// this could be malloced later, change once time permits
	UINT32 i, passCounter=0, idJumpErr=0, msgDataErr=0, failCounter=0;
	clock_t currentTime = clock() / (CLOCKS_PER_SEC / 1000);
	clock_t receiveStartTime = clock() / (CLOCKS_PER_SEC / 1000);

	//starting and ending message ID's
	UINT32 startValue = 0x4e2, endValue = 0x4e2+49;
	UINT32 numMessagePacks=0;

	if(receiveHigh)
	{
		startValue += 0x100;
		endValue += 0x100;
		currentByte+= 0x100;
	}

	for(i=0; i<50; i++)
		receivedMessageBucket[i]=0;

	currentByte = startValue-1;

	while (runReceiveTestThread==TRUE)
	{

#ifdef READ_BYTESTREAM
				errorCode = CAN_readBytes(&canDevice,receiveBuffer,1024);
		for(i=0; i<errorCode; i++)
			fprintf(fp, "%02x ",receiveBuffer[i]);

#else


		errorCode = CAN_read(&canDevice, &msgBuff );

		
		if(errorCode!= CAN_SUCCESS && errorCode != LIBUSB_ERROR_TIMEOUT && outputToScreen)
			printf("nontrivial error %d %s received\n",errorCode, errToString(errorCode));
		else if(errorCode!= CAN_SUCCESS && errorCode != LIBUSB_ERROR_TIMEOUT && fp)
			fprintf(fp,"nontrivial error %d %s received\n",errorCode, errToString(errorCode));

		if(errorCode == LIBUSB_ERROR_TIMEOUT)
			continue;
		// this test only looks at these messages, but extras can be added to
		// load down the bus, so long as they are not in the acceptance range
		if(msgBuff.u32Id < startValue || msgBuff.u32Id > endValue)
			continue;

		messagesRead++;
		receivedMessageBucket[msgBuff.u32Id%50]++;

		if(msgBuff.u32Id != currentByte+1)
		{
			if(!(msgBuff.u32Id ==startValue && currentByte ==endValue ))
			{
				errCount++;
				idJumpErr++;
				if(fp)
				{
					fprintf(fp,"jump: expected 0x%04x but received 0x%04x on port %d", currentByte+1, msgBuff.u32Id, msgBuff.u8CANPort);
					fprintf(fp," 0x%x (%d) messages since last set\n",messagesRead,messagesRead);
				}
				currentByte = msgBuff.u32Id;
			}
			else 
				currentByte = msgBuff.u32Id;
		}
		else
			currentByte= currentByte+1;



		// next we check if the data field is valid, we put pseudorandom data in there
		if(msgBuff.au8Data[0] != (msgBuff.u32Id&0xFF) && outputToScreen)
		{
			printf("invalid data byte received, received %02x but expected %02x\n",msgBuff.au8Data[0],(msgBuff.u32Id &0xFF));
			errCount++;
		}
		else if(msgBuff.au8Data[0] != (msgBuff.u32Id&0xFF))
		{
			fprintf(fp,"invalid data byte received, received %02x but expected %02x\n",msgBuff.au8Data[0],(msgBuff.u32Id &0xFF));
			errCount++;
		} 
		for(i=1; i<6; i++)
		{
			if(msgBuff.au8Data[i] != (msgBuff.u32Id*(11+i*2) &0xFF) && outputToScreen)
			{
				if(msgBuff.au8Data[i] != (msgBuff.u32Id*(11+i*2) &0xFF))
				{
					msgDataErr++;
					fprintf(fp,"invalid data byte received, received %02x but expected %02x\n",msgBuff.au8Data[i],(msgBuff.u32Id*(11+i*2) &0xFF));
					errCount++;
					// if a data field is corrupt just show it once
					i=8;
				}


			}
		}

		if(currentByte ==startValue)
		{
			currentTime = clock() / (CLOCKS_PER_SEC / 1000);					
			if(loopCounter%10 ==0)
			{
				if(errCount ==0)
				{
					passCounter++;
					printf(" PASS: \t\t Total-- PASS:%d FAIL:%d\n", passCounter, failCounter);
					if(fp)
						fprintf(fp," PASS: \t\t Total-- PASS:%d FAIL:%d\n", passCounter, failCounter);
				}
				else
				{
					failCounter++;
					printf(" FAIL: %d idErr %d msgErr\t Total-- PASS:%d FAIL:%d\n",idJumpErr, msgDataErr, passCounter, failCounter);
					if(fp)
						fprintf(fp," FAIL: %d idErr %d msgErr\t Total-- PASS:%d FAIL:%d\n",idJumpErr, msgDataErr, passCounter, failCounter);

				}

				receiveStartTime = clock() / (CLOCKS_PER_SEC / 1000);

				errCount=0;
				idJumpErr=0;
				msgDataErr=0;
			}
			loopCounter++;

		}
#endif // READ_BYTESTREAM


	}

	if(outputToScreen)
	{
		printf("thread quitting, Beginning the bucket dump:\n");
		temp=0;
		for(i=0; i<50; i++)
		{
			printf("%d:%d\n",i,receivedMessageBucket[i]);
			temp+= receivedMessageBucket[i];
		}
		printf("received a total of %d messages\n",temp);
	}
	else
	{
		fprintf(fp,"thread quitting, Beginning the bucket dump:\n");
		temp=0;
		for(i=0; i<50; i++)
		{
			fprintf(fp,"%d:%d\n",i,receivedMessageBucket[i]);
			temp+= receivedMessageBucket[i];
		}
		fprintf(fp,"received a total of %d messages\n",temp);
	}
	return 1;
}

//generic read, this will read one message at a time and dump to STDOUT or a file
#ifdef linux
void *readCANThread(void *pV)
#else
static unsigned __stdcall readCANThread(void *pV)
#endif
{
	UINT32 errorCode;
	int displayNoDataTimer=0, bufferFlushCounter=100;

	stRX_MESSAGE	msgBuff;

	while (runReceiveThread==TRUE) {

		// check for message received
		errorCode = CAN_read(&canDevice, &msgBuff );
		if (errorCode==CAN_SUCCESS ){
			displayNoDataTimer=0; // make sure we don't print "no data" while reading data
			if(outputToScreen)
			{
				printToScreen(&msgBuff);
			}
			else
			{
				printToFile(&msgBuff);
				if(bufferFlushCounter==0)
				{
					bufferFlushCounter=100;
					fflush(fp);
				}
				else
					bufferFlushCounter--;
			} 
		}
		else {

			// it was just a status message, no problem
			if (errorCode == CAN_STATUS);

			else if (errorCode == -5) {
				printf ("Read error #%d: the board is disabled, exiting... (press <enter>)\n", errorCode);
				_getch();
				CAN_close(&canDevice);
				//				errorCode = CAN_open(&canDevice);
				if(errorCode) exit(1);
			}
			else if( errorCode == LIBUSB_ERROR_TIMEOUT)
			{
				displayNoDataTimer++;
				if(displayNoDataTimer==10) // only talk about data every 1000 * .01 seconds
				{
					printf("no data...\n");
					displayNoDataTimer=0;
				}
			}
			else
				printf("error received: %s\n", errToString(errorCode));			
		}

		// don't spam the screen while a menu is up, sleep 3ms between each check
		while(pauseReceiveThread==TRUE)
			Sleep(3);
	}
	return 0;
}


#ifdef linux
void *        thr_id;         /* thread ID for the newly created thread */
pthread_t  p_thread;       /* thread's structure                     */
// thread to read the CAN messages
int StartReadCANThread(int threadType)
{

	if(threadType==0)
	{
		runReceiveThread=1;
		/* returns 0 on success, err number otherwise */
		thr_id = pthread_create(
			&p_thread,				// p_thread *
			NULL,					// pthread_attr_t * attributes ( scheduling, scope, etc )
			readCANThread,			// void *(*start_routine)(void*) method which will be running
			NULL);					// void * arguments
	}
	else if(threadType ==1)
	{
		runReceiveTestThread=1;
		/* returns 0 on success, err number otherwise */
		thr_id = pthread_create(
			&p_thread,				// p_thread *
			NULL,					// pthread_attr_t * attributes ( scheduling, scope, etc )
			readCANTestThread,		// void *(*start_routine)(void*) method which will be running
			NULL);					// void * arguments
	}

	if(thr_id != 0)
		return -1;
	return thr_id;
}

// kill the CAN reading thread
int stopReadCANThread()
{
	thr_id = NULL;
	runReceiveThread = 0;
}
#else
HANDLE thr_id;
int StartReadCANThread(int threadType)
{
	HANDLE ThreadHandle = 0;
	unsigned ThreadIdentifier = 0;

	if(threadType ==0)
	{
		runReceiveThread=1;
		ThreadHandle = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			readCANThread,			// thread function name
			NULL,					// argument to thread function 
			0,                      // use default creation flags 
			&ThreadIdentifier);		// returns the thread identifier 
	}

	else if(threadType ==1)
	{
		runReceiveTestThread=1;
		ThreadHandle = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			readCANTestThread,		// thread function name
			NULL,					// argument to thread function 
			0,                      // use default creation flags 
			&ThreadIdentifier);		// returns the thread identifier 
	}
	if (ThreadHandle == NULL)
		return -1;

	thr_id = ThreadHandle;

	// Give the new thread a chance to run
	Sleep(0);
	return 1;
}
// kill the CAN reading thread
stopReadCanThread(int threadType)
{
	UINT32 temp;
	temp = WaitForSingleObject(thr_id,10);
	if(temp == WAIT_TIMEOUT)
		printf("timed out waiting for reader thread to die...\n");
	else if( temp== WAIT_FAILED)
		printf(" wait for timeout failed with code %08x\n",GetLastError());
	CloseHandle(thr_id);
	thr_id = NULL;
	runReceiveThread=0;
}
#endif

int main(int argc, void* argv[])
{
	//  #define PERFORMANCE_TEST 1
	//    BOOL bResult = TRUE;
	stTX_DATA		txData;
	int i=0, j, k;
	int errorCode;
	char c;
	BOOL runProgram = TRUE;
	UINT8 receiveBuffer[1024];
	int bytesRead;
	clock_t currentTime, oneSecondLater;

	errorCode = CAN_init();
	errorCode = CAN_open(&canDevice);
	if (errorCode != CAN_SUCCESS){
		if (errorCode==100) {
			printf("USB Can controller not found (error %d)\n", errorCode);
			printf ("Exiting... (press <enter>)\n");
			_getch();
			exit(1);
		}

		printf("Failed to open the USB Can controller error %d\n", errorCode);
		printf ("Exiting... (press <enter>)\n");
		_getch();
		exit(1);
	}

	canDevice.RxTimeOutValue = 100;		 	// .10 second read time out
	errorCode = CAN_removeAllCyclic(&canDevice);
	if (errorCode != CAN_SUCCESS){
		printf("Failed to communicate with the USB Can controller\n");
		printf ("Exiting... (press <enter>)\n");
		_getch();
		goto done;
	}
	// flush remaining bytes from the receiving side of the USB buffer
	do{
	bytesRead = CAN_readBytes(&canDevice, receiveBuffer, 1024 );
	}while(bytesRead==1024);

		printf("please choose output method, 1 for screen 0 to write to a file: ");
		c = _getch();
		printf("\n");
		if(c!='0')
			outputToScreen=TRUE;
		else
		{
	// choose a file location to write to
	outputToScreen=FALSE;
	fp=fopen("canLog.txt", "w+");
	if (fp==NULL) 
	{
		printf("open failed for %s, with code 0x%08x\n", "canLog.txt", GetLastError());
		printf("i'll just output to the screen then\n");
		outputToScreen = TRUE;
	}
		}

	// start the thread that will pull data from the board and push to the screen/file
	runReceiveThread = TRUE;
	StartReadCANThread(0);
	//	StartReadCANThread(0);
	//	StartReadCANThread(0);

	// make some generic messages
	txData.u32Id = 0x123;
	txData.u8CANPort =0;
	txData.u32TxRate = 1;
	txData.u32IERReg = PLACE_DLC(1);
	txData.au8Data[0]=0x11;
	txData.au8Data[1]=0x55;
	txData.au8Data[2]=0x66;
	txData.au8Data[3]=0x77;
	txData.au8Data[4]=0x88;
	txData.au8Data[5]=0x99;
//	CAN_writeCyclic(&canDevice, &txData );
	txData.u32Id = 0x234;
	txData.au8Data[0]=0x22;
//	CAN_writeCyclic(&canDevice, &txData );
	txData.u32Id = 0x345;
	txData.au8Data[0]=0x33;
//	CAN_writeCyclic(&canDevice, &txData );
	txData.u32Id = 0x456;
	txData.au8Data[0]=0x44;
//	CAN_writeCyclic(&canDevice, &txData );

	// 500kbaud
//	errorCode= CAN_setBaud(&canDevice,(UINT32)6,(UINT32)3);

	printf("press <enter> for command menu\n");
	while(runProgram)
	{		
		// check for user input
		if (_kbhit()) {
			// pause the receive thread
			pauseReceiveThread =TRUE;
			// enter new message
			while (_kbhit())
				_getch();   // Empties buffer
			printf("Command input mode: \n");
			printf("0: send message\n");
			printf("1: clear all cyclical messages\n");
			printf("2: on-line the board\n");
			printf("3: get status\n");
			printf("4: set baud rate\n");
			printf("5: wipe firmware\n");
			printf("6: quickTest\n");
			printf("8: perform a send continuous single transmit test\n");
			printf("9: stopwatch\n");
			printf("a: get the status of buffers\n");
			printf("b: clear buffers\n");
			printf("c: switch buffer clear mode\n");
			printf("x: exit the application\n");
			printf("any other character to display received messages\n");
			printf("Enter command: >");
			c=_getch();
			printf("%c\n",c);
			switch (c) {
				// exit the program
			case 'x':
			case 'X':
				printf("exiting program and clearing cyclic messages...\n");
				runProgram = FALSE;
			case '1':		 // remove all cyclic commands
				errorCode = CAN_removeAllCyclic(&canDevice);
				break;
			case '2':
				errorCode = CAN_onLine(&canDevice);
				break;
			case '3':
				errorCode = CAN_status(&canDevice);
				break;
			case '4':
				while (_kbhit())
					_getch();   // Empties buffer
				for(i=0;i<BAUD_RATE_PRESETS; i++)
					printf("%d: %s\n",i,baudRates[i]);
				printf("Please enter the number of the baud rate you want:\n");
				scanf("%d",&i);
				printf("Please enter which channel(s) to set (1 for CAN0, 2 for CAN1, 3 for both)\n");
				scanf("%d",&j);
				errorCode= CAN_setBaud(&canDevice,(UINT32)i,(UINT32)j);
				while (_kbhit())
					_getch();   // clear the last line feed on linux
				// flush remaining bytes from the receiving side of the USB buffer
				bytesRead = CAN_readBytes(&canDevice, receiveBuffer, 1024 );
				bytesRead = CAN_readBytes(&canDevice, receiveBuffer, 1024 );
				bytesRead = CAN_readBytes(&canDevice, receiveBuffer, 1024 );

				break;
			case '5':
				printf("please enter the version number in decimal:\n");
				scanf("%d",&j);
				errorCode = CAN_disableFirmware(&canDevice,j);
				break;

			case '7':
		//		testingMode();				
				pauseReceiveThread=0; 
				runSpecialTest();
				break;
			case '8':
				printf(" please enter 1 if this board will be receiving, 2 for transmitting, 3 for both\n");
				scanf("%d",&j);

				if(j&1)
				{

					printf(" please enter 1 if this board will RECEIVE high range messages ( 5e2+ ), 0 for low\n");
					scanf("%d", &k);
					if(k>=1)
						receiveHigh = 1;
					else
						receiveHigh = 0;
				}
				if(j&2)
				{
					printf(" please enter 1 if this board will TRANSMIT high range messages ( 5e2+ ), 0 for low\n");
					scanf("%d", &i);
					if(i>=1)
						transmitHigh = 1;
					else
						transmitHigh = 0;
				}


				if( j ==1)
					sendContinuous((j>>1)&0x1,j&0x1,k);
				else
				{
					printf(" please enter the delay between messages in ms\n");
					scanf("%d",&k);
					sendContinuous((j>>1)&0x1,j&0x1,k);
				}

				break;
			case '9':
				printf(" i am going to wait for 10 seconds and then print another message, press a key when ready\n");
				_getch();
				currentTime = clock() / (CLOCKS_PER_SEC / 1000);
				oneSecondLater = clock() / (CLOCKS_PER_SEC / 1000);
				printf("the current time is: %d in ms\n",currentTime);
				while( oneSecondLater - currentTime <10000)
					oneSecondLater = clock() / (CLOCKS_PER_SEC / 1000);
				printf(" one second has passed, current time is %d in ms\n", oneSecondLater);
				break;

				

				// run the default test messages
			case '6':
				while (_kbhit())
					_getch();   // Empties buffer

				//set the baud rate to 500k
				errorCode= CAN_setBaud(&canDevice,(UINT32)6,(UINT32)3);

				// send 0x4e2-0x513 and expect those back
				receiveHigh=0;
				transmitHigh=0;

				// send, recieve, 20ms delay
				sendContinuous(1,1,30);				
				
				break;

				// a will be to retrieve the buffer status, b to clear
			case 'a':
				while (_kbhit())
					_getch();   // Empties buffer
				CAN_buffer_fullness(&canDevice);
				break;

				// clear buffer
			case 'b':
				while (_kbhit())
					_getch();   // Empties buffer
				printf("which buffer would you like to clear (0/1/others to cancel) ? ");
				scanf("%d", &j);
				if(j==0 || j==1)
					CAN_clear_buffer(&canDevice,(UINT8)j);

				break;

				// switch buffer mode
			case 'c':
				while (_kbhit())
					_getch();   // Empties buffer
				printf(" choose 0 for manual clear, 1 for auto clear (0/1/others to cancel)?");
				scanf("%d", &j);
				if(j==0 || j==1)
					CAN_switchBufferMode(&canDevice,(UINT8)j);
				break;

				// enter new message
			case '0':
				while (_kbhit())
					_getch();   // Empties buffer
				txData.u32IERReg=0;
				printf("rate in mS (0 for once): ");
				scanf("%d", &j);
				txData.u32TxRate = j;

				printf("id (hexadecimal): 0x");
				scanf("%x", &j);
				txData.u32Id = j;

				printf("port (0/1): ");
				scanf("%d", &j);
				if (j) j = 1;
				else j = 0;

				txData.u8CANPort = j;
				while (_kbhit())
					_getch();   // Empties buffer
				printf("will this be a remote transmit message ? ( N/y)\n");
				c = _getch();
				if(c=='y' || c=='Y')
					txData.u32IERReg |= PLACE_RTR(1);

				while (_kbhit())
					_getch();   // Empties buffer

				if(txData.u32Id > 0x3FF)
				{
					printf("will this be an extended frame message ? ( Y/n)\n");
					c = _getch();
					if(c!='n' && c!='N')
						txData.u32IERReg |= PLACE_EXT(1);
				}
				else
				{
					printf("will this be an extended frame message ? ( N/y)\n");
					c = _getch();
					if(c=='y'|| c=='Y')
						txData.u32IERReg |= PLACE_EXT(1);
				}


				printf("number of bytes (0..8): ");
				scanf("%x", &j);
				if (j>8) j = 8;
				if (j<0) j = 0;
				txData.u32IERReg |= PLACE_DLC(j);
				if( !EXTRACT_RTR(txData.u32IERReg))
					for (i = 1; i<=j; i++) {
						printf("byte #%d (hexadecimal): 0x", i);
						scanf("%x", &k);
						txData.au8Data[i-1] = k;
					}
					txData.u8Type=77; // 1 = multipacket, 2 = fastpacket, other = single send normal
					if (txData.u32TxRate == 0)
						CAN_write(&canDevice, &txData);

					else
						CAN_writeCyclic(&canDevice, &txData );

					txData.u32IERReg=0;
					break;
			}
			// free the receive thread
			pauseReceiveThread=0;
		}
		Sleep(1);
	}

	// make sure the thread hasn't blocked itself
	pauseReceiveThread=FALSE;
	// allow the thread to die
	runReceiveThread=FALSE;

	//give the threads a chance to quit
	Sleep(1000);

	// keep libusb happy
	CAN_close(&canDevice);
done:
	if(fp!=NULL)
		fclose(fp);
	return 0;
}

