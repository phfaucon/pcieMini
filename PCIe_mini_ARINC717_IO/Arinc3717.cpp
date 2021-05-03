/*       3717Driver.c
 *       Holt HI-3717 Demonstration Program 
 *
 *
 *      	This file demonstrates a simple ANSI C program structure for 
 *			the initializing and using a Holt HI-3717 device.
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

#include "Arinc3717.h"

// ------------------------------------------------------------------
//  SPI function to load 8 bits of label data
// ------------------------------------------------------------------   
uint8_t Arinc3717::txrx8bits_8 (unsigned char txbyte, unsigned char return_when_done) {
    spi->setTxData(txbyte);
    return spi->getRxData();
}

// ------------------------------------------------------------------
/*
 Initialize the HI-3717

 RETURNS: 0 if successful, 0x0F if failed
 
 INPUT PARMS
  test: 1=test mode. 0=normal mode
  wordRate: 0-7 = words/sec per 3717 data sheet
  wps32: 1=32 WPS (overrides wordRate selections)
  rxsel: 1=BPRZ, 0=HBP
  
*/
// ------------------------------------------------------------------
uint8_t Arinc3717::Init3717(uint8_t test,  uint8_t wordRate, uint8_t wps32, uint8_t rxsel )
   {
   unsigned char cmd=0;
       
// Write control-1 register   
   W_CommandValue(W_CTRL1, test);           // Initialize the test mode if parm set
       
// Write control-0 register last    
   cmd =  rxsel;
   
   if(wps32)
      {
         cmd |= WPS32;
      } else 
      {      
         cmd |= slewRate[wordRate] << 1;       // Select slew rate based on bitrate  
         cmd |= wordRate << 4;                 // Select the actual word rate
           
      }
      
  // Initialize the Control 0 register
  // if parms are passed: WPS32, Slew Rate and word rate
   W_CommandValue(W_CTRL0, cmd);           
   
  // Perform a check to see if writing the word to the 3717 is correct? 
   if(R_Register (R_CTRL0) != cmd)      // Did 3717 receive write ok?
      return INIT_FAIL;                 // fail
   else
      return 0;                         // ok
       
   }


void Arinc3717::softReset3717(void)
{
   uint8_t temp;
   
   temp = R_Register (R_CTRL1);      // Read CTRL1  
   temp |= SRST;         
   W_CommandValue(W_CTRL1, temp); 
   W_CommandValue(W_CTRL1, temp & ~SRST); 
}

/* ------------------------------------------------------------------
/  SPI0 8-Bit Send Data / Receive Data Function
/  ------------------------------------------------------------------
Argument(s):  txbyte, return_when_done

     Return:  rxbyte
     
     Action:  Using SPI0, this function sends txbyte and returns rxbyte
              as part of a chained multi-byte transfer. The calling 
              program controls the /SS chip select instead of using the
              automatic /SS option available in the Freescale SPI port.
              This permits simple chaining of op code commands and Tx/Rx
              data as a series of 8-bit read/writes followed by /SS 
              deassertion by the calling program.
              
              If return_when_done is True, the function waits for transfer
              completion before returning, which may be needed for back-to-
              back commands. If return_when_done is False, the function
              returns immediately after initiating the transfer. 
              
Example Call: rcv_byte = txrx8bits(0xFF,1) // sends data 0xFF then returns 
                                           // data when xfer is done  */
unsigned char Arinc3717::txrx8bits (unsigned char txbyte, unsigned char return_when_done) {
  unsigned char rxbyte;  
  
  spi->setTxData(0);
  return spi->getRxData();  // get received data byte from Data Register 
 }


// ------------------------------------------------------------------
// Single command byte write
// ------------------------------------------------------------------
void Arinc3717::W_Command (uint8_t cmd) {

  uint32_t msg;
  uint8_t words = 1;

  //keep the CS low during the transaction
  spi->setControl(spi->control_SSO_mask | spi->control_resetFifo_mask);

  spi->setTxData(cmd);
  spi->getRxData();

  spi->setControl(0);  // reset SSO
}

// ------------------------------------------------------------------
// Write SPI Command (Opcode) with a Value
//  ------------------------------------------------------------------
void Arinc3717::W_CommandValue (uint8_t cmd, uint8_t value){
  //keep the CS low during the transaction
  spi->setControl(spi->control_SSO_mask | spi->control_resetFifo_mask);

  spi->setTxData(cmd);
  spi->setTxData(value);
  spi->getRxData();
  spi->getRxData();

  spi->setControl(0);  // reset SSO
}


/* ------------------------------------------------------------------
/  Read Register Read Function
/  ------------------------------------------------------------------
Argument(s):  Register to read

     Return:  8-bit Register Value 
*/
uint8_t Arinc3717::R_Register (uint8_t Reg) {
    //keep the CS low during the transaction
    uint8_t value;

    spi->setControl(spi->control_SSO_mask | spi->control_resetFifo_mask);

    spi->setTxData(Reg);
    spi->setTxData(0);
    spi->getRxData();
    value = spi->getRxData();

    spi->setControl(0);  // reset SSO
    return value;
}

// ------------------------------------------------------
// Set the SPI nCS high then low
// ------------------------------------------------------
void Arinc3717::CS_HL (void) {
    spi->resetControlBits(spi->control_SSO_mask);  // reset SSO
    spi->setControlBits(spi->control_SSO_mask);
} 

