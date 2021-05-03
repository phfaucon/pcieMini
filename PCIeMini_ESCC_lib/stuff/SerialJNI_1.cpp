#include <stdio.h>
#include <stdlib.h>
#include "jni.h"
#include "SerialJNI.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <fstream>
#include <string>
#include "uart_channel.h"
#include <string.h>
#include <iostream>
#include <sys/time.h>
#include "TestClass.h"
#include <pthread.h>
#include <time.h>
#include "vme_32io.h"

using namespace std;

vme_SCC16 *dut16;
vme_32io *dut;
/*
uart_channel *channel;
*/

int receive(uart_channel * chan, char * buffer, int bufferSize, int timeOutVal);
int send(uart_channel * chan, const char* buffer, int buffsize, int timeOutVal);
void test();

JNIEXPORT jlong JNICALL Java_serialdriver_SerialDriver_openSerialPort
  (JNIEnv *env, jobject callingClass, jobject sconfig ) 
{
//    test();
    usleep(1000000);
    dut16 = new vme_SCC16(0x8000);
        char buffer[128];
        uart_channelConfig config;
        int i;
        int nbrReceived;

//        printf("time 1 %f " + getimeofday());
        usleep(1000000);
        // connect to the VME SCC 16
        if (0x2ba30540 == dut16->modelNumber)
                cout << "The VME_SCC16 has been found!\n";
        else {
//                printf("time 2 %f " + getimeofday());
                cout << "The VME_SCC16 has not been found: exiting!!\n";
                return 0;
        }
        printf("Model number: %d (0x%08x)\n", dut16->modelNumber, dut16->modelNumber);

        // set the UART configuration
        config.baudRate = 9600;
        config.dataBits = 8;
        config.eolChar = '\n';
        config.parity = 1;
        config.parityEnable = 0;
        config.stopBits = 1;
        config.bufferConfig = uart_channel::RS_422_buffers;
        config.useRxFifo = true;
        config.useTxFifo = true;

        // Print  UART configuration
        printf("Baud Rate  %d bps \n", config.baudRate );
        

        // We are going to use an outside loopback cable between SCC #2 and SCC #3
//        uart_channel * rx_uart1 = new uart_channel(dut16, &config, 1);
//        uart_channel * rx_uart1 = new uart_channel(dut, 0, 1);
//        printf("1 Baud Rate  %d bps \n", config.baudRate );
        printf("Sleep Start\n");
        usleep(1000000);
        printf("Sleep End\n");
//        uart_channel *rx_uart2 = new uart_channel(dut16, &config, 2);
//        printf("2 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart3 = new uart_channel(dut, &config, 3);
//        printf("3 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart4 = new uart_channel(dut, &config, 4);
//        printf("4 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart5 = new uart_channel(dut, &config, 5);
//        printf("5 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart6 = new uart_channel(dut, &config, 6);
//        printf("6 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart7 = new uart_channel(dut, &config, 7);
//        printf("7 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart8 = new uart_channel(dut, &config, 8);
//        printf("8 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart9 = new uart_channel(dut, &config, 9);
//        printf("9 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart10 = new uart_channel(dut, &config, 10);
//        printf("10 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart11 = new uart_channel(dut, &config, 11);
//        printf("11 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart12 = new uart_channel(dut, &config, 12);
//        printf("12 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart13 = new uart_channel(dut, &config, 13);
//        printf("13 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart14 = new uart_channel(dut, &config, 14);
//        printf("14 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart15 = new uart_channel(dut, &config, 15);
//        printf("15 Baud Rate  %d bps \n", config.baudRate );
        uart_channel *rx_uart16 = new uart_channel(dut16, &config, 0);
        rx_uart16 = new uart_channel(dut16, &config, 0);
        printf("16 Baud Rate  %d bps \n", config.baudRate );
        bool running = true;


//      while(running)
//        {
//      for (int loop = 0; loop<10; loop++) {
                // This is the transmitting part
//                printf("Starting For loop\n");
//              printf("Sending  '%s' (length=%d)\n", out_buffer, (int)strlen(out_buffer));
//              double start = msTime();
//              tx_uart->Scc_puts(out_buffer);
//              double end = msTime();
//              printf("sent %d characters in %g milliseconds\n", (int)strlen(out_buffer), end-start);

                // This is the receiving part
//              start = msTime();
//                for(int i=0; i<128; i++)
//                {
//                    buffer[i] = 0;
////                }
//                nbrReceived = receive(rx_uart1, buffer, 126, 1000);
//////////              end = msTime();
//////////              printf("received %d characters in %g milliseconds\n", (int)strlen(out_buffer), end-start);
//                printf("Received 1: ");
//                for(int i=0; i<nbrReceived; i++)
//                {
//                        printf(" %02x ", buffer[i] & 0xff);
//                }
//                printf("\n");
//                
//                for(int i=0; i<128; i++)
//                {
//                    buffer[i] = 0;
//                }
             
//                nbrReceived = receive(rx_uart2, buffer, 1000);
//                printf("Received 2: ");
////                for(int i=0; i<nbrReceived; i++)
////                {
////                        printf(" %02x ", buffer[i] & 0xff);
////                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart3, buffer, 1000);
//                printf("Received 3: ");
////                for(int i=0; i<nbrReceived; i++)
////                {
////                        printf(" %02x ", buffer[i] & 0xff);
////                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart4, buffer, 1000);
//                printf("Received 4: ");
////                for(int i=0; i<nbrReceived; i++)
////                {
////                        printf(" %02x ", buffer[i] & 0xff);
////                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart5, buffer, 1000);
//                printf("Received 5:");
//                for(int i=0; i<nbrReceived; i++)
//                {
//                        printf(" %02x ", buffer[i] & 0xff);
//                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart6, buffer, 1000);
//                printf("Received 6:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart7, buffer, 1000);
//                printf("Received 7:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart8, buffer, 1000);
//                printf("Received 8:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart9, buffer, 1000);
//                printf("Received 9:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart10, buffer, 1000);
//                printf("Received 10:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart11, buffer, 1000);
//                printf("Received 11:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart12, buffer, 1000);
//                printf("Received 12:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart13, buffer, 1000);
//                printf("Received 13:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart14, buffer, 1000);
//                printf("Received 14:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart15, buffer, 1000);
//                printf("Received 15:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart16, buffer, 126, 1000);
//                printf("Received 0:");
//                for(int i=0; i<nbrReceived; i++)
//                {
//                        printf(" %02x ", buffer[i] & 0xff);
//                }
//                printf(" length %d\n",nbrReceived);

for(int i=0; i<100; i++)
{
     usleep(15625);
     nbrReceived = receive(rx_uart16, buffer, 126, 1000);
     printf("Receive 15: ");
     for(int i=0; i<nbrReceived; i++)
     {
         printf(" %02x ", buffer[i] & 0xff);
     }
     printf("\n");
}

                
                
//                rx_uart1->Scc_Close();
//                rx_uart2->Scc_Close();
//                rx_uart3->Scc_Close();
//                rx_uart4->Scc_Close();
//                rx_uart5->Scc_Close();
//                rx_uart6->Scc_Close();
//                rx_uart7->Scc_Close();
//                rx_uart8->Scc_Close();
//                rx_uart9->Scc_Close();
//                rx_uart10->Scc_Close();
//                rx_uart11->Scc_Close();
//                rx_uart12->Scc_Close();
//                rx_uart13->Scc_Close();
//                rx_uart14->Scc_Close();
//                rx_uart15->Scc_Close();
                rx_uart16->Scc_Close();
                delete dut;

                cout << "Exiting!!!" << endl;
}


