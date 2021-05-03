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
#include "vme_scc16.h"
#include "vmeBusAnalyzer.h"

using namespace std;

#define  MAX_BUFFER  256

vme_32io  * dut32;
vme_SCC16 * dut16;

vme_SCC16 * dutBA;


int receive(uart_channel * chan, char * buffer, int bufferSize);
int receive(uart_channel * chan, char * buffer, int bufferSize, int pollDelay);
int send(uart_channel * chan, const char* buffer, int buffsize);

UINT32 prev23;

UINT32 ticToc = 0;

/*
 * Class:     serialInterface_SerialInterface
 * Method:    openSerialPort
 * Signature: (Lserialdriver/SerialConfig;)J
 */
JNIEXPORT jlong JNICALL Java_serialInterface_SerialInterface_openSerialPort (JNIEnv *env, jobject callingClass, jobject config)
{
    jlong rv = 0;
    jmethodID method;
    jclass configClass;
    jint baudRate;
    char eolChar;
    int parityEnb;
    int port;
    int bufferConfig;
    int useRxFifo;
    int useTxFifo;
    
    // issue new against the address of the board
    vme_SCC16 * dut = new vme_SCC16(0x8000);   
    
    printf("SerialJNI::openSerialPort(0x2BA30540); Accessing vme_SCC16 Model 0x%x\n", dut->modelNumber);
    fflush(stdout);
        
    // check returned information against expected Model Number, proceed if models match
    if (0x2ba30540 == dut->modelNumber)
    {
        // extract UART channel configuration information from the config filer
        configClass = env->GetObjectClass(config);  
        method = env->GetMethodID(configClass,"getBaudRate","()I");      
        baudRate = env->CallIntMethod(config,method);                
        method = env->GetMethodID(configClass,"getEolChar", "()I");
        eolChar =(char)env->CallIntMethod(config,method);        
        method = env->GetMethodID(configClass,"getParity", "()I");
        parityEnb = env->CallIntMethod(config,method);        
        method = env->GetMethodID(configClass,"getPort", "()I");
        port = env->CallIntMethod(config,method);        
        method = env->GetMethodID(configClass,"getBufferConfig", "()I");
        bufferConfig = env->CallIntMethod(config, method);    
        method = env->GetMethodID(configClass,"getUseRxFifo", "()I");
        useRxFifo = env->CallIntMethod(config, method);        
        method = env->GetMethodID(configClass,"getUseTxFifo", "()I");
        useTxFifo = env->CallIntMethod(config, method);  
        printf("\nSerialJNI::Protocol: 0x%02x\n", bufferConfig);
        printf("SerialJNI::Baud Rate: %d\n", baudRate);
        printf("SerialJNI::EOL Char: 0x%02x\n", eolChar);
        printf("SerialJNI::Parity Enable: %d\n", parityEnb);
        printf("SerialJNI::Port: %d\n", port);
        printf("SerialJNI::Buffer Config: 0x%02x\n", bufferConfig);
        printf("SerialJNI::RxFifo: %d\n", useRxFifo);
        printf("SerialJNI::TxFifo: %d\n", useTxFifo);
        fflush(stdout);        
        
        // instantiate a UART configuration using the extracted data
        uart_channelConfig uartconfig;
        uartconfig.baudRate = baudRate;
        uartconfig.dataBits = 8;
        uartconfig.eolChar = eolChar;
        uartconfig.parity = 1;
        uartconfig.parityEnable = parityEnb;
        uartconfig.stopBits = 1;
        uartconfig.bufferConfig = bufferConfig;
        uartconfig.useRxFifo = (useRxFifo != 0);
        uartconfig.useTxFifo = (useTxFifo != 0);
        uart_channel * channel = new uart_channel(dut, &uartconfig, port);
        
// TDB DOES THIS SERVE A PURPOSE?
        char buffer[MAX_BUFFER];
        int nbrReceived = receive(channel, buffer, MAX_BUFFER);
        
        // print the result
        printf("SerialJNI::Port Opened %d\n", port);
        printf("SerialJNI::Channel %d\n", (jlong)channel);
        printf("SerialJNI::VME base address %d\n", (jlong)channel->uart->baseAddress);
        fflush(stdout);
        
        // return channel
        rv = (jlong)channel;
        
// IS THIS REALLY NECESSARY?
        // pause for 1 second
        usleep(1000000);
    }
    else 
    {
        cout << "The VME_SCC16 has not been found: exiting!!\n";
    }
            
    return rv;
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    closeSerialPort
 * Signature: (I[B)I 
 */
JNIEXPORT jint JNICALL Java_serialInterface_SerialInterface_closeSerialPort (JNIEnv *, jobject, jlong serialId)
{
    uart_channel *channel = (uart_channel*)serialId;
    printf("SerialJNI::Closing port %d\n", channel);
    int rv = channel->Scc_Close();
    delete channel;    
    return (jint)rv;    
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    writeSerialData
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_serialInterface_SerialInterface_writeSerialData (JNIEnv *env, jobject, jlong serialId, jbyteArray data, jint dataSize)
{    
    uart_channel* channel = (uart_channel*)serialId;

    jbyte* jbyteData = env->GetByteArrayElements(data, NULL);
    
    char* buffer = new char[dataSize];
    
    //Get Array
    for(int i=0; i<dataSize; i++)
    {
        buffer[i] = jbyteData[i];
    }

    int rv = send(channel, buffer, dataSize);
    
    delete[] buffer;
    
    env->ReleaseByteArrayElements(data, jbyteData, 0);
    
    return rv;    
}

JNIEXPORT jbyteArray JNICALL Java_serialInterface_SerialInterface_readMultipleSerialData
  (JNIEnv *env, jclass, jlongArray serialIdArray)
{
    jsize len = env->GetArrayLength(serialIdArray);
    char ** buffer= new char*[len];
    
    for(int i=0; i<len; i++)
    {
        buffer[i] = new char[MAX_BUFFER];
    }
        
    int *nbrReceived = new int[len];
    jlong *ptr = env->GetLongArrayElements(serialIdArray, 0);
    
    for(int i=0; i<len; i++)
    {
        uart_channel* channel = (uart_channel*)ptr[i];
        nbrReceived[i] = receive(channel, buffer[i], MAX_BUFFER);
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
            jbarray[jsize] = ':';
        }
        
        for(int j=0; j<nbrReceived[i]; j++, jsize++)
        {
            jbarray[jsize] = buffer[i][j];
        }
    }
    
    jbyteArray rv = env->NewByteArray(size);
    env->SetByteArrayRegion(rv,0,size,jbarray);
    delete [] jbarray;
    delete [] nbrReceived;
    
    for(int i=0; i<len; i++)
    {
        delete [] buffer[i];
    }
    
    delete [] buffer;
    
    env->ReleaseLongArrayElements(serialIdArray, ptr, 0);
    
    return rv;
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    readSerialData
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_serialInterface_SerialInterface_readSerialData__J(JNIEnv * env, jobject, jlong serialId)
{
    uart_channel* channel = (uart_channel*)serialId;
    char buffer[MAX_BUFFER];
    
    int nbrReceived = receive(channel, buffer, MAX_BUFFER);
    jbyte* jbarray = new jbyte[nbrReceived];
    
    for(int i=0; i < nbrReceived; i++)
    {
        jbarray[i] = buffer[i];
    }
    
    jbyteArray rv = env->NewByteArray(nbrReceived);
    
    env->SetByteArrayRegion(rv, 0, nbrReceived, jbarray);
    
    delete [] jbarray;
    
    return rv;
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    readSerialData
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_serialInterface_SerialInterface_readSerialData__JI(JNIEnv * env, jobject, jlong serialId, jint portDelay)
{
    uart_channel* channel = (uart_channel*)serialId;
    char buffer[MAX_BUFFER];
    
    int nbrReceived = receive(channel, buffer, MAX_BUFFER, (int)portDelay);
    jbyte* jbarray = new jbyte[nbrReceived];

    jbyteArray rv = env->NewByteArray(nbrReceived);
    
    env->SetByteArrayRegion(rv, 0, nbrReceived, jbarray);
    
    delete [] jbarray;
    
    return rv;
}

/*
 * Send serial data to the specified channel
 */
int send(uart_channel * chan, const char* buffer, int buffsize)
{
    int i=0;

    // Continue to send data until the TXFIFOSpace is full or the data buffer to be sent is complete
    while (chan->Scc_transmitReady() && i<buffsize)
    {
        char c = chan->Scc_putch(buffer[i]);

        //Error in transmission return status.
        if(c != buffer[i])
        {
            printf("WARNING: Serial write terminated prematurely\n");
            return c;
        }

        i++;
    }
    
    return i;
}

/*
 * Read serial data from the specified channel
 */
int receive(uart_channel * chan, char * buffer, int buffsize)
{
    int i=0;

    // Continue to read data until the RXFIFOSpace is full or the requested buffer size has been met
    while (chan->Scc_ReceiveReady() && i<buffsize)
    {
        char c = chan->Scc_getch();
        buffer[i] = c;
        i++;
    }
    
    // this may very well be bad ... buffer[i] = 0;
    
    return i;
}

int receive(uart_channel * chan, char * buffer, int buffsize, int pollDelay)
{
    int i=0;
    
    // Continue to read data until the RXFIFOSpace is full or the requested buffer size has been met
    while (chan->Scc_ReceiveReady() && i<buffsize)
    {
        char c = chan->Scc_getch(pollDelay);
        buffer[i] = c;
        i++;
    }
    
    // this may very well be bad ... buffer[i] = 0;
    
    return i;
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    initializeIOBoard
 * Signature: (S)Z
 */
JNIEXPORT jboolean JNICALL Java_serialInterface_SerialInterface_initializeIOBoard(JNIEnv *, jobject, jshort boardAddress)
{
    printf("SerialJNI::initialize VME_32io Board\n");
    fflush(stdout);
    
    dut32 = new vme_32io(boardAddress);
    jboolean success;
    
    // check returned information against expected Model Number
    if (0x2ba2fd70 == dut32->modelNumber)
    {
        cout << "The VME_32io has been found!\n";
        success = true;
    }
    else 
    {
        cout << "The VME_32io has not been found: exiting!!\n";
        success = false;
    }
    
    fflush(stdout);
    
    return success;
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    terminateIOBoardSession
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_serialInterface_SerialInterface_terminateIOBoardSession(JNIEnv *, jobject)
{
    dut32->~vme_IOboard();
}


/*
 * Class:     serialInterface_SerialInterface
 * Method:    initializeIOBoard
 * Signature: (S)Z
 */
JNIEXPORT jboolean JNICALL Java_serialInterface_SerialInterface_initializeSCC16(JNIEnv *, jobject, jshort boardAddress)
{
    printf("SerialJNI::initialize SSC16 Board\n");
    fflush(stdout);

    jboolean success = true;
    
    // issue new against the address of the board
    vme_SCC16 * dut16 = new vme_SCC16(0x8000);   
    
    printf("SerialJNI::openSerialPort(0x2BA30540); Accessing vme_SCC16 Model 0x%08x\n", dut16->modelNumber);
    fflush(stdout);
  
    // check returned information against expected Model Number, proceed if models match
    if (0x2ba30540 != dut16->modelNumber)    
    {
        cout << "The SSC16 has not been found: exiting!!\n";
        success = false;
    }
    
    fflush(stdout);
    
    return success;
}


/*
 * Class:     serialInterface_SerialInterface
 * Method:    setOutputs
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_serialInterface_SerialInterface_setOutputs(JNIEnv *, jobject, jint value, jint mask)
{
    // check returned information against expected Model Number
    if (0x2ba2fd70 != dut32->modelNumber)
    {    
        printf("\nJava_serialInterface_SerialInterface_setOutputs(SEVERE); NOT a VME32IO\n");
        fflush(stdout);
    }
    
    dut32->setOutputs(value, mask);
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    getIntInputs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_serialInterface_SerialInterface_getIntInputs(JNIEnv *, jobject)
{
    int inputs = dut32->getInputs();
    
    // check returned information against expected Model Number
    if (0x2ba2fd70 != dut32->modelNumber)
    {    
        printf("\nJava_serialInterface_SerialInterface_getIntIputs(SEVERE); NOT a VME32IO\n");
        fflush(stdout);
    }    
    
    return inputs;    
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    getOutputs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_serialInterface_SerialInterface_getOutputs(JNIEnv *, jobject)
{  
    // check returned information against expected Model Number
    if (0x2ba2fd70 != dut32->modelNumber)
    {    
        printf("\nJava_serialInterface_SerialInterface_getOutputs(SEVERE); NOT a VME32IO\n");
        fflush(stdout);
    }    
    
    return dut32->getOutputs();
}

/*
 * Class:     serialInterface_SerialInterface
 * Method:    E_Stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_serialInterface_SerialInterface_EStop(JNIEnv *, jobject)
{
    vme_SCC16 * dut = new vme_SCC16(0x8000);
    dut->E_Stop();
}


/*
 * Class:     serialInterface_SerialInterface
 * Method:    getVMEstatus
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_serialInterface_SerialInterface_getVMEstatus(JNIEnv *, jobject)
{
    jint rc = 0x00;
           
    
    /*  DEBUG  */
//    printf("\n\tgetVMEstatus(); timeStamp: 0x%08x\n", dutBA->vme->baseAddress[0x18]);
//    fflush(stdout);
//        
//    if(dutBA->vme->baseAddress[0x18] != 0)
//    {            
//        printf("\n\tgetVMEstatus(); ... data acquisition occurred...\n");
//        
//        for (int i = 0x10; i < 0x18; i++)
//        {   
//            printf("\n\tgetVMEstatus(); address # 0x%02x: 0x%08x", i, dutBA->vme->baseAddress[i]);
//        }        
//
//        printf("\n\n");
//        fflush(stdout);        
//    }            
//      
//        
//    printf("\n\t<+>  getVMEstatus(); prev23 0x%08x and now 0x%08x  <+>\n", prev23, dutBA->vme->baseAddress[0x17]);
//    fflush(stdout);
    /*  DEBUG  */

    
    if(prev23 != dutBA->vme->baseAddress[0x17])
    {
//        printf("\n\t<->  getVMEstatus(); Bus Error at 0x%08x  <->\n", dutBA->vme->baseAddress[0x14]);
//        fflush(stdout);
        
        rc = dutBA->vme->baseAddress[0x14];
    }

    prev23 = dutBA->vme->baseAddress[0x17];
        
    return rc;
}


/*
 * Class:     serialInterface_SerialInterface
 * Method:    initVMEanalyzer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_serialInterface_SerialInterface_initVMEanalyzer(JNIEnv *, jobject)
{
    dutBA = new vme_SCC16(0x8000);

    printf("\ninitVMEanalyzer(); baseAddress 0x%08x\n", dutBA->vme->baseAddress);
    fflush(stdout);
        
    UINT32 masks[4] = {0,0,0,0};
    masks[2] = 1 << VME_OFFSET_BERR_N; // this says to trigger on a falling Bus Error edge        
    // masks[2] = 1 << VME_OFFSET_AS_N; // this says to trigger on address strobe        
    
    printf("\ninitVMEanalyzer(); masks[2]: 0x%08x\n", dutBA->vme->baseAddress[10]);
    fflush(stdout);
    
    dutBA->vme->setEdgeMask(masks);
    dutBA->vme->setEdgeTrigger(masks);
    
    prev23 = dutBA->vme->baseAddress[0x23];
}


/*
 * Class:     serialInterface_SerialInterface
 * Method:    getPortStatus
 * Signature: (J)I  
 */
JNIEXPORT jint JNICALL Java_serialInterface_SerialInterface_getPortStatus (JNIEnv *, jobject, jlong serialId)
{
    uart_channel * channel = (uart_channel *)serialId;
    
    printf("SerialJNI::Port Status port %d\n", channel);
    fflush(stdout);
    
    UINT32 addr = 0x2710; // 10000
    UINT8 tmp;
    int rv; 
    
    // 10000 (0x2710) was 0
    channel->uart->sccRegisterRead(addr, &tmp);
    tmp &= ~(Rx_CH_AV | Tx_BUF_EMP);
    
    return (jint)tmp;    
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                           test and DEBUG code                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/*
 * Class:     serialInterface_SerialInterface
 * Method:    genBusError
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_serialInterface_SerialInterface_genBusError(JNIEnv *, jobject, jint addr, jboolean loop)
{
    vme_32io * testDut;
    
    if(loop)
    {
        testDut = new vme_32io(0xBAD0);
        int i = 0;

        for(;;) 
        {
            /*
             * VME bus error at
             * 
             *    0xBAD0 + 0x40 >>===> 0xBB10
             *    0xBAD0 + 0x44 >>===> 0xBB14
             * 
             * What we see at the bus analyzer
             * when running this section
             *
             */

            testDut->getInputs();

            if(i == 1111)
            {
                printf("Java_serialInterface_SerialInterface_genBusError(); forever...\n");
                fflush(stdout);

                i = 0;
            }

            ++i;
        }
    }
    else
    {
        printf("Java_serialInterface_SerialInterface_genBusError(); one ping only, ticToc is %d...\n", ticToc);
        fflush(stdout);
               
        if(ticToc % 2)
        {
            /*  ... one ping only...  */
            dutBA->vme->baseAddress[0x1000] = 0; // creates SSC16 based VME Bus Error        
        }
        else
        {        
            /*  ... one ping only...  */
            testDut = new vme_32io(0xBAD0);      // creates 32_IO based VME Bus Error
        }
        
        ++ticToc;
    }
}


void test()
{
    vme_SCC16 * dut = new vme_SCC16(0x8000);
    char buffer[128];
    uart_channelConfig config;
    int i;
    int nbrReceived;

    usleep(1000000);

    // connect to the VME SCC 16
    if (0x2ba30540 == dut->modelNumber)
    {
        cout << "The VME_SCC16 has been found!\n";
    }
    else
    {
        cout << "The VME_SCC16 has not been found: exiting!!\n";
        return;
    }
    printf("SerialJNI::Model number: %d (0x%08x)\n", dut->modelNumber, dut->modelNumber);

    // set the UART configuration
    config.baudRate = 9600;
    config.dataBits = 8;
    config.eolChar = '\n';
    config.parity = 1;
    config.parityEnable = 0;
    config.stopBits = 1;
    config.bufferConfig = uart_channel::RS_422_buffers;
    config.useRxFifo = false;
    config.useTxFifo = false;

    // Print  UART configuration
    printf("SerialJNI::Baud Rate  %d bps \n", config.baudRate );        

    // We are going to use an outside loopback cable between SCC #2 and SCC #3
    uart_channel * rx_uart1 = new uart_channel(dut, &config, 1);

    printf("SerialJNI::1 Baud Rate  %d bps \n", config.baudRate );
    printf("SerialJNI::Sleep Start\n");
    usleep(1000000);
    printf("SerialJNI::Sleep End\n");

    uart_channel *rx_uart16 = new uart_channel(dut, &config, 0);
    printf("SerialJNI::16 Baud Rate  %d bps \n", config.baudRate );
    bool running = true;

    nbrReceived = receive(rx_uart1, buffer, 126);

    printf("Received 1: ");
    for(int i=0; i < nbrReceived; i++)
    {
        printf(" %02x ", buffer[i] & 0xff);
    }
    printf("\n");

    nbrReceived = receive(rx_uart16, buffer, 126);
    printf("SerialJNI::Received 0:");
    for(int i=0; i<nbrReceived; i++)
    {
        printf("SerialJNI:: %02x ", buffer[i] & 0xff);
    }
    printf("SerialJNI:: length %d\n",nbrReceived);

    rx_uart1->Scc_Close();
    rx_uart16->Scc_Close();
    delete dut;

    cout << "Exiting!!!" << endl;
    
    fflush(stdout);
}

void *threadTest(void *arg)
{
    test();
    pthread_exit(NULL);
}

JNIEXPORT void JNICALL Java_serialInterface_SerialInterface_test(JNIEnv *, jobject)
{
    test();
}


/*
 *  Ooo, main()
 * 
 */
int main()
{
    test();
}