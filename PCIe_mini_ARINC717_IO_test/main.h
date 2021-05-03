/*       Main.h
 *       Holt HI-3593 Demonstration Program
 *
 *
 *      	This file demonstrates a simple ANSI C program structure for
 *			the initializing and using a Holt HI-3110 device.
 *
 *
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
 */

#include "Arinc3717Test.h"

typedef unsigned char uint8;
typedef signed char int8;


#define g_RXBuffSize 4  
#define OFFF OFF
#define K_1MS  1
#define K_10MS 10
#define K_100MS  100
#define K_1SEC  1000
#define TOGGLE 1
#define MATCH_VALUE 5         // this value limited to 0 - 0x1FFF

#define DEADLOOP 1
#define BOARD_MODE 0
#define TRANSMIT_RECEIVE_MODE    1
#define TRANSMIT_RECEIVE_SELFTEST_MODE 2
#define TRANSMIT_SPECIAL_MODE 3
#define RECEIVE_MODE 4
#define RECEIVE_COMPARE_MODE 5
#define SFTSYNC_MODE 6                  // new in V 1.2


#define SELFTEST_ON     1
#define SELFTEST_OFF    0
#define COMPARE_OFF     0
#define COMPARE_ON      1


#define SERIAL_COMMANDS 7

#define OPT1       1            // SW2 position 1
#define OPT2       2            // SW2 position 2


#define PP0Interrupt 1
#define PP1Interrupt 2
#define PP2Interrupt 4
#define PP3Interrupt 8

// Prototypes
void TIMER_ISR(void);
void PORTP_ISR(void);
void Delay100us(unsigned int delay);
void PeriphInit(void);
void Timer_Init(void);
void PortP_Init(void);
uint8_t MODE_SELECT(void);
void GPIO_InitInterrupts(void);
void HW_RESET(void);
void LED_CTL(uint8 ledNumber, uint8 OnOff);
void SerialCommands(void);
void debounceButton(uint8 button);
void debounceButtonMake(uint8 button);
void debounceButtonBreak(uint8 button);
char TimerScheduler100us(void);         // reserved for possible use
//void DisplayStatusRegisters(void);

//void displayOneDataWord(unsigned int ArincWord);
//void Transmitter2(void);
//void SftSync_Mode_Demo(const uint8);

void BoardTest(void);

