/*       Main.c
 *       Holt HI-3717 ARINC 717 Demonstration Program
 *
 *
 *      	This file demonstrates a simple ANSI C program structure for
 *			the initializing and using a Holt HI-3717 device.
 *
 *       Tool: Codewarrior 5.9.0 (build 2836)
 *
 *			HOLT DISCLAIMER
 *
 *			THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *			EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES
 *			OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *			NONINFRINGEMENT.
 *			IN NO EVENT SHALL HOLT, INC BE LIABLE FOR ANY CLAIM, DAMAGES
 *			OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *			OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *			SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *			Copyright (C) 2009 - 2010 by HOLT, Inc.
 *			All Rights Reserved.

 *       Revision 1.0   8/24/2011. WS.
         Revision 1.1   9/17/2013. Fix 4096 bit rate.
         Revision 1.2   1/8/2014. Added Mode 6, Software Synchronization Mode to emulate proper subframe SNYC1:0 bits on
            Freescale MCU output pins PT5:PT4 (base board J1 pins 3&4).

         Revision 1.3   3/12/15. Update 3.75us slew rate for 1024 WPS rate.

 */

 /*
  You may see compiler warning like these and they are consider normal:
  Warning : C12056: SP debug info incorrect because of optimization or inline assembler
 */

 /* This change was made to the Project.prm file to gain access to another 4k SRAM space:
  12K SRAM is not available.
   non-paged RAM
       RAM           = READ_WRITE  DATA_NEAR            0x2000 TO   0x3FFF;           Was
       RAM           = READ_WRITE  DATA_NEAR            0x1000 TO   0x3FFF;           Modified to
 */

 /* ------------------------Definitions of the LEDs and the Dip Switches---------------------------


  LED1 - flashes to indicate a sync word was loaded into the TX FIFO.
  LED2 - flashes to indicate a sync word or data word that happens to match a sync word is received.
  LED3 - flashes to indicate any word is received from the RX FIFO.
  LED4 - Turns on when the word count matches the value in the RX match count register.

  LED5 - On indicates Receiver FIFO Full
  LED6 - On indicates Transmitter FIFO Full
  LED7 - Processor running indicator flashes at 1 second.
  LED8 - Red to indicate HI-3717 failed to initialize or to indicate when a data compare does not match.

 DIP SWITCH ASSIGNMENTS ON THE SPI MAIN BOARD

 MODE0 - MODE7 MODES:
    MODE0 = BOARD TEST
    MODE1 = TRANSMIT/RECEIVE
    MODE2 = TRANSMIT/RECEIVE IN TEST MODE
    MODE3 = TRANSMIT SPECIAL PATTERNS
    MODE4 = RECEIVE WITH NO COMPARE
    MODE5 = RECEIVE WITH COMPARE
    MODE6 = NOT USED
    MODE7 = SERIAL COMMANDS


 BITRATE0-BITRATE2 7 RATES:
    0 = 64
    1 = 128
    2 = 256
    3 = 512
    4 = 1024
    5 = 2048
    6 = 4096
    7 = 8192


 OPT1 = RXSEL
        OPEN = BPRZ
        CLOSED = HBP

 OPT2 = 32 WPS (THIS OVERRIDES OTHER RATE SWITCHES)
        OPEN = 32WPS
        CLOSED = WORD RATE IS SELECTED BY BITRATE0-BITRATE7 SWITCHES

 Some of the signals from the HI3717 connect to the main board Freescale processor.

  MC9S12XDT HI3717 signals

  PP0        RFIFO
  PP1        MATCH
  PP2        ROVF
  PP3        TFIFO
  PP7        TEMPTY

  PA3        INSYNC

 ----------------------------------------------------------------------------------------------*/


#define NEWBOARD 1            // 0= old board. Accounts for LED7 reversed logic.

#define SW_REVISION 0x13      // Software Revision to display on the LEDs and console
 // format [Revision, Sub revision] for eg. 0x12 = Rev 1.2.       
#include "stdint.h"
#include <string.h>
#include <stdio.h>                           
#include "main.h"
#include "Uart.h"

// hardware for now
void EnableInterrupts(void);
void DisableInterrupts(void);

int UartConfig(int bps);

uint8_t LED1;
uint8_t LED2;
uint8_t LED3;
uint8_t LED4;
uint8_t LED5;
uint8_t LED6;
uint8_t LED7;
uint8_t LED8;
#define LED_1   1
#define LED_2   2
#define LED_3   3
#define LED_4   4
#define LED_5   5
#define LED_6   6
#define LED_7   7
#define LED_8   8

#define ON  1
#define OFF 0

uint8_t PORTA;
uint8_t PORTB;
uint8_t PORTE;
uint8_t PORTT;

uint8_t PIFP_PIFP0 = 1; 
uint8_t PIFP_PIFP1 = 1;   
uint8_t PIFP_PIFP2 = 1;   
uint8_t PIFP_PIFP3 = 1;    

uint8_t SW1;
uint8_t SW2;
uint8_t SW3;
uint8_t SW4;

uint32_t TC0;
#define LO 0
#define HI 0xffffffff
uint32_t PP4;
uint8_t PTT_PTT4;
uint8_t PTT_PTT5;
uint8_t SPI0BR;
uint32_t PP5;
uint32_t PIFP;

#define BUTTON1 1

// * Global Variables and arrays *
uint8_t DebugArray[16];                        // Global array for 3717 status registers
const char WelcomeMesg[] = { "Holt HI-3717 Demonstration." };

char DebugArrayStrings[][34] = {              // Status register dump message headers
   "\n\rControl Reg 0 \t\t",
   "Control Reg 1 \t\t",
   "FIFO Status Reg \t",
   "FIFO XMT Reg \t\t",
   "REC FIFO Status Pin Reg ",
   "FAST REC REC FIFO",
};

char BitRateStrings[][12] = {                 // Strings for BitRate console output

   "64 WPS",
   "128 WPS",
   "256 WPS",
   "512 WPS",
   "1024 WPS",
   "2048 WPS",
   "4096 WPS",
   "8192 WPS",
   "32 WPS",

};


const unsigned int wordRates[9] =
{
   64, 128, 256, 512, 1024, 2048, 4096, 8192, 32
};

// Receiver variables
uint16_t RXBufferInt[4];              // Two 16 bit words. Data and Word Count

uint16_t FIFOBufferInt[4096];         // Large array to store last received messages
unsigned int MessageCount = 0;              // count and pointer into the FIFOBufferInt array
unsigned int g_wordCounter;
uint16_t TXBufferInt[2];              // Transmit Buffer
unsigned int g_max;                       // Max word count


volatile unsigned int g_count100us;       // Global timer tick for delays.
volatile int g_tick100us;                 // Timer function
volatile char g_ledFlashBool;             // Global LED8 flash enable, YES or NO. 
unsigned char MODES, OPTION;
const uint16_t SYNC1 = 0x0247;
const uint16_t SYNC2 = 0x05B8;
const uint16_t SYNC3 = 0x0A47;
const uint16_t SYNC4 = 0x0DB8;


unsigned int sync[] = {                   // ARINC 717 Barker Sync Word Values
    0x247,		// SW 1
    0x5b8,		// SW 2
    0xa47,		// SW 3
    0xdb8		   // SW 4
};

uint8_t syncPointer;                     // receiver next sync pointer
unsigned int syncLog[4] = { 0,0,0,0 };	// log total sync words received including if found in data
unsigned char syncPointerTX = 0;
unsigned int MessageCount_R = 0;
unsigned char syncPointer_R = 0;

unsigned char rxfstat;                 // HI-3717 RX Status Reg last read
unsigned char txfstat;                 // HI-3717 TX Status Reg last read

// Test Variables
unsigned int g_SentCount;              // Total number of TX FIFO loads
unsigned int g_ReceivedCount;          // Total number of RX FIFO reads

