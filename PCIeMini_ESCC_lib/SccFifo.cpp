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
#include "SccFifo.h"

#include "Windows.h"

/** @brief Constructor
 *
 * This is called during the board open process. It should not be called by the user.
 * @param ctrlAddress Pointer to the FIFO structure in user space.
 * @param isReadFifo Read FIFO have a 4 byte buffer that needs to be taken in count when calculating FIFO usage
 */
SccFifo::SccFifo(uint32_t* ctrlAddress, RxFifoData* fifo8to32)
{
    ctrlReg = ctrlAddress;
    readFifo = (fifo8to32 != NULL);
    rxFifoData = fifo8to32;
    inBufValid = 0;
    inBuf = 0;
    bytesReceived = 0;
}

uint32_t SccFifo::getCtrlReg()
{
    return *ctrlReg;
}

void SccFifo::setCtrlReg(uint32_t val)
{
    *ctrlReg = val;
}

/*! \brief Check the FIFO usage.
*
* @return The number of characters in the FIFO.
*/
int SccFifo::usage()
{
    int totalChar;
    uint32_t reg32;

    // if fifo full, return TxFifoSize
    reg32 = getCtrlReg();
    if (reg32 & (1 << 16)) {
        totalChar = fifoSize;
    }
    else {
        // get the number of char in the fifo
        totalChar = reg32 & 0xffff;
    }
    if (readFifo) {
        totalChar += rxFifoData->getCurrValid() + inBufValid;         // we need to add the number of characters in the output buffer
    }
    return totalChar;
}

/*! \brief Resets the FIFO.
 *
 * The pointers are reset and the FIFO is emptied.
*/
void SccFifo::reset()
{
    uint32_t reg32;

    // set the Tx DMA flag in the control register
    reg32 = getCtrlReg() | fifoResetMask;
    setCtrlReg(reg32);
    Sleep(1);
    reg32 &= ~fifoResetMask;
    setCtrlReg(reg32);
    bytesReceived = 0;

    if (readFifo) {
        inBufValid = 0;
        inBuf = 0;
        rxFifoData->getData();
    }
}

/** @brief Check the FIFO space left.
 *
 * @return The number of characters that can still be added to the FIFO.
*/
int	SccFifo::fifoSpace()
{
    return fifoSize - usage();
}

/** @brief Check if the fifo is empty
 * @retval True if the FIFO is empty
 */
bool	SccFifo::isFifoEmpty() { return !!(fifoEmptyMask & getCtrlReg()); }				///< Returns true if the fifo is empty

/*! \brief Check if the fifo is almost empty
 * @retval True if the FIFO is almost empty
 */
bool	SccFifo::isFifoAlmostEmpty() { return !!(fifoAlmostEmptyMask & getCtrlReg()); }	///< Returns true if the fifo is almost empty

/*! \brief Check if the fifo is almost full
 * @retval True if the FIFO is almost full
 */
bool	SccFifo::isFifoAlmostFull() { return !!(fifoAlmostFullMask & getCtrlReg()); }	///< Returns true if the fifo is almost full

/*! \brief Check if the fifo is full
 * @retval True if the FIFO is full
 */
bool	SccFifo::isFifoFull() { return !!(fifoFullMask & getCtrlReg()); }				///< Returns true if the fifo is full

/*! \brief get a byte from the 8 to 32 bit FIFO adapter.
 *
 * If there are no byte left in the cache, try to read from the adapter.
 * @return the byte, or 0xffffffff if no byte is available.
*/
int SccFifo::getByte()
{
    uint8_t retChar;

    // if nothing in the local buffer, grab what might be in the FIFO
    if (inBufValid == 0)
    {
        // else check the hardware
        inBuf = rxFifoData->getData();
        inBufValid = rxFifoData->getRcvdValid();
        inBufValid &= 0xf;
        if (inBufValid == 0)
        {
            return -1;
        }
        bytesReceived += inBufValid;
    }

    retChar = (uint8_t)(inBuf & 0xff);
    inBuf >>= 8;
    inBufValid--;

    return retChar;
}

