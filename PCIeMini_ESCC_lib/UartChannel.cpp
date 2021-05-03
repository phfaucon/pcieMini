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
#include <stdio.h>
#include "SccChannel.h"
#include "AlphiBoard.h"

/** @brief Receive a line terminated by the EOL character
 * 
 * The EOL character is defined in the SccConfig structure. The string is always 0 terminated.
 * @param buffer Buffer receiving the characters
 * @param n Size of the buffer
 * @retval Address of the buffer if the operation succeeded or NULL
 */
char * SccChannel::gets_s (char *buffer, size_t n)
{
    size_t i = 0;
    unsigned char c = 0xff;

    // while we don't have all the character and it is not the end of line loop
    while( (i < (n - 1)) && ((c != scc_config.eolChar) || (scc_config.eolChar ==  0xFF)))
    {
        c = inch();
        buffer[i++] = c;
    }
    buffer[i] = 0;

    return buffer;
}

/** @brief Transmit a 0 terminated string
 * @param str Pointer to the string.
 * *retval Returns the number of characters transmitted
 */
int SccChannel::puts(const char *str)
{
    int i = 0;
    
    while (str[i] != 0)
    {
        outch(str[i++]);
    }
    
    return i;
}

/*! \brief Transmit a data byte.
 *
 * Transmit a character directly to the SCC if the DMA is disabled, or using the FIFO if the DMA is enabled. Returns
 * when the character has been written to its destination, even if the actual transmission has not actually started.
 * @param val The character to transmit.
*/
void SccChannel::outch(uint8_t val)
{
    uint8_t tmp;

    // check if it is a FIFO operation
    if (useFifoTrmt)
    {
        outch_FIFO(val);
        return;
    }

    do
    {
        AlphiBoard::MsSleep(pollingDelay);
        sccRegisterRead(0, &tmp);
    } while (!(tmp & Tx_BUF_EMP));
    sccDataWrite(val);
}

/*! \brief Transmit a data byte, if possible.
 *
 * Transmit a character directly to the SCC if the DMA is disabled, or using the FIFO if the DMA is enabled. Does not wait
 * if the transmitter logic is not ready.
 * @param val The character to transmit.
 * @return This function returns -1 if the character has not been written, and 0 if it has been written successfully
*/
int SccChannel::outchnw(uint8_t val)
{
    uint8_t tmp;

    // check if it is a FIFO operation
    if (useFifoTrmt)
    {
        return outchnw_FIFO(val);
    }

    sccRegisterRead(0, &tmp);

    if (!(tmp & Tx_BUF_EMP))
    {
        return SCC_BUSY;
    }

    sccDataWrite(val);
    return 0;
}

/************************************************************************/
/****** inch: This subroutine inputs a byte from a serial Port. *******/

/*! \brief Receive a data byte.
    *
    * receive a character either directly from the SCC if the DMA is disabled, or from the FIFO if the DMA is enabled. Wait until a character is available.
    * @return This function returns the character.
*/
int SccChannel::inch()
{
    uint8_t tmp;

    // check if it is a FIFO operation
    if (useFifoRcv)
    {
        return inch_FIFO();
    }

    sccRegisterRead(0, &tmp);

    // Frame Error
    if (tmp & BRK_ABRT)
    {
        // Reset error	 
        sccRegisterWrite(0, RES_H_IUS);
    }

    // Wait for RXD data flag
    do
    {
        sccRegisterRead(0, &tmp);
        AlphiBoard::MsSleep(pollingDelay);
    } while (!(tmp & Rx_CH_AV));

    sccDataRead(&tmp);
    return tmp;
}


/************************************************************************/
/*! \brief Receive a data byte.
    *
    * receive a character either directly from the SCC if the DMA is disabled, or from the FIFO if the DMA is enabled. Wait until a character is available.
        * @param pollDelay set the pollDelay to be used
        * @return This function returns the character.
*/
int SccChannel::inch(int pollDelay)
{
    uint8_t tmp;

    // check if it is a FIFO operation
    if (useFifoRcv)
    {
        return inch_FIFO();
    }

    sccRegisterRead(0, &tmp);

    // Frame Error
    if (tmp & BRK_ABRT)
    {
        // Reset error
        sccRegisterWrite(0, RES_H_IUS);
    }

    // Wait for RXD data flag
    do
    {
        sccRegisterRead(0, &tmp);
        AlphiBoard::MsSleep(pollDelay);
    } while (!(tmp & Rx_CH_AV));

    sccDataRead(&tmp);
    return tmp;
}

/*! \brief Receive a data byte, if possible.
 *
 * receive a character either directly from the SCC if the DMA is disabled, or from the FIFO if the DMA is enabled. Does not wait
 * if there is no character available.
 * @return This function returns -1 if no character is available, and the character if the character was received successfully.
*/
int SccChannel::inchnw()
{
    uint8_t tmp;

    // check if it is a FIFO operation
    if (useFifoRcv)
    {
        return inchnw_FIFO();
    }

    sccRegisterRead(0, &tmp);

    // Frame Error
    if (tmp & BRK_ABRT)
    {
        // Reset error
        sccRegisterWrite(0, RES_H_IUS);
    }

    /* Wait for RXD data flag */
    sccRegisterRead(0, &tmp);
    if ((tmp & Rx_CH_AV))
    {
        sccDataRead(&tmp);
        return tmp;
    }
    else return -1;
}