Arinc3717Test *dut;

//------------------------ MAIN ------------------------------------------------
int main() {
    uint8_t brdNbr = 0;
    PCIeMini_status st;
    time_t ts;
	char buff[20];

    dut = new Arinc3717Test(brdNbr);
    dut->setVerbose(0);
    st = dut->open(brdNbr);

    std::cout << "Opening the PCIeMini_ARINC717_IO: " << getAlphiErrorMsg(st) << std::endl;
    if (st != ERRCODE_NO_ERROR) {
        std::cout << "Exiting: Press <Enter> to exit." << std::endl;
        getc(stdin);
        exit(0);
    }

    uint32_t id = dut->getFpgaID();
    std::cout << "FPGA ID: 0x0" << std::hex << id << std::endl;
    if (id == 0xffffffff || id == 0) {
        std::cout << "Cannot read the board!" << std::endl;
        std::cout << "Exiting: Press <Enter> to exit." << std::endl;
        getc(stdin);
        exit(0);
    }

    ts = dut->getFpgaTimeStamp();
    std::cout << "FPGA Time Stamp: " << ts << " -- ";
    strftime(buff, 20, "%m/%d/%Y %H:%M:%S", localtime(&ts));
    printf("%s\n", buff);

    std::cout << std::endl << "---------------------------------" << std::endl;
    std::cout << "PCIeMini-ARINC717_IO Confidence Test" << std::endl;
    std::cout << "Using board number #" << (char)('0' + brdNbr) << std::endl;
    if (dut->verbose) std::cout << "verbose mode " << std::endl;
    std::cout << "---------------------------------" << std::endl;

    g_ledFlashBool = OFF;           // Turn off the LED7 active 1 second blinker

    dut->PeriphInit();                   // initialize PLL, SPI Clocks and GPIO                

    dut->InitInterrupts();          // initialize interrupts

    dut->Timer_Init();                  // initialize T0 for the main 100us periodic system interrupt
                                   //clear all possible false interrupts just in case
    PIFP_PIFP0 = 1;                //clear PPO TRFO - Not Enabled 
    PIFP_PIFP1 = 1;                //clear PP1 MATCH Interrupt is Enabled for this demo 
    PIFP_PIFP2 = 1;                //clear PP2 ROVF - Not Enabled 
    PIFP_PIFP3 = 1;                //clear PP3 TFIFO - Not Enabled  

    // Display the software revision on the LEDs for 1 second
    PORTT = ~SW_REVISION;             // write out to LEDs1-4 
    PORTE = SW_REVISION;              // write out to LEDs5-8 
    LED_CTL(LED_8, OFF);

    dut->EnableInterrupts();
    Sleep(K_1SEC);             // delay time to show the SW revision on the LEDs
    Sleep(K_1SEC);             // delay time to show the SW revision on the LEDs



    LED_CTL(LED_1, OFF);             // Turn off LED1-LED8s
    LED_CTL(LED_2, OFF);
    LED_CTL(LED_3, OFF);
    LED_CTL(LED_4, OFF);
    LED_CTL(LED_5, OFF);
    LED_CTL(LED_6, OFF);
    LED_CTL(LED_7, OFF);
    LED_CTL(LED_8, OFF);               // LED is Red
    g_ledFlashBool = ON;              // Turn ON the LED7 blinker

    //UartConfig(b115200);              // configure the UART, b230400=115200BAUD or b115200=115200BAUD, 8 bits, no parity
    putchar(XON);                       // Turn off software flow control just in case
    putchar(NEWPAGE);                   // clear the console screen

    if (!SW4)                        // Set SPI to 10Mhz if SW4 pressed at power up
    {
        SPI0BR = 0x01;                //  10Mhz SPI
        printf("SPI 10MHz\n");
    }


    printf("\n\rHolt HI-3717 Demonstration Software Revision: %x.%x", SW_REVISION >> 4, SW_REVISION & 0x0F); // print the SW sub revision

    PTT_PTT4 = 0;     // clear the emulated Sync status bits
    PTT_PTT5 = 0;     // clear the emulated Sync status bits


    printf("\n");

    HW_RESET();                         // issue a hardware reset to the HI-3717

    for (;;) {

        MODES = MODE_SELECT();                    // Read and save Mode switch settings

        switch (MODES) {                          // MODES = 0-7. See Function headers for DIP Switch and SW Button options.

        case BOARD_MODE:                     // 0  Cycle the LEDs as a simple board test
            dut->BoardTest();
            break;

        case TRANSMIT_RECEIVE_MODE:          // 1  Transmit an incrementing counter value 
            dut->Transmit_Receive_Mode(SELFTEST_OFF);
            break;

        case TRANSMIT_RECEIVE_SELFTEST_MODE: // 2 Transmit an incrementing counter value TEST (LoopBack Mode)
            dut->Transmit_Receive_Mode(SELFTEST_ON);
            break;

        case TRANSMIT_SPECIAL_MODE:          // 3 TRANSMIT SPEICAL PATTERNS, No Receiving
            dut->Transmit_SPECIAL_Mode();
            break;

        case RECEIVE_MODE:                   // 4 RECEIVE MODE only
            dut->Receive_Mode(COMPARE_OFF);
            break;

        case RECEIVE_COMPARE_MODE:           // 5 RECEIVE MODE only with COMPARE (use with another board with Mode 2)
            dut->Receive_Mode(COMPARE_ON);
            break;

        case SFTSYNC_MODE:                   // 6 SOFTWARE SYNC MODE demo
            dut->SftSync_Mode_Demo(SELFTEST_OFF);   // Outputs new SYNC bits on PT5 and PT4.   
            break;

        case SERIAL_COMMANDS:
            dut->SerialCommands();             // USE THIS FOR SPECIAL TESTING                       
            break;

        default:break;
        };

    }; // for

    return 0;
}	// main end


// ------------------------------------------------------
// Serial Commands - simple menu to exercise test or special functions
// ------------------------------------------------------
void Arinc3717Test::SerialCommands(void)
{
    for (;;) {

        char ch;
        putchar(NEWPAGE);

        for (;;) {
            printf("%s","\n\r ******* HOLT HI-3717 Serial Console Commands ******* \n");
            printf("%s","\n\r Input String Demo                0");
            printf("%s","\n\r Reset HI-3717 using the MR pin   1");
            printf("%s","\n\r Control and Status Register Dump 2");
            printf("%s","\n\r Software Reset HI-3717           3");
            printf("%s","\n\n\r Enter Selection: ");

            do {
                ch = getchar();
            } while (ch == EOF);

            putchar(ch);
            printf("\n");


            switch (ch)
            {
            case '0':
            {
                // This demonstrates how to read multiple in characters from the serial port.
                // This can be useful to implement additional commands which require input from the console.                     
                char buffer[40];
                printf("%s","\n\r Enter a string then press Return\n\r");
                (void)memset(buffer, 0, sizeof(buffer));
                scanf("%s", buffer);
                printf("\n");
                printf("%s","Input was: ");
                printf("%s",buffer);
                printf("\n");
            }
            break;
            case '1':
            {
                HW_RESET();
                printf("%s","\n\r 1 Hardware Reset Generated\n\r");
            }
            break;

            case '2':

                printf("%s","\n\rHI-3717 Status and Control Registers\r\n");
                DisableInterrupts();
                DisplayStatusRegisters();       // display the registers
                EnableInterrupts();
                break;

            case '3':

                printf("%s","\n\rHI-3717 Software Reset\r\n");
                DisableInterrupts();
                arinc->softReset3717();
                EnableInterrupts();

                break;


                break;

            default: break;
            } // switch

        }  // for
    }   //for
}

