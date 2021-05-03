/*       Uart.h
 *       Holt HI-3593 Demonstration Program
 *
 *
 *      	This file demonstrates a simple ANSI C program structure for
 *			the initializing and using a Holt HI-3593 device.
 *
 */

 // SCI register pointers to access the SCI registers
#define SCIBRH   (*((volatile unsigned char*)(0x00C8))) 
#define SCIBRL   (*((volatile unsigned char*)(0x00C9))) 
#define SCICR1   (*((volatile unsigned char*)(0x00CA))) 
#define SCICR2   (*((volatile unsigned char*)(0x00CB))) 
#define SCISR1   (*((volatile unsigned char*)(0x00CC))) 
#define SCISR2   (*((volatile unsigned char*)(0x00CD))) 
#define SCIDRH   (*((volatile unsigned char*)(0x00CE))) 
#define SCIDRL   (*((volatile unsigned char*)(0x00CF))) 


// Constants

#define ERROR_OK    1
#define ERROR_ERROR 0
#define START_CYCLE 1
#define WAIT_CYCLE  0
#define TAB 9
#define SPACE 0x20         // space
#define CR 0x0D            // carrage return
#define LF 0x0A            // line feed
#define NEWPAGE 0x0C       // new page (clears the full screen)
#define XON 0x11           // Flow control ON
#define XOFF 0x13          // Flow control OFF
#define  b115200 22        // constant for 115200 KB
#define  b230400 11        // constant for 230400 KB
//#define EOF -1

// Prototypes
char atohex2(char upperNibble, char lowerNibble);
uint8 atohex(char ch);
unsigned char SCIConfig(unsigned char baudRate);
void UartConfig(unsigned char baudRate);
void PrintOneHexByte(uint8 c);
void xputc(char ch);
void xprint(char* string);
void crlf(void);
char xgetchar(void);
void xscan(char* stringBuffer);
void PrintOneWord(unsigned int w);
void PrintOneWord0x(unsigned int w);