JNIEXPORT jint JNICALL Java_serialdriver_SerialDriver_closeSerialPort
  (JNIEnv *, jobject, jlong serialId)
{
    uart_channel *channel = (uart_channel*)serialId;
    printf("Closing port %d\n",channel);
//    uart_channel * channel = (uart_channel*)serialId;
//    channel = (uart_channel*)serialId;
    int rv = channel->Scc_Close();
    printf("Port Closed\n");
    delete channel;
    printf("Port Deleted\n");
    return (jint)rv;
    
}

/*
 * Class:     serialdriver_SerialDriver
 * Method:    writeSerialData
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_serialdriver_SerialDriver_writeSerialData
  (JNIEnv *env, jobject, jlong seriaId, jbyteArray data, jint dataSize)
{
    
    uart_channel* channel = (uart_channel*)seriaId;
    jboolean jbool = false;
    jbyte* jbyteData = env->GetByteArrayElements(data,NULL);
    
    //env->GetByteArrayRegion(data,0,dataSize,jbyteData);
    
    char* buffer = new char[dataSize];
    //printf("BEFORE: ");
    //Get Array
    for(int i=0; i<dataSize; i++)
    {
        buffer[i] = jbyteData[i];
        //printf("%x ",buffer[i]);
    }
    
    //printf("\n");
    int rv =  send(channel, buffer, dataSize, 1000);
    //int rv =0;
    delete[] buffer;
    env->ReleaseByteArrayElements(data, jbyteData, 0);
    

    return rv;
    
}


JNIEXPORT jbyteArray JNICALL Java_serialdriver_SerialDriver_readMultipleSerialData
  (JNIEnv *env, jclass, jlongArray serialIdArray)
{
//    jsize len = env->GetArrayLength(serialIdArray);
//    jlong *ptr = env->GetLongArrayElements(serialIdArray, 0);
//    uart_channel* channel = (uart_channel*)ptr[0];
////    printf("Reading from  port %d\n",channel);
//    char buffer[256];
//    int nbrReceived = receive(channel, buffer, 256, 1000);
////    printf("Reading from  port flag 1\n");
//    jbyte* jbarray = new jbyte[nbrReceived];
//    for(int i=0; i<nbrReceived; i++)
//    {
//        printf(" %02x ", buffer[i]);
//        jbarray[i] = buffer[i];
//    }
//    printf("\n");
//    jbyteArray rv = env->NewByteArray(nbrReceived);
////    printf("Reading from  port flag 3\n");
//    env->SetByteArrayRegion(rv,0,nbrReceived,jbarray);
////    printf("Reading from  port flag 4\n");
//    delete[] jbarray;
//    env->ReleaseLongArrayElements(serialIdArray, ptr, 0);
////    printf("\n");
//    return rv;
    
    
    
    
    jsize len = env->GetArrayLength(serialIdArray);
    char ** buffer= new char*[len];
    for(int i=0; i<len; i++)
    {
        buffer[i] = new char[256];
    }
    int * nbrReceived = new int[len];
    jlong *ptr = env->GetLongArrayElements(serialIdArray, 0);
    
    for(int i=0; i<len; i++)
    {
        uart_channel* channel = (uart_channel*)ptr[i];
    //    printf("Reading from  port %d\n",channel);
        nbrReceived[i] = receive(channel, buffer[i], 256, 1000);
    //    printf("Reading from  port flag 1\n");
    }
    int size = 0;
    for(int i=0; i<len; i++)
    {
        size = size + nbrReceived[i];
    }
    size = size + (len * 3);
    
    jbyte* jbarray = new jbyte[size];
    int jsize = 0;
    
    for(int i=0; i<len; i++)
    {
        for(int j=0; j<3; j++, jsize++)
        {
            printf(" %02x ", ':');
            jbarray[jsize] = ':';
        }
        
        for(int j=0; j<nbrReceived[i]; j++, jsize++)
        {
            printf(" %02x ", buffer[i]);
            jbarray[jsize] = buffer[i][j];
        }
        printf("\n");
    }
    printf("\n");
//    jbyte* jbarray = new jbyte[size];
//    for(int i=0; i<nbrReceived; i++)
//    {
//        printf(" %02x ", buffer[i]);
//        jbarray[i] = buffer[i];
//    }
//    printf("\n");
    
    jbyteArray rv = env->NewByteArray(size);
//    printf("Reading from  port flag 3\n");
    env->SetByteArrayRegion(rv,0,size,jbarray);
//    printf("Reading from  port flag 4\n");
    delete [] jbarray;
    delete [] nbrReceived;
    for(int i=0; i<len; i++)
    {
        delete [] buffer[i];
    }
    delete [] buffer;
    env->ReleaseLongArrayElements(serialIdArray, ptr, 0);
//    printf("\n");
    return rv;
}


/*
 * Class:     serialdriver_SerialDriver
 * Method:    readSerialData
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_serialdriver_SerialDriver_readSerialData
  (JNIEnv * env, jobject, jlong serialId)
{
    uart_channel* channel = (uart_channel*)serialId;
//    printf("Reading from  port %d\n",channel);
    char buffer[256];
    int nbrReceived = receive(channel, buffer, 256, 1000);
//    printf("Reading from  port flag 1\n");
    jbyte* jbarray = new jbyte[nbrReceived];
    for(int i=0; i<nbrReceived; i++)
    {
       // printf(" %02x ", buffer[i]);
        jbarray[i] = buffer[i];
    }
    //printf("\n");
    jbyteArray rv = env->NewByteArray(nbrReceived);
//    printf("Reading from  port flag 3\n");
    env->SetByteArrayRegion(rv,0,nbrReceived,jbarray);
//    printf("Reading from  port flag 4\n");
    delete[] jbarray;
//    printf("\n");
    return rv;
}


int send(uart_channel * chan, const char* buffer, int buffsize, int timeOutVal) {
	int i=0;
	int timeout=timeOutVal;
        
        //printf("AFTER: ");
	while (chan->Scc_transmitReady() && i<buffsize){//|| timeout>0) {
		if (chan->Scc_transmitReady()) {
                    
                        //printf("Before %x\n ", buffer[i]);
			char c = chan->Scc_putch(buffer[i++]);
                        //printf("After %x\n ", (unsigned char)c);
//                        printf("\n");
                        
                        //Error in transmission return status.
                        if(c != buffer[i-1])
                        {
                            return c;
                        }
                        //printf("Send %02x ", c);
		        timeout = timeOutVal;
		}
		else {
			timeout--;
		}
	}
        //printf("\n");
	//buffer[i] = 0;

	return i;
}


int receive(uart_channel * chan, char * buffer, int buffsize, int timeOutVal) {
	int i=0;
	int timeout=timeOutVal;
	while (chan->Scc_ReceiveReady() && i<buffsize){//|| timeout>0) {
		if (chan->Scc_ReceiveReady()) {
			char c = chan->Scc_getch();
                        //printf(" %02x ", c);
			buffer[i++] = c;
			timeout = timeOutVal;
		}
		else {
			timeout--;
		}
	}
//        printf("\n");
	buffer[i] = 0;

	return i;
}

void test()
{
        dut16 = new vme_SCC16(0x8000);
        char buffer[128];
        uart_channelConfig config;
        int i;
        int nbrReceived;

//        printf("time 1 %f " + getimeofday());
        usleep(1000000);
        // connect to the VME SCC 16
        if (0x2ba30540 == dut16->modelNumber)
                cout << "The VME_SCC16 has been found!\n";
        else {
//                printf("time 2 %f " + getimeofday());
                cout << "The VME_SCC16 has not been found: exiting!!\n";
                return;
        }
        printf("Model number: %d (0x%08x)\n", dut16->modelNumber, dut16->modelNumber);

        // set the UART configuration
        config.baudRate = 9600;
        config.dataBits = 8;
        config.eolChar = '\n';
        config.parity = 1;
        config.parityEnable = 0;
        config.stopBits = 1;
        config.bufferConfig = uart_channel::RS_422_buffers;
        config.useRxFifo = true;
        config.useTxFifo = true;

        // Print  UART configuration
        printf("Baud Rate  %d bps \n", config.baudRate );
        

        // We are going to use an outside loopback cable between SCC #2 and SCC #3
        uart_channel * rx_uart1 = new uart_channel(dut16, &config, 1);
//        uart_channel * rx_uart1 = new uart_channel(dut, 0, 1);
        printf("1 Baud Rate  %d bps \n", config.baudRate );
        printf("Sleep Start\n");
        usleep(1000000);
        printf("Sleep End\n");
        uart_channel *rx_uart2 = new uart_channel(dut16, &config, 2);
        printf("2 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart3 = new uart_channel(dut, &config, 3);
//        printf("3 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart4 = new uart_channel(dut, &config, 4);
//        printf("4 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart5 = new uart_channel(dut, &config, 5);
//        printf("5 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart6 = new uart_channel(dut, &config, 6);
//        printf("6 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart7 = new uart_channel(dut, &config, 7);
//        printf("7 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart8 = new uart_channel(dut, &config, 8);
//        printf("8 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart9 = new uart_channel(dut, &config, 9);
//        printf("9 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart10 = new uart_channel(dut, &config, 10);
//        printf("10 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart11 = new uart_channel(dut, &config, 11);
//        printf("11 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart12 = new uart_channel(dut, &config, 12);
//        printf("12 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart13 = new uart_channel(dut, &config, 13);
//        printf("13 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart14 = new uart_channel(dut, &config, 14);
//        printf("14 Baud Rate  %d bps \n", config.baudRate );
//        uart_channel *rx_uart15 = new uart_channel(dut, &config, 15);
//        printf("15 Baud Rate  %d bps \n", config.baudRate );
        uart_channel *rx_uart16 = new uart_channel(dut16, &config, 0);
        printf("16 Baud Rate  %d bps \n", config.baudRate );
        bool running = true;


//      while(running)
//        {
//      for (int loop = 0; loop<10; loop++) {
                // This is the transmitting part
//                printf("Starting For loop\n");
//              printf("Sending  '%s' (length=%d)\n", out_buffer, (int)strlen(out_buffer));
//              double start = msTime();
//              tx_uart->Scc_puts(out_buffer);
//              double end = msTime();
//              printf("sent %d characters in %g milliseconds\n", (int)strlen(out_buffer), end-start);

                // This is the receiving part
//              start = msTime();
//                for(int i=0; i<128; i++)
//                {
//                    buffer[i] = 0;
//                }
                nbrReceived = receive(rx_uart1, buffer, 126, 1000);
//////              end = msTime();
//////              printf("received %d characters in %g milliseconds\n", (int)strlen(out_buffer), end-start);
                printf("Received 1: ");
                for(int i=0; i<nbrReceived; i++)
                {
                        printf(" %02x ", buffer[i] & 0xff);
                }
                printf("\n");
//                
//                for(int i=0; i<128; i++)
//                {
//                    buffer[i] = 0;
//                }
             
//                nbrReceived = receive(rx_uart2, buffer, 1000);
//                printf("Received 2: ");
////                for(int i=0; i<nbrReceived; i++)
////                {
////                        printf(" %02x ", buffer[i] & 0xff);
////                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart3, buffer, 1000);
//                printf("Received 3: ");
////                for(int i=0; i<nbrReceived; i++)
////                {
////                        printf(" %02x ", buffer[i] & 0xff);
////                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart4, buffer, 1000);
//                printf("Received 4: ");
////                for(int i=0; i<nbrReceived; i++)
////                {
////                        printf(" %02x ", buffer[i] & 0xff);
////                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart5, buffer, 1000);
//                printf("Received 5:");
//                for(int i=0; i<nbrReceived; i++)
//                {
//                        printf(" %02x ", buffer[i] & 0xff);
//                }
//                printf(" length %d\n",nbrReceived);
//                nbrReceived = receive(rx_uart6, buffer, 1000);
//                printf("Received 6:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart7, buffer, 1000);
//                printf("Received 7:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart8, buffer, 1000);
//                printf("Received 8:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart9, buffer, 1000);
//                printf("Received 9:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart10, buffer, 1000);
//                printf("Received 10:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart11, buffer, 1000);
//                printf("Received 11:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart12, buffer, 1000);
//                printf("Received 12:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart13, buffer, 1000);
//                printf("Received 13:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart14, buffer, 1000);
//                printf("Received 14:' %x size %d\n", buffer, nbrReceived);
//                nbrReceived = receive(rx_uart15, buffer, 1000);
//                printf("Received 15:' %x size %d\n", buffer, nbrReceived);
                nbrReceived = receive(rx_uart16, buffer, 126, 1000);
                printf("Received 0:");
                for(int i=0; i<nbrReceived; i++)
                {
                        printf(" %02x ", buffer[i] & 0xff);
                }
                printf(" length %d\n",nbrReceived);
                for(int i=0; i<100; i++)
                {
                    usleep(15625);
                    nbrReceived = receive(rx_uart16, buffer, 126, 1000);
                    printf("Received 0:");
                    for(int i=0; i<nbrReceived; i++)
                    {
                            printf(" %02x ", buffer[i] & 0xff);
                    }
                    printf(" length %d\n",nbrReceived);
                }
                
                
                rx_uart1->Scc_Close();
//                rx_uart2->Scc_Close();
//                rx_uart3->Scc_Close();
//                rx_uart4->Scc_Close();
//                rx_uart5->Scc_Close();
//                rx_uart6->Scc_Close();
//                rx_uart7->Scc_Close();
//                rx_uart8->Scc_Close();
//                rx_uart9->Scc_Close();
//                rx_uart10->Scc_Close();
//                rx_uart11->Scc_Close();
//                rx_uart12->Scc_Close();
//                rx_uart13->Scc_Close();
//                rx_uart14->Scc_Close();
//                rx_uart15->Scc_Close();
                rx_uart16->Scc_Close();
                delete dut;

                cout << "Exiting!!!" << endl;
/*
                return 0;
*/


    
}