// Several of these functions don't have prototypes. They are placed here before use
// as a convience so that the user can make quicker modifications to the sub-frame content
// ------------------------------------------------------
// Load the TX FIFO with data
// ------------------------------------------------------
void Arinc3717Test::TxWord(unsigned int ArincWord)
{

#if (1)       
    while (0x80 & arinc->R_Register(R_FIFO_XMT_REG));     // wait for FIFO not full before continuing
#else
    while (0x08 & PORTB);           // check the PB3 port pin instead
#endif

    TXBufferInt[0] = ArincWord;
    arinc->TransmitCommandAndDataWords(W_XMTFIFO, TXBufferInt);

}

// ------------------------------------------------------
// Transmits Zeros. Can be modified to any data with additional case statements.
// ------------------------------------------------------
void Arinc3717Test::GoodSubFrame0(unsigned int sync)
{
    unsigned int wordCounter = 0;
    while (wordCounter < g_max)
    {
        switch (wordCounter)
        {
        case 0:
            TxWord(sync);
            PORTA ^= 0x80;
            break;

        default:

#if 1      // put a 1 here to send 000 ,or 0 to send the wordCounter value as data     
            TxWord(0x0000);
#else            
            TxWord(wordCounter);
#endif                      
            break;
        };
        wordCounter++;
    }; // while 
}
// ------------------------------------------------------
// Transmits 1 subframe with 1's
// ------------------------------------------------------
void Arinc3717Test::GoodSubFrame1(unsigned int sync)
{
    unsigned int wordCounter = 0;
    while (wordCounter < g_max)
    {
        switch (wordCounter)
        {
        case 0:
            TxWord(sync);
            PORTA ^= 0x80;
            break;

        default:
            TxWord(0x0FFF);

            break;
        };
        wordCounter++;
    }; // while 
}
// ------------------------------------------------------
// Transmits 1 subframe with an incrementing data pattern
// ------------------------------------------------------
void Arinc3717Test::GoodSubFrameC(unsigned int sync)
{
    unsigned int wordCounter = 0;
    while (wordCounter < g_max)
    {
        switch (wordCounter)
        {
        case 0:
            TxWord(sync);
            PORTA ^= 0x80;
            break;

        default:

#if 0      // put a 1 here to send 000 ,or 0 to send the wordCounter value as data     
            TxWord(0x0000);
#else            
            TxWord(wordCounter);
#endif                      
            break;
        };
        wordCounter++;
    }; // while 
}



// ------------------------------------------------------
// Transmits zeros for 4 subframes
// ------------------------------------------------------
void Arinc3717Test::GoodFrame0(void)
{
    GoodSubFrame0(SYNC1);
    GoodSubFrame0(SYNC2);
    GoodSubFrame0(SYNC3);
    GoodSubFrame0(SYNC4);
}
// ------------------------------------------------------
// Transmits 1's for 4 subframes
// ------------------------------------------------------
void Arinc3717Test::GoodFrame1(void)
{
    GoodSubFrame1(SYNC1);
    GoodSubFrame1(SYNC2);
    GoodSubFrame1(SYNC3);
    GoodSubFrame1(SYNC4);
}

// ------------------------------------------------------
// Transmits a counting data pattern for 4 subframes
// ------------------------------------------------------
void Arinc3717Test::GoodFrameC(void)
{
    GoodSubFrameC(SYNC1);
    GoodSubFrameC(SYNC2);
    GoodSubFrameC(SYNC3);
    GoodSubFrameC(SYNC4);
}

// ------------------------------------------------------
// Transmits zeros in the data
// ------------------------------------------------------
// Transmit special values - blocking code
void Arinc3717Test::TransmittAllZeros()
{
    GoodFrame0();
    LED1 ^= 1;

}

// ------------------------------------------------------
// Transmits all 1's
// ------------------------------------------------------
// Transmit special values - blocking code
void Arinc3717Test::TransmitAll1()
{
    GoodFrame1();
    LED1 ^= 1;
}

// ------------------------------------------------------
// Transmits 0's for subframes 1 and 3 and 1's for subframe 2 
// and an incrementing data pattern for subframe 4
// ------------------------------------------------------
// Transmit special values - blocking code
void Arinc3717Test::TransmitFrameSpecial()
{
    GoodSubFrame0(SYNC1);     // 0's
    LED1 ^= 1;
    GoodSubFrame1(SYNC2);     // 1's
    LED1 ^= 1;
    GoodSubFrame0(SYNC3);     // 0's
    LED1 ^= 1;
    GoodSubFrameC(SYNC4);     // Incrementing data
    LED1 ^= 1;

}


// ------------------------------------------------------
// Transmit Special Data Patterns. No Receiver function.
// ------------------------------------------------------
void Arinc3717Test::Transmit_SPECIAL_Mode(void)
{

    char ch, mode;                   // console character
    uint8_t rxsel, wps32, wordRate;

    MessageCount = 0;

    printf("%s","\n\rTransmit Special Mode\n\r");
    printf("%s","\n\rInitializing the HI-3717\r\n");

    // BPRZ or HBP? 
    if (OPT1 & PORTA)
    {
        rxsel = 1;
        printf("%s","\n\rReceiver Mode: BPRZ");
    }
    else
    {
        rxsel = 0;
        printf("%s","\n\rReceiver Mode: HBP");
    }

    wordRate = PORTB & 0x07;       // get all bitrate switches 
    printf("%s","\n\rWord Rate = ");

    // 32WPS?       
    if (OPT2 & PORTA)
    {
        wps32 = 1;
        g_max = wordRates[8];
        printf("%s",BitRateStrings[8]);
        printf("%s","\n\rSlew Rate 7.5uS Selected");
        printf("\n");
    }
    else
    {
        wps32 = 0;
        printf("%s",BitRateStrings[wordRate]);
        g_max = wordRates[wordRate];
        if (wordRate > 5)
            printf("%s","\n\rConsole output disabled due to speed > 2048WPS");
        printf("\n");
        if (wordRate > 4)
            printf("%s","Slew Rate 1.5us Selected");
        else if (wordRate == 3)
            printf("%s","Slew Rate 3.75us Selected");
        else
            printf("%s","Slew Rate 7.5us Selected");

    }
    printf("\n");

    g_wordCounter = 0;
    syncPointer = 0;

    // Pass these param to the init function: TEST mode, wordRate, 32WPS, RXSEL               
    if (INIT_FAIL == arinc->Init3717(0, wordRate, wps32, rxsel))
    {
        LED_CTL(LED_8, ON);              // turn on RED LED if failed
        printf("%s","** Failed to Initialize the HI-3717!!! **\n\r");
        for (;;);
    }
    else
    {
        printf("%s","HI-3717 Initialized OK\n\r");
    }


    // The Match Pin will generate an interrupt where the LED4 will be turned. 
    // LED4 will be turned off at the end of each frame in this main code below.


    printf("%s","\n\rPress S2 or '1' on the console any time to transmit Zeros");
    printf("%s","\n\rPress S3 or '2' on the console any time to transmit Ones");
    printf("%s","\n\rPress S4 or '3' on the console any time to transmit Zeros/Onces/Zeros/Increment\n\r\n");

    mode = '3';    // default to special data    

    for (;;) {
        ch = getchar();

        if (!SW1 || ch == ' ')                         // Displaythe status/control registers

        {
            printf("%s","\n\rHI-3717 Status, Control Registers\r\n");
            DisableInterrupts();
            DisplayStatusRegisters();               // Display the status/control registers
            EnableInterrupts();
            printf("%s","Press SW4 or spacebar to continue");
            while (SW4 && ' ' != getchar());        // start again if SW-4 pressed/spacebar
            printf("%s","\n\rTransmitting...\n\r");
        }

        if (!SW2 || ch == '1')                       // transmit zeros

        {
            mode = '1';
        }

        if (!SW3 || ch == '2')                       // transmit ones

        {
            mode = '2';
        }

        if (!SW4 || ch == '3')                       // transmit zeros,ones,zeros,incrementing

        {
            mode = '3';
        }


        switch (mode) // change this number   
        {
        case '1':
            printf("%s","Transmitting Zeros \n\r");
            TransmittAllZeros();
            break;

        case '2':
            printf("%s","Transmitting Ones \n\r");
            TransmitAll1();

            break;

        case '3':
            printf("%s","Transmitting Zeros,Ones, Zeros, and Incrementing \n\r");
            TransmitFrameSpecial();
            break;

        }

        PORTB ^= 0x08;

    };

}