// ------------------------------------------------------------------
// Transmits the Message Command and data contained in the passed array pointer
// Transmit cmd + 12 bits 
// Valid data range is 00-0X0FFF
// ------------------------------------------------------------------
void Arinc3717::TransmitCommandAndDataWords(uint8_t cmd, uint16_t*TXBuffer)
{              
    uint8_t static dummy;
 
    spi->setControl(spi->control_SSO_mask | spi->control_resetFifo_mask);

    dummy = txrx8bits(cmd, 1);      // Transmit the whole message, ignore return values 

    // Transmit upper 8 bits   
    dummy = txrx8bits((unsigned char)((TXBuffer[0]>>8) & 0x0F),1);   
    // Transmit lower 8 bits
    dummy = txrx8bits((unsigned char)(TXBuffer[0] & 0xFF),1);

    spi->setControl(0);  // reset SSO
}

// ------------------------------------------------------------------
// Transmits the Message Command and data contained in the passed array pointer
// Transmit cmd + 16 bits for 2 byte transmit commands
// Valid data range is 00-0XFFFF
// ------------------------------------------------------------------
void Arinc3717::TransmitCommandAndData16(uint8_t cmd, uint16_t TXBuffer)
{              
    uint8_t static dummy;
 
    spi->setControl(spi->control_SSO_mask | spi->control_resetFifo_mask);

    dummy = txrx8bits(cmd, 1);      // Transmit the whole message, ignore return values 

    // Transmit upper 8 bits   
    dummy = txrx8bits((unsigned char)((TXBuffer>>8) & 0xFF),1);   
    // Transmit lower 8 bits
    dummy = txrx8bits((unsigned char)(TXBuffer & 0xFF),1);

    spi->setControl(0);  // reset SSO
}

// This version returns an Integer array instead of a byte array
void Arinc3717::FetchFIFOWordInt(uint16_t *passedArray)
{
  static uint8_t WordCount, NumberToRead;
  
  unsigned int dummy16;
  
   NumberToRead=1;         // 1 word for now
    
   CS_HL();  

   spi->setTxData(R_REC_FIFO_WORD);     //0xF6;          // SPI RVC FIFO Read Message buffer 
   spi->getRxData();         // read/ignore Rx data in Data Reg, resets SPIF                    
   for (WordCount=0; WordCount<NumberToRead; WordCount++) {
      dummy16 = (unsigned char)txrx8bits(0x00,1);  // fetch d16-d8 uppper word
      dummy16 = dummy16 << 8;                      // position to upper bits
      dummy16 |= (unsigned char)txrx8bits(0x00,1); // fetch d7-0 lower word and combine into 1 16 bit word         
      passedArray[WordCount] = dummy16; 
     }

   spi->setControl(0);  // reset SSO

}
// ------------------------------------------------------
// This function reads the number of 16 bit words into the array by pointer
// ------------------------------------------------------
void Arinc3717::FetchFIFOWordsWithCmd(uint8_t cmd,uint8_t count, uint16_t*passedArray)
{
  static uint8_t WordCount;
  
  unsigned int dummy16;
  
   
   CS_HL();  

   spi->setTxData(cmd);         //0xF6;     // Send the 3717 SPI command
    
   dummy16 = spi->getRxData();         // read/ignore Rx data in Data Reg, resets SPIF                    
   for (WordCount=0; WordCount<count; WordCount++) {
      dummy16 = (unsigned char)txrx8bits(0x00,1);  // fetch d16-d8 uppper word
      dummy16 = dummy16 << 8;                      // position to upper bits
      dummy16 |= (unsigned char)txrx8bits(0x00,1); // fetch d7-0 lower word and combine into 1 16 bit word         
      passedArray[WordCount] = dummy16;            // load word into array
     }

   spi->setControl(0);  // reset SSO

}
// ------------------------------------------------------
// Fetch the Two FIFO Data WORDs and load the data into the callers array
// ------------------------------------------------------
void Arinc3717::FetchFIFOWord(unsigned char *passedArray)
{
  static uint8_t dummy, ByteCount;
   
   CS_HL();  

   spi->setTxData(0xF6);          // SPI RVC FIFO Read Message buffer 
   dummy = spi->getRxData();         // read/ignore Rx data in Data Reg, resets SPIF                    
   for (ByteCount=0; ByteCount<2; ByteCount++) {
      dummy = txrx8bits(0x00,1); 
      passedArray[ByteCount] = dummy; 
     }

   spi->setControl(0);  // reset SSO

}   




// ------------------------------------------------------
// Fetch the SPI FIFO data and load the data into the passed array pointer
// This is a more generic version of this function which could be used to read 
// other SPI multibyte reads.
// Inputs:  ReadCommand=SPI Command
//          count=number of bytes to read
//
// Output:  *passedArray = pointer to the array that gets loaded
// ------------------------------------------------------
void Arinc3717::MultiByteRead(uint8_t ReadCommand, uint8_t count, uint8_t *passedArray)
{
   uint8_t dummy, ByteCount;
   
   CS_HL();  

   spi->setTxData(ReadCommand);            // SPI read command
   dummy = spi->getRxData();                  // read/ignore Rx data in Data Reg, resets SPIF                    
   for (ByteCount=0; ByteCount < count; ByteCount++) {
      dummy = txrx8bits(0x00,1); 
      passedArray[ByteCount] = dummy; 
     }

   spi->setControl(0);  // reset SSO

}




// End