void *threadTest(void *arg)
{
    test();
    pthread_exit(NULL);
}


JNIEXPORT void JNICALL Java_serialdriver_SerialDriver_test
  (JNIEnv *, jobject)
{
//    pthread_t testThread;
//    pthread_create(&testThread,NULL,threadTest,NULL);
//    test();
//    pthread_exit(NULL);
    test();
}


/*
 * Class:     serialdriver_SerialDriver
 * Method:    initializeIOBoard
 * Signature: (S)Z
 */
JNIEXPORT jboolean JNICALL Java_serialdriver_SerialDriver_initializeIOBoard
  (JNIEnv *, jobject, jshort boardAddress)
{
    printf("initializeIOBoard\n");
    dut = new vme_32io(boardAddress);
    jboolean success;
    if (0x2ba2fd70 == dut->modelNumber)
    {
        cout << "The VME_32io has been found!\n";
        cout << "Resetting the outputs : setOutputs(0, 0xffff)";
        dut->setOutputs(0,0xffff);
        success = true;
    }
    else 
    {
        cout << "The VME_32io has not been found: exiting!!\n";
        success = false;
    }    
    return success;
}

/*
 * Class:     serialdriver_SerialDriver
 * Method:    terminateIOBoardSession
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_serialdriver_SerialDriver_terminateIOBoardSession
  (JNIEnv *, jobject)
{
    printf("terminateIOBoardSession\n");
    dut->~vme_IOboard();
}

/*
 * Class:     serialdriver_SerialDriver
 * Method:    setOutputs
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_serialdriver_SerialDriver_setOutputs__II
  (JNIEnv *, jobject, jint value, jint mask)
{
    //printf("setOutputs with Ints\n");
    dut->setOutputs(value, mask);
}


JNIEXPORT jint JNICALL Java_serialdriver_SerialDriver_getIntInputs
  (JNIEnv *, jobject)
{
    //printf("getInputs with int\n");
    int inputs = dut->getInputs();
    return inputs;    
}

int main()
{
    printf("Hello World\n");
//    pthread_t testThread;
//    pthread_create(&testThread,NULL,threadTest,NULL);
//    test();
//    pthread_exit(NULL);
    test();
}