// ------------------------------------------------------
// TRANSMIT RECEIVE MODE 
// Available SW Button commands:
//    S1    Push to start the transmission/receptions initially
//
//    S1    Display the status registers. Press SW-4 to continue..
//    OPT1 DIP SW     BPRZ = open, HBP = closed
//    OPT2 DIP SW     32WPS = open
//    BITR0 - BITR2 select bit rates 64-8192WPS. OPT2 32WPS will override these if open!    
//    Console Commands:
//       Pressing the space bar will fetch and display the Control and Status 
//       registers on the console. Press space bar again to resume.
// ------------------------------------------------------
void Arinc3717Test::Transmit_Receive_Mode(const uint8_t SELFTEST) {
    static unsigned char Status_F;

    char ch;                   // console character
    uint8_t rxsel, wps32, wordRateSwitches;

    MessageCount = 0;

    printf("%s","\n\rTransmit and Receive Mode\n\r");

    if (SELFTEST)
        printf("%s","\n\rSELF-TEST, Internal Digital Loop-back, No data is transmitted externally\n\r");

    printf("%s","\n\rInitializing the HI-3717\r\n");


    // BPRZ or HBP? 
    if (OPT1 & PORTA)
    {
        rxsel = 1;
        printf("%s","\n\rReceiver Mode: BPRZ");
    }
    else
    {
        rxsel = 0;
        printf("%s","\n\rReceiver Mode: HBP");
    }

    wordRateSwitches = PORTB & 0x07;       // get all bitrate switches 
    printf("%s","\n\rWord Rate = ");

    // 32WPS?       
    if (OPT2 & PORTA)
    {
        wps32 = 1;
        g_max = wordRates[8];
        printf("%s",BitRateStrings[8]);
        printf("%s","\n\rSlew Rate 7.5uS Selected");
        printf("\n");
    }
    else
    {
        wps32 = 0;
        printf("%s",BitRateStrings[wordRateSwitches]);
        g_max = wordRates[wordRateSwitches];

        if (g_max > 2048)
            printf("%s","\n\rConsole ARINC data output disabled due to speed > 2048WPS");
        printf("\n");
        if (g_max > 1024)
            printf("%s","Slew Rate 1.5us Selected");
        else if (g_max == 1024)
            printf("%s","Slew Rate 3.75us Selected");
        else
            printf("%s","Slew Rate 7.5us Selected");

    }
    printf("\n");

    g_wordCounter = 0;
    syncPointer = 0;

    // Pass these param to the init function: TEST mode, wordRate, 32WPS, RXSEL               
    if (INIT_FAIL == arinc->Init3717(SELFTEST, wordRateSwitches, wps32, rxsel))
    {
        LED_CTL(LED_8, ON);              // turn on RED LED if failed
        printf("%s","** Failed to Initialize the HI-3717!!! **\n\r");
        for (;;);
    }
    else
    {
        printf("%s","HI-3717 Initialized OK\n\r");
    }


    arinc->W_CommandValue(W_REC_FIFO_STATUS_PIN, 0x40);             // set TRFO pin to RFHALF

    arinc->TransmitCommandAndData16(W_WRDCNT, (MATCH_VALUE << 3));  // Initialize the Word Count Match register
    // The Match Pin will generate an interrupt where the LED4 will be turned. 
    // LED4 will be turned off at the end of each sub frame in this main code below. 
    printf("%s","Match Word Count Register set to = ");
    printf("0x%x",MATCH_VALUE);

    printf("%s","\n\r\nPress S1 to start\n\r\n");

    debounceButtonMake(BUTTON1);
    debounceButtonBreak(BUTTON1);
    printf("%s","Press S1 or Space Bar to Display Status and Control Registers\n\r");
    printf("%s","\n\rTransmitting\n\r");

    for (;;) {
        ch = getchar();

        if (!SW1 || ch == ' ')                         // print out the status registers if
                                                    // SW3 pressed or spacebar from PC                                           
        {
            printf("%s","\n\n\rHI-3717 Status, Control Registers\r\n");
            DisableInterrupts();
            getRegStatus();
            DisplayStatusRegisters();               // print out the status registers
            EnableInterrupts();
            printf("%s","Transmitting Paused\n\r");
            printf("%s","\n\rPress S4 or spacebar to continue\n\r");

            while (SW4 && ' ' != getchar());        // start again if SW-4 pressed/spacebar

            printf("%s","Transmitting Continuing...");

            arinc->softReset3717();                        // Soft Reset the 3717

            MessageCount_R = 0;                       // Reinitialize the counters/pointers
            syncPointer_R = 0;
            MessageCount = 0;
            syncPointerTX = 0;
            g_wordCounter = 0;

        }

        Transmitter2();                            // Transmit data if FIFO not full


   // Check the Receiver FIFO for any data to fetch.     
        Status_F = arinc->R_Register(R_FIFO_STATUS_REG);  // Fetch the Receiver FIFO status reg  

        if (0x00 == (Status_F & RFEMPTY))           // Check Receive FIFO Not Empty to fetch data
        {

            PORTB ^= 0x08;
            LED3 ^= HI;                              // Flash the LED3 to show some data was received

            arinc->FetchFIFOWordsWithCmd(R_REC_FIFO_WORD_COUNT, 2, RXBufferInt);
            FIFOBufferInt[MessageCount & 0xFFF] = RXBufferInt[0];   // Log the data in the FIFO Buffer

            g_ReceivedCount++;

            // Log all the sync words received no matter where they are                  
            for (syncPointer = 0; syncPointer < 4; syncPointer++)
            {
                if (RXBufferInt[0] == sync[syncPointer])
                {
                    syncLog[syncPointer] += 1;
                    LED2 ^= 1;
                }
            }

            //  Display the data with format 123. Sync words will show * like 0247*
            if (g_max > 2048)       // Rev 1_1 changed to 2048 from 4096.
            {
                if (MessageCount == 0)
                    displayOneDataWord(RXBufferInt[0]);       // Display sync words only where expected
            }
            else
            {
                print3717DataWord(MessageCount, RXBufferInt); // display the 3 nibbles of data
               // At lower baudrates you can also display the word count.             
               // Disable displaying word count as default
               // displayOneDataWord(RXBufferInt[1] >> 3);       // Display word count                 
            }


            // Verify the SNYC Word and Data Word against the next predicted values
            if (Data717Compare(RXBufferInt))
            {
                LED_CTL(LED_8, ON);              // turn on RED LED if failed
                printf("%s","\n\rData = ");
                print3717DataWord(MessageCount, RXBufferInt);  // display the 3 nibbles of data
                printf("%s","  Word Count = ");
                displayOneDataWord(RXBufferInt[1] >> 3);       // Display word count                              
                for (;;);
            }

            MessageCount++;
            if (MessageCount == g_max)        // Reset FIFO data pointer if exceeded size
            {
                LED_CTL(LED_4, OFF);         // Turn off the Match LED
                MessageCount = 0;
            }

        }// end of receiver data handling

        rxfstat = arinc->R_Register(R_FIFO_STATUS_REG);     // Read the Receiver FIFO Status Reg
        if (rxfstat & RFFULL)                           // Turn on LED if FIFO Full
            LED5 = ON;
        else
            LED5 = OFF;

        txfstat = arinc->R_Register(R_FIFO_XMT_REG);        // Read the Receiver FIFO Status Reg
        if (rxfstat & TFFULL)                           // Turn on LED if FIFO Full
            LED6 = ON;
        else
            LED6 = OFF;



    };// for   
}  // Transmit Mode End

