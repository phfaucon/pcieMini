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


#include "SccChannel.h"
#include "Windows.h"

/*******************************************************************/
/*  dmaSetup    enables/disables the DMA */

/*! \brief Enable the Receive FIFO.
    *
    *		When the FIFO is enabled, any character available will be copied through DMA to the associated receive FIFO.
    *		The character must then be retrieved by accessing the FIFO instead of the 8530 SCC.
*/
int SccChannel::enableRxDma()
{
    uint8_t reg;

    // SCC: enable receive DMA on /W//REQ
    reg = cachedRegs[1];

    reg |= WT_RDY_RT|WT_FN_RDYFN|WT_RDY_ENAB;    /* Wait/Ready Enable */
    sccRegisterWrite(1, reg);  /* enable receive DMA on /W//REQ */

    // set the Rx DMA flag in the control register
    *controlRegister |= sccControlRxDmaEnableMask;
    useFifoRcv = true;
    
    return 0;
}

/*! \brief Disable the Receive FIFO.
 *
 *		When the FIFO is disabled, The received characters must be retrieved by accessing the 8530 SCC.
*/
int SccChannel::disableRxDma()
{
    // set the Rx DMA flag in the control register
    *controlRegister &= ~sccControlRxDmaEnableMask;
    useFifoRcv = false;
    return 0;
}

/*! \brief Enable the Transmit FIFO.
 *
 *		When the FIFO is enabled, whenever the transmitter is ready, any character available in the transmit FIFO will be copied through DMA to the SCC.
 *		The character must then be transmitted by writing to the FIFO instead of the 8530 SCC.
*/
int SccChannel::enableTxDma()
{
    // set the Tx DMA flag in the control register
    *controlRegister |= sccControlTxDmaEnableMask;
    useFifoTrmt = true;
    return 0;
}

/*! \brief Disable the Transmit FIFO.
 *
 *		When the FIFO is disabled, the character must be transmitted by writing to the 8530 SCC.
*/
int SccChannel::disableTxDma()
{
    // set the Tx DMA flag in the control register
    *controlRegister &= ~sccControlTxDmaEnableMask;
    useFifoTrmt = false;
    return 0;
}

int SccChannel::inchnw_FIFO()
{
    if (rxFifo->usage() == 0)
    {
        return -1;
    }

    return rxFifo->getByte();
}

int SccChannel::inch_FIFO()
{
    int val;

    do
    {
        val = inchnw_FIFO();
        if (val == -1)
        {
           Sleep(pollingDelay);
        }
    }
    while (val == -1);
    return val;
}

int SccChannel::outchnw_FIFO(uint8_t val)
{
    // if none return -1
    if (txFifo->isFifoFull())
    {
        return -1;
    }
    else
    {	// write to the FIFO
        *txFiFoData8bit = val;
        return 0;
    }
}

void SccChannel::outch_FIFO(uint8_t val)
{
    while(1)
    {
        if (!outchnw_FIFO(val))
        {
            return;
        }
        Sleep(pollingDelay);
    }
    return;
}