// ------------------------------------------------------
// *****SOFTWARE SYNC TRANSMIT RECEIVE MODE *****       
// Rev 1.2 addition:
// This sets the SFTSYNC bit-2 in the CTRL1 register to enable Software Sync Mode.
// The SYNC ID status is checked by examing the Sync Word(Barker Code) in the data and sets PT5 and PT4 accordingly.
// 
// Available SW Button commands:
//    S1    Push to start the transmission/receptions initially
//
//    S1    Display the status registers. Press SW-4 to continue..
//    OPT1 DIP SW     BPRZ = open, HBP = closed
//    OPT2 DIP SW     32WPS = open                                                            
//    BITR0 - BITR2 select bit rates 64-8192WPS. OPT2 32WPS will override these if open!    
//    Console Commands:
//       Pressing the space bar will fetch and display the Control and Status 
//       registers on the console. Press space bar again to resume.
// ------------------------------------------------------
void Arinc3717Test::SftSync_Mode_Demo(const uint8_t SELFTEST) {
    static unsigned char Status_F;

    char ch;                   // console character
    uint8_t rxsel, wps32, wordRateSwitches;

    unsigned int   syncWord;    // received sync word
    uint8_t ctrl1;
    uint8_t softSync;           // Bit1:0 reflect new Sync status


    MessageCount = 0;

    printf("%s","\n\rTransmit and Receive Mode\n\r");

    if (SELFTEST)
        printf("%s","\n\rSELF-TEST, Internal Digital Loop-back, No data is transmitted externally\n\r");

    printf("%s","\n\rInitializing the HI-3717 - SOFTWARE SYNC MODE-6\r\n");


    // BPRZ or HBP? 
    if (OPT1 & PORTA)
    {
        rxsel = 1;
        printf("%s","\n\rReceiver Mode: BPRZ");
    }
    else
    {
        rxsel = 0;
        printf("%s","\n\rReceiver Mode: HBP");
    }

    wordRateSwitches = PORTB & 0x07;       // get all bitrate switches 
    printf("%s","\n\rWord Rate = ");

    // 32WPS?       
    if (OPT2 & PORTA)
    {
        wps32 = 1;
        g_max = wordRates[8];
        printf("%s",BitRateStrings[8]);
        printf("%s","\n\rSlew Rate 7.5uS Selected");
        printf("\n");
    }
    else
    {
        wps32 = 0;
        printf("%s",BitRateStrings[wordRateSwitches]);
        g_max = wordRates[wordRateSwitches];

        if (g_max > 2048)
            printf("%s","\n\rConsole ARINC data output disabled due to speed > 2048WPS");
        printf("\n");

        if (g_max > 1024)
            printf("%s","Slew Rate 1.5us Selected");
        else if (g_max == 1024)
            printf("%s","Slew Rate 3.75us Selected");
        else
            printf("%s","Slew Rate 7.5us Selected");

    }
    printf("\n");

    g_wordCounter = 0;
    syncPointer = 0;


    ctrl1 = SELFTEST;
    ctrl1 |= SFTSYNC;    // enable Software Sync mode
    // Pass these param to the init function: TEST mode, wordRate, 32WPS, RXSEL                 
    if (INIT_FAIL == arinc->Init3717(ctrl1, wordRateSwitches, wps32, rxsel))
    {
        LED_CTL(LED_8, ON);              // turn on RED LED if failed
        printf("%s","** Failed to Initialize the HI-3717!!! **\n\r");
        for (;;);
    }
    else
    {
        printf("%s","HI-3717 Initialized Software Sync mode OK\n\r");
    }


    arinc->W_CommandValue(W_REC_FIFO_STATUS_PIN, 0x40);             // set TRFO pin to RFHALF

    arinc->TransmitCommandAndData16(W_WRDCNT, (MATCH_VALUE << 3));  // Initialize the Word Count Match register
    // The Match Pin will generate an interrupt where the LED4 will be turned. 
    // LED4 will be turned off at the end of each sub frame in this main code below. 
    printf("%s","Match Word Count Register set to = ");
    printf("0x%x",MATCH_VALUE);

    printf("%s","\n\n\rSync Barker codes 0x0247 0x05B8 0x0A47 0xDB8\r\n");
    printf("%s","\n\r\nPress S1 to start\n\r\n");

    debounceButtonMake(BUTTON1);
    debounceButtonBreak(BUTTON1);
    printf("%s","Press S1 or Space Bar to Display Status and Control Registers\n\r");
    printf("%s","\n\rTransmitting\n\r");

    // The starting sync word to transmit can be altered by changing this variable (0-3)
    syncPointerTX = 0;
    // Starting Barker code sequence in SFTSYNC mode.     
    // 0x0A47, 0x0DB8, 0x0247, 0x05B8                            

    for (;;) {
        ch = getchar();

        if (!SW1 || ch == ' ')                         // print out the status registers if
                                                    // SW3 pressed or spacebar from PC                                           
        {
            printf("%s","\n\n\rHI-3717 Status, Control Registers\r\n");
            DisableInterrupts();
            getRegStatus();
            DisplayStatusRegisters();               // print out the status registers
            EnableInterrupts();
            printf("%s","Transmitting Paused\n\r");
            printf("%s","\n\rPress S4 or spacebar to continue\n\r");

            while (SW4 && ' ' != getchar());        // start again if SW-4 pressed/spacebar

            printf("%s","Transmitting Continuing...");

            arinc->softReset3717();                        // Soft Reset the 3717

            MessageCount_R = 0;                       // Reinitialize the counters/pointers
            syncPointer_R = 0;
            MessageCount = 0;
            syncPointerTX = 0;
            g_wordCounter = 0;

        }

        Transmitter2();                            // Transmit data if FIFO not full


   // Check the Receiver FIFO for any data to fetch.     
        Status_F = arinc->R_Register(R_FIFO_STATUS_REG);  // Fetch the Receiver FIFO status reg  

        if (0x00 == (Status_F & RFEMPTY))           // Check Receive FIFO Not Empty to fetch data
        {

            PORTB ^= 0x08;
            LED3 ^= HI;                              // Flash the LED3 to show some data was received

            arinc->FetchFIFOWordsWithCmd(R_REC_FIFO_WORD_COUNT, 2, RXBufferInt);
            FIFOBufferInt[MessageCount & 0xFFF] = RXBufferInt[0];   // Log the data in the FIFO Buffer

            g_ReceivedCount++;

            // Log all the sync words received no matter where they are                  
            for (syncPointer = 0; syncPointer < 4; syncPointer++)
            {
                if (RXBufferInt[0] == sync[syncPointer])
                {
                    syncLog[syncPointer] += 1;     // keep a count of all syncs received
                    LED2 ^= 1;
                }
            }

            //  Display the data with format 123. Sync words will show * like 0247*
            if (g_max > 2048)       // Rev 1_1 changed to 2048 from 4096.
            {
                if (MessageCount == 0)
                    displayOneDataWord(RXBufferInt[0]);       // Display sync words only where expected
            }
            else
            {
                print3717DataWord(MessageCount, RXBufferInt); // display the 3 nibbles of data                
            }

            // Determine Sync phase by examining the Barker Codes.
            // Check for SYNC words only at message count zero. If a SYNC is found
            // set PT5 and PT4 (Base board J1 pin 3 and pin 4 output pins to the
            // true phase. User software could also just example the new variable softSync.
            // Alter the starting value of syncPointerTX (0-3) to change the transmitter starting 
            // Barker Code.      
            if (MessageCount == 0)
            {
                syncWord = RXBufferInt[0];          // get sync word (Barker code)

                if (syncWord == SYNC1)
                {
                    PTT_PTT4 = 0;                    // Write to PT4 output pin
                    PTT_PTT5 = 0;                    // Write to PT5 output pin
                    softSync = 0;
                }
                else if (syncWord == SYNC2)
                {
                    PTT_PTT4 = 1;
                    PTT_PTT5 = 0;
                    softSync = 1;
                }
                else if (syncWord == SYNC3)
                {
                    PTT_PTT4 = 0;
                    PTT_PTT5 = 1;
                    softSync = 2;
                }
                else if (syncWord == SYNC4)
                {
                    PTT_PTT4 = 1;
                    PTT_PTT5 = 1;
                    softSync = 3;
                }

            }


            MessageCount++;
            if (MessageCount == g_max)        // Reset FIFO data pointer if exceeded size
            {
                LED_CTL(LED_4, OFF);         // Turn off the Match LED
                MessageCount = 0;
            }

        }// end of receiver data handling

        rxfstat = arinc->R_Register(R_FIFO_STATUS_REG);     // Read the Receiver FIFO Status Reg
        if (rxfstat & RFFULL)                         // Turn on LED if FIFO Full
            LED5 = ON;
        else
            LED5 = OFF;

        txfstat = arinc->R_Register(R_FIFO_XMT_REG);        // Read the Receiver FIFO Status Reg
        if (rxfstat & TFFULL)                         // Turn on LED if FIFO Full
            LED6 = ON;
        else
            LED6 = OFF;



    };// Loop   
} // New softsync mode end


// ------------------------------------------------------
// Transmit an incrementing data pattern up to the MAX word count allowed - non blocking
// ------------------------------------------------------
void Arinc3717Test::Transmitter2(void)
{

    PORTA |= 0x80;
    if (0 == (0x80 & arinc->R_Register(R_FIFO_XMT_REG)))  // Fetch the transmit FIFO status reg
                                                  // Only load FIFO if it's NOT full.
    {
        PORTA &= 0x7f;

        TXBufferInt[0] = g_wordCounter++;  // fetch the next data word and transmit if FIFO not full 
        if (TXBufferInt[0] == 0)   //g_max)
        {
            g_wordCounter = 1;
            TXBufferInt[0] = sync[syncPointerTX++];     // insert the SYNC word (Barker code)                  
            PP5 ^= HI;                                  // TEST
            LED1 ^= 1;

            syncPointerTX &= 0x03;
        }

        if (g_wordCounter == g_max)
            g_wordCounter = 0;

        arinc->TransmitCommandAndDataWords(W_XMTFIFO, TXBufferInt);  // data pattern from sync, 1, 2, 4...0xFFF      
        g_SentCount++;
        PORTA |= 0x80;
    }


    if (0 == (0x80 & arinc->R_Register(R_FIFO_XMT_REG)))  // Fetch the transmit FIFO status reg
                                                  // Only load FIFO if it's NOT full.
    {
        PORTA &= 0x7f;

        TXBufferInt[0] = g_wordCounter++;  // fetch the next data word and transmit if FIFO not full 
        if (TXBufferInt[0] == 0)   //g_max)
        {
            PORTA &= 0x7f;
            g_wordCounter = 1;
            TXBufferInt[0] = sync[syncPointerTX++];     // insert the SYNC word
            PP5 ^= HI;                                  // TEST
            LED1 ^= 1;
            syncPointerTX &= 0x03;
        }

        if (g_wordCounter == g_max)
            g_wordCounter = 0;

        arinc->TransmitCommandAndDataWords(W_XMTFIFO, TXBufferInt);
        g_SentCount++;
        PORTA |= 0x80;
    }


}

// ------------------------------------------------------
// Receive Only Mode. With or without comparing SNYC and Data.
//
//    S1    Display the status registers. Press SW-4 to continue..
//    OPT1 DIP SW     BPRZ = open, HBP = closed
//    OPT2 DIP SW     32WPS = open
//    BITR0 - BITR2 select bit rates 64-8192WPS. OPT2 32WPS will override these if open!    
//    Console Commands:
//       Pressing the space bar will fetch and display the Control and Status 
//       registers on the console. Press space bar again to resume.
// ------------------------------------------------------
void Arinc3717Test::Receive_Mode(const uint8_t compare)
{
    static unsigned char Status_F;

    char ch;                   // console character
    uint8_t rxsel, wps32, wordRateSwitches;

    MessageCount = 0;

    printf("%s","\n\rReceive Only Mode\n\r");
    if (compare)
        printf("%s","Compare is Enabled");
    else
        printf("%s","Compre is Disabled");


    printf("%s","\n\rInitializing the HI-3717\r\n");

    // BPRZ or HBP? 
    if (OPT1 & PORTA)
    {
        rxsel = 1;
        printf("%s","\n\rReceiver Mode: BPRZ");
    }
    else
    {
        rxsel = 0;
        printf("%s","\n\rReceiver Mode: HBP");
    }

    wordRateSwitches = PORTB & 0x07;       // get all bitrate switches 
    printf("%s","\n\rWord Rate = ");

    // 32WPS?       
    if (OPT2 & PORTA)
    {
        wps32 = 1;
        g_max = wordRates[8];
        printf("%s",BitRateStrings[8]);
        printf("%s","\n\rSlew Rate 7.5uS Selected");
        printf("\n");
    }
    else
    {
        wps32 = 0;
        printf("%s",BitRateStrings[wordRateSwitches]);
        g_max = wordRates[wordRateSwitches];

        if (g_max > 2048)
            printf("%s","\n\rConsole ARINC data output disabled due to speed > 2048WPS");
        printf("\n");

        if (g_max > 1024)
            printf("%s","Slew Rate 1.5us Selected");
        else if (g_max == 1024)
            printf("%s","Slew Rate 3.75us Selected");
        else
            printf("%s","Slew Rate 7.5us Selected");

    }
    printf("\n");


    g_wordCounter = 0;
    syncPointer = 0;

    // Pass these param to the init function: TEST mode, wordRate, 32WPS, RXSEL               
    if (INIT_FAIL == arinc->Init3717(0, wordRateSwitches, wps32, rxsel))
    {
        LED_CTL(LED_8, ON);              // turn on RED LED if failed
        printf("%s","** Failed to Initialize the HI-3717!!! **\n\r");
        for (;;);
    }
    else
    {
        printf("%s","HI-3717 Initialized OK\n\r");
    }

    arinc->TransmitCommandAndData16(W_WRDCNT, (MATCH_VALUE << 3));  // Initialize the Word Count Match register
    // The Match Pin will generate an interrupt where the LED4 will be turned. 
    // LED4 will be turned off at the end of each sub frame in this main code below.      

    printf("%s","Match Word Count Register set to = ");
    printf("0x%x",MATCH_VALUE);

    printf("%s","\n\r\nPress S1 or Space Bar to Display Status and Control Registers\n\r");

    printf("%s","\n\r");

    printf("%s","\n\rReady to Receive data...\n\r");

    for (;;) {
        ch = getchar();

        if (!SW1 || ch == ' ')                         // print out the status registers if
                                                    // SW3 pressed or spacebar from PC                                           
        {
            printf("%s","\n\n\rHI-3717 Status, Control Registers\r\n");
            DisableInterrupts();
            getRegStatus();
            DisplayStatusRegisters();                  // print out the status registers
            EnableInterrupts();
            printf("%s","\n\rPress SW4 or spacebar to continue\n\r");
            printf("%s","Receiving Paused\n\r");

            while (SW4 && ' ' != getchar());        // start again if SW-4 pressed/spacebar

            printf("%s","Receiver Continuing...");

            // Soft Reset the 3717 
            arinc->softReset3717();
            MessageCount_R = 0;
            syncPointer_R = 0;
            MessageCount = 0;
        }

        // Receiver Only     

        Status_F = arinc->R_Register(R_FIFO_STATUS_REG);  // Fetch the Receiver FIFO status reg

        if (0x00 == (Status_F & RFEMPTY)) // Check Receive FIFO Not Empty to fetch data
        {

            PORTB ^= 0x08;
            LED3 ^= HI;                              // Flash the LED3 to show some data was received

            arinc->FetchFIFOWordsWithCmd(R_REC_FIFO_WORD_COUNT, 2, RXBufferInt);

            // Log data in the 4096 word buffer. MessageCount will range from 0-8191. 
            // Sync words will be found at MessageCount=0.                
            FIFOBufferInt[MessageCount & 0xFFF] = RXBufferInt[0];   // Log the data in the FIFO Buffer

           // Log all the sync words received even no matter where they are                  
            for (syncPointer = 0; syncPointer < 4; syncPointer++)
            {
                if (RXBufferInt[0] == sync[syncPointer])
                {
                    syncLog[syncPointer] += 1;
                }
            }


            //  Display the data with format 123. Sync words will have * like 0247*
            if (g_max > 2048)      // Rev 1.1
            {
                if (MessageCount == 0)
                    displayOneDataWord(RXBufferInt[0]);       // Display sync words only where expected            
            }
            else
            {
                print3717DataWord(MessageCount, RXBufferInt); // display the 3 nibbles of data                       
               // Disable displaying word count as default
               // displayOneDataWord(RXBufferInt[1] >> 3);       // Display word count                 
            }

            if (compare)
            {

                // Verify the SNYC Word and Data Word
                if (Data717Compare(RXBufferInt))
                {
                    LED_CTL(LED_8, ON);                             // turn on RED LED if failed
                    printf("%s","\n\rData = ");
                    print3717DataWord(MessageCount, RXBufferInt); // display the 3 nibbles of data
                    printf("%s","  Word Count = ");
                    displayOneDataWord(RXBufferInt[1] >> 3);      // Display word count                              
                    for (;;);
                }
            }

            MessageCount++;
            if (MessageCount == g_max)        // Reset FIFO data pointer if exceeded size
            {
                LED_CTL(LED_4, OFF);         // Turn off the Match LED
                MessageCount = 0;
            }


        }// end of receiver data handling

        rxfstat = arinc->R_Register(R_FIFO_STATUS_REG);     // Read the Receiver FIFO Status Reg
        if (rxfstat & RFFULL)                         // Turn on LED if FIFO Full
            LED5 = ON;
        else
            LED5 = OFF;

        txfstat = arinc->R_Register(R_FIFO_XMT_REG);        // Read the Receiver FIFO Status Reg
        if (rxfstat & TFFULL)                         // Turn on LED if FIFO Full
            LED6 = ON;
        else
            LED6 = OFF;


    };
}

// ------------------------------------------------------
// Verify the SYNC + data with the next expected values
// Returns false if no error found, true if error found
// ------------------------------------------------------
unsigned char Arinc3717Test::Data717Compare(uint16_t* data)
{
    static uint8_t errFlag;

    errFlag = false;

    // Check for proper SYNC
    if (MessageCount_R == 0)
    {
        if (data[0] != sync[syncPointer_R++])
        {
            printf("%s","\n\r\n** SYNC value not matched **\n\r");
            printf("%s","Expected SYNC = ");
            displayOneDataWord(sync[syncPointer_R]); //++]);
            printf("%s","Received SYNC = ");
            displayOneDataWord(data[0]);
            printf("\n");
            errFlag = true;

        }
        syncPointer_R &= 0x03;
    }
    else
    {

        // Check for valid predicted data  
        if (data[0] != (MessageCount_R & 0xFFF))
        {
            printf("%s","\n\r\n** Data Failed to Verify **\n\r");
            printf("%s","Expected = ");
            displayOneDataWord(MessageCount_R + 1);
            printf("\n");
            errFlag = true;
        }


        // Check fetched word count also with predicted data which should match
        if ((data[1] >> 3) != ((MessageCount_R + 1) & 0x1FFF))
        {
            printf("%s","\n\r** Word Count Failed to Verify **\n\r");
            printf("%s","Expected = ");
            displayOneDataWord(MessageCount_R + 1);
            printf("\n");
            errFlag = true;
        }

    }


    MessageCount_R++;         // next value compare prediction
    if (MessageCount_R == g_max)  //
        MessageCount_R = 0;

    return errFlag; //false;      // No Error found

}

// ------------------------------------------------------
// Debounce the button for Make (low) or Break (high)
// Waits for the button to be released for 10ms
// ------------------------------------------------------
void debounceButtonMake(uint8_t button)
{
    uint8_t debounceCnt;

    debounceCnt = 10;
    while (debounceCnt) {
        Delay100us(K_1MS);            // delay 1ms
        if (PORTB & button)
            debounceCnt = 10;
        else
            debounceCnt--;
    }
}
// ------------------------------------------------------
// Debounce the button for Break (high)
// Waits for the button to be released for 10ms
// ------------------------------------------------------
void debounceButtonBreak(uint8_t button)
{
    uint8_t debounceCnt;

    debounceCnt = 10;
    while (debounceCnt) {
        Delay100us(K_1MS);            // delay 1ms
        if (!(PORTB & button))
            debounceCnt = 10;
        else
            debounceCnt--;
    }
}

// ------------------------------------------------------
// Debounce the button on the rising edge only.
// Waits for the button to be released for 10ms
// ------------------------------------------------------
void debounceButton(uint8_t button)
{
    uint8_t debounceCnt;

    debounceCnt = 10;
    while (debounceCnt) {
        Delay100us(K_1MS);            // delay 1ms
        if (!(PORTB & button))
            debounceCnt = 10;
        else
            debounceCnt--;
    }
}

// ------------------------------------------------------
// Display the HI-3717 Control and Status registers on the console
// ------------------------------------------------------
void Arinc3717Test::DisplayStatusRegisters(void)
{
    uint8_t i;
    getRegStatus();
    for (i = 0; i < 5; i++) {
        printf("%s 0x%02x\n",DebugArrayStrings[i], DebugArray[i]);  // Output the status registers
    }

    printf("WORD Count Utility Reg \t0x%02x%02x\n",
        DebugArray[6],
        DebugArray[7]);    // Output 

    printf("REC FIFO Word and Count 0x%02x%02x 0x%02x%02x\n",
        DebugArray[10],
        DebugArray[11],
        DebugArray[12],
        DebugArray[13]);   // Output 
}

// ------------------------------------------------------
// Get the HI-3717 Readable Registers for viewing in the debugger and
// ------------------------------------------------------
void Arinc3717Test::getRegStatus(void) {
    uint8_t tempArray[4];

    DebugArray[0] = arinc->R_Register(R_CTRL0);     // Fetch Control register 0
    DebugArray[1] = arinc->R_Register(R_CTRL1);     // Fetch Control register 1

    DebugArray[2] = arinc->R_Register(R_FIFO_STATUS_REG);
    DebugArray[3] = arinc->R_Register(R_FIFO_XMT_REG);

    DebugArray[4] = arinc->R_Register(R_REC_FIFO_STATUS_PIN);
    DebugArray[5] = arinc->R_Register(R_READ_FIFO_FAST);

    arinc->MultiByteRead(R_WORD_COUNT_REG, 2, tempArray);
    DebugArray[6] = tempArray[0];
    DebugArray[7] = tempArray[1];

    arinc->MultiByteRead(R_REC_FIFO_WORD_COUNT, 4, tempArray);
    DebugArray[10] = tempArray[0];
    DebugArray[11] = tempArray[1];
    DebugArray[12] = tempArray[2];
    DebugArray[13] = tempArray[3];

}

// ------------------------------------------------------
// General timer tick 100us for delays
// ------------------------------------------------------
void Delay100us(unsigned int delay) {
    g_count100us = delay;
    while (g_count100us);

}

// ------------------------------------------------------
// General timer tick 100us for delays  - reserved for possible use
// ------------------------------------------------------
char TimerScheduler100us(void) {

    if (g_tick100us == 0)
    {
        g_tick100us = 1000;       // 100ms time   
        return true;
    }
    else
    {
        return false;
    }
}

// ------------------------------------------------------
// Control LED1 - LED8
// ledNumber: LED_1,LED_2,LED_3,LED_4....LED_8 [1-8]
// OnOff: 1=ON, 0=OFF
// ------------------------------------------------------
void LED_CTL(uint8_t ledNumber, uint8_t OnOff) {
#if NEWBOARD
    if (ledNumber > 4 && ledNumber < 8)          // LEDs 5-7 have reversed HW logic so invert these 3
#else
    if (ledNumber > 4)                         // Old board.
#endif

        OnOff = ~OnOff;
    switch (ledNumber) {
    case 1:  LED1 = OnOff;  break;
    case 2:  LED2 = OnOff;  break;
    case 3:  LED3 = OnOff;  break;
    case 4:  LED4 = OnOff;  break;
    case 5:  LED5 = OnOff;  break;
    case 6:  LED6 = OnOff;  break;
    case 7:  LED7 = OnOff;  break;
    case 8:  LED8 = OnOff;  break;
    default: break;
    }
}


// ------------------------------------------------------
// Pulse the MR pin of the HI-3717
// ------------------------------------------------------
void HW_RESET(void) {
    unsigned char i;

//    MR = LO;                   // MR is active low on 3717
    for (i = 0; i < 15; i++);      // ~ 2us at 40MHZ Bus clock
 //   MR = HI;

}

// ------------------------------------------------------
// Read in the MODE0, MODE1, MODE2 switches and save it.
// ------------------------------------------------------
uint8_t MODE_SELECT(void) {
    printf("Select an action:\n");
    printf("0 - Board Test\n");
    printf("1 - Transmit an incrementing counter value\n");
    printf("2 - Transmit an incrementing counter value TEST (LoopBack Mode)\n");
    printf("3 - TRANSMIT special PATTERNS, No Receiving\n");
    printf("4 - RECEIVE MODE only\n");
    printf("5 - RECEIVE MODE only with COMPARE (use with another board with Mode 2)\n");
    printf("6 - SOFTWARE SYNC MODE demo\n");
    printf("7 - USE THIS FOR SPECIAL TESTING\n");
    printf("Please enter a number (0-7)? ");
    char c = getchar();
    return c & 0xf;
}

// ------------------------------------------------------
//Timer_Init
// ------------------------------------------------------
void Timer_Init(void)
{
/*    TIOS = 0x03;   //channels 0 and 1 as output compare
    TCTL1 = 0x00;  //channels disconnected from pin logic    
    TIE = 0x01;    //interrupt enable for channel 0 (one channel is servicing in interrupt 
                   //routine, the second one in main loop through polling)  

    PTPSR = 9;     //precision prescaler for div 10
    TSCR1 = 0xF8;*/
}


// ------------------------------------------------------
//  Ext. interrupt pin support
// ------------------------------------------------------
void IRQ_Init(void)
{
    // taken out 3/30/10 different MCU  error compile
    //  INTCR_IRQE = 1;   // falling edge
    //  INTCR_IRQEN = 1;  // enable IRQ    

    // added 3/30/10 
//    IRQCR_IRQE = 1;   // falling edge         
//    IRQCR_IRQEN = 1;   // enable IRQ          

}

// ------------------------------------------------------
// Print the ARINC 717 word on the console, format [D11-D8] [D7-D4] [D3-D0].
// ------------------------------------------------------
void Arinc3717Test::print3717Data(unsigned int index, unsigned char* array)
{
    unsigned char i;
    static unsigned char charCol = 0;


    i = 0;

#if 0   
    printf("%x ", index);
#endif


    printf("%x", array[i++] & 0x0f);
    printf("%x", array[i]);

    if (index == 0)
    {
        putchar('*');
        charCol = 1;
    }
    else
        putchar(' ');

    if (index == g_max - 1)
        printf("\n");

    if (charCol == 20)
    {
        printf("\n");
        charCol = 0;
    }

    charCol++;


}

// ------------------------------------------------------
// Displays the ARINC 717 12 bit data with 3 consecutive nibbles: 123 etc.
// ------------------------------------------------------ 
void Arinc3717Test::print3717DataWord(unsigned int index, uint16_t* array)
{
    unsigned char i;
    static unsigned char charCol = 0;
    static unsigned int temp;

    i = 0;

#if 0   // to show the word count enable this. Works only at slower bit rates.
    printf("%x ", index);
#endif


    printf("%x", array[i] & 0xfff);

    if (index == 0)
    {
        putchar('*');
        charCol = 1;
    }
    else
        putchar(' ');

    if (index == g_max - 1)
        printf("\n");

    if (charCol == 20)
    {
        printf("\n");
        charCol = 0;
    }

    charCol++;


}

// ------------------------------------------------------
// Generic Display Word eg. 1234
// ------------------------------------------------------
void Arinc3717Test::displayOneDataWord(unsigned int ArincWord)
{
    printf("%x ", ArincWord & 0xfff);
}

//==============================================================================
//TIMER_ISR
// Precaler is 10 so:
// Period is 40Mhz/10*400 = 10Khz. Or use: 10 * 400/40Mhz = 100us
//==============================================================================
void TIMER_ISR(void)
{
    static int count100us = 10;

    // Don't change any of this code below
    TC0 += 400;               // Sets up 100us period

    if (g_count100us)
        g_count100us--;

    if (g_tick100us)
        g_tick100us--;


    count100us--;

    //User Code Section  

    if (!count100us)
    {
        count100us = K_1SEC;              // 1 second scheduler
        if (ON == g_ledFlashBool)            // Blink the LED7 if enabled
            LED7 ^= TOGGLE;                 // Alive 1 second blink 

    }
}



// ------------------------------------------------------
// PortP interrupt Service Routine 
// Ensure you disable interrupts around any foreground code that accesses the SPI
// if you want to access the SPI in this interrupt routine.
// Optional interrupt handlers for user code.
// ------------------------------------------------------
void PORTP_ISR(void)
{

    PP4 = HI;                         // identify which interrupt with scope

    if (PIFP & PP1Interrupt)       //MATCH
    {
        LED_CTL(LED_4, ON);
        PP4 = LO;
        PP4 = HI;
        PP4 = LO;
        PP4 = HI;
        PIFP_PIFP1 = 1;               //clear this interrupt       
    }

#if 0   // These other interrupts are not used in this demo
    if (PIFP & PP0Interrupt)
    {
        PP4 = LO;
        PP4 = HI;

        PIFP_PIFP0 = 1;               //clear this interrupt       
    }


    if (PIFP & PP2Interrupt)                     //ROVF 
    {
        PP4 = LO;
        PP4 = HI;
        PP4 = LO;
        PP4 = HI;
        PP4 = LO;
        PP4 = HI;


        PIFP_PIFP3 = 1;               //clear this interrupt       
    }


    if (PIFP & PP3Interrupt)                     //TFIFO 
    {
        PP4 = LO;
        PP4 = HI;
        PP4 = LO;
        PP4 = HI;
        PP4 = LO;
        PP4 = HI;
        PP4 = LO;
        PP4 = HI;


        PIFP_PIFP4 = 1;                 //clear this interrupt       
    }

#endif

    PP4 = LO;

}


// ------------------------------------------------------
// Initialize interrupt routine
// ------------------------------------------------------
void XIRQ_Init(void)
{
        LED_CTL(LED_7, ON);
}

// ------------------------------------------------------
// Unused interrupt Service Routine
// ------------------------------------------------------
void IRQ_ISR(void)
{
    LED_CTL(LED_7, ON);
}


// ------------------------------------------------------
// XIRQ unused Interrupt Service Routine
// ------------------------------------------------------
void XIRQ_ISR(void)
{
    LED_CTL(LED_7, ON);
}






/* End of File */