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
/** @file AlteraDma.cpp
* @brief Implementation of the DMA block controller
*/
// Maintenance Log
//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "AlteraDma.h"

/*
 * alt_avalon_dma_launch_bidir() is called to launch a new transaction when
 * both the receive and transmit channels are using incrementing addresses,
 * i.e. both channels are accesing memory rather than devices.
 */

void AlteraDma::launch_bidir(TransferDesc *t)
{
    base[rdAddress_index] = t->src_offset;
    base[wrAddress_index] = t->dest_offset;

    base[length_index] = t->tfr_length;
}

#if 0
/*
 * alt_avalon_dma_launch_txonly() is called to launch a new transaction when
 * only the transmit channels is using incrementing addresses,
 * i.e. the receive channel is accessing a single memory location (which is
 * probably a device register).
 */

void AlteraDma::launch_txonly(TransferDesc& t)
{
    alt_avalon_dma_txslot* tx_slot;

    if (t.dest_offset != t.tx_end)
    {
        t.active = 1;
        tx_slot = &tx_buf[t.dest_offset];

        base[rdAddress_index] = (uint32_t)tx_slot->from;
        base[length_index] = tx_slot->len;
    }
    else
    {
        t.active = 0;
    }
}

/*
 * alt_avalon_dma_launch_rxonly() is called to launch a new transaction when
 * only the receive channel is using incrementing addresses,
 * i.e. the transmit channel is accessing a single memory location (which is
 * probably a device register).
 */

void AlteraDma::launch_rxonly(TransferDesc& t)
{
    alt_avalon_dma_rxslot* rx_slot;

    if (t.src_offset != t.rx_end)
    {
        t.active = 1;
        rx_slot = &rx_buf[t.src_offset];

        base[wrAddress_index] = (uint32_t)rx_slot->data;
        base[length_index] = rx_slot->len;
    }
    else
    {
        t.active = 0;
    }
}

/*
 * alt_avalon_dma_ioctl is used to process ioctl request for a given DMA
 * device instance. See alt_dma_dev.h for the meaning of the supported
 * ioctl requests.
 */

int AlteraDma::ioctl(int req, void* arg)
{
    int     status = 0;
    uint32_t mode;

    /*
     * Ioctl calls should not be made while the device is active (i.e. while there
     * are DMA transfers outstanding), since this can lead to unpredictable
     * behaviour.
     *
     * The test below attempts to trap this error. It is not foolproof,
     * since it cannot catch concurrent calls to alt_alavalon_dma_prepare()
     * or alt_avalon_dma_send(), but it should at least catch the most
     * common class of problems.
     */

    if ((t.dest_offset != t.tx_end) ||
        (t.src_offset != t.rx_end) ||
        base[length_index])
    {
        return -EIO;
    }

    /* Now process the ioctl. */

    switch (req)
    {
    case ALT_DMA_TX_STREAM_ON:
        if (!(flags & ALT_AVALON_DMA_RX_STREAM))
        {
            base[rdAddress_index] = (uint32_t)arg;
            flags |= ALT_AVALON_DMA_TX_STREAM;
            launch = launch_rxonly;
        }
        else
        {
            status = -EIO;
        }
        break;
    case ALT_DMA_TX_STREAM_OFF:
        flags &= ~ALT_AVALON_DMA_TX_STREAM;
        launch = launch_bidir;
        break;
    case ALT_DMA_RX_STREAM_ON:
        if (!(flags & ALT_AVALON_DMA_TX_STREAM))
        {
            base[wrAddress_index] = (uint32_t)arg;
            flags |= ALT_AVALON_DMA_RX_STREAM;
            launch = launch_txonly;
        }
        else
        {
            status = -EIO;
        }
        break;
    case ALT_DMA_RX_STREAM_OFF:
        flags &= ~ALT_AVALON_DMA_RX_STREAM;
        launch = launch_bidir;
        break;
    case ALT_DMA_SET_MODE_8:
        flags = (flags & ~ALT_AVALON_DMA_MODE_MSK) |
            ALT_AVALON_DMA_MODE_8;
        break;
    case ALT_DMA_SET_MODE_16:
        flags = (flags & ~ALT_AVALON_DMA_MODE_MSK) |
            ALT_AVALON_DMA_MODE_16;
        break;
    case ALT_DMA_SET_MODE_32:
        flags = (flags & ~ALT_AVALON_DMA_MODE_MSK) |
            ALT_AVALON_DMA_MODE_32;
        break;
    case ALT_DMA_SET_MODE_64:
        flags = (flags & ~ALT_AVALON_DMA_MODE_MSK) |
            ALT_AVALON_DMA_MODE_64;
        break;
    case ALT_DMA_SET_MODE_128:
        flags = (flags & ~ALT_AVALON_DMA_MODE_MSK) |
            ALT_AVALON_DMA_MODE_128;
        break;
    default:
        status = -ENOTTY;
    }

    if (!status)
    {
        switch (flags & ALT_AVALON_DMA_MODE_MSK)
        {
        case ALT_AVALON_DMA_MODE_8:
            mode = ALTERA_AVALON_DMA_CONTROL_BYTE_MSK;
            break;
        case ALT_AVALON_DMA_MODE_16:
            mode = ALTERA_AVALON_DMA_CONTROL_HW_MSK;
            break;
        case ALT_AVALON_DMA_MODE_32:
            mode = ALTERA_AVALON_DMA_CONTROL_WORD_MSK;
            break;
        case ALT_AVALON_DMA_MODE_64:
            mode = ALTERA_AVALON_DMA_CONTROL_DWORD_MSK;
            break;
        default:
            mode = ALTERA_AVALON_DMA_CONTROL_QWORD_MSK;
        }

        if (flags & ALT_AVALON_DMA_TX_STREAM)
        {
            base[control_index] = 
                mode |
                ALTERA_AVALON_DMA_CONTROL_GO_MSK |
                ALTERA_AVALON_DMA_CONTROL_I_EN_MSK |
                ALTERA_AVALON_DMA_CONTROL_REEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_WEEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_LEEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_RCON_MSK;
        }
        else if (flags & ALT_AVALON_DMA_RX_STREAM)
        {
            base[control_index] = 
                mode |
                ALTERA_AVALON_DMA_CONTROL_GO_MSK |
                ALTERA_AVALON_DMA_CONTROL_I_EN_MSK |
                ALTERA_AVALON_DMA_CONTROL_REEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_WEEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_LEEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_WCON_MSK;
        }
        else
        {
            base[control_index] = 
                mode |
                ALTERA_AVALON_DMA_CONTROL_GO_MSK |
                ALTERA_AVALON_DMA_CONTROL_I_EN_MSK |
                ALTERA_AVALON_DMA_CONTROL_REEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_WEEN_MSK |
                ALTERA_AVALON_DMA_CONTROL_LEEN_MSK;
        }
    }

    return status;
}

/*
 * Perform an ioctl request for a transmit channel.
 */

int AlteraDma::tx_ioctl(int req,
    void* arg)
{
    return ioctl(req,arg);
}

/*
 * Perform an ioctl request for a receive channel.
 */

int AlteraDma::rx_ioctl(int req,
    void* arg)
{
    return ioctl(req,
        arg);
}

/*
 * Register a DMA receive requests.
 */

int AlteraDma::prepare(void* data,
    uint32_t          len,
    alt_rxchan_done* done,
    void* handle)
{
    alt_avalon_dma_rxslot* slot;
    uint32_t                start;
    uint32_t                end;
    uint32_t                next;
    uint32_t                align_mask;
    ;

    /*
     * Ensure that the data is correctly aligned, and that it's not too
     * big for the device.
     */

    align_mask = flags & ALT_AVALON_DMA_MODE_MSK;

    if ((((uint32_t)data) & align_mask) || (len & align_mask) ||
        (len > tfr_length))
    {
        return -EINVAL;
    }

    start = src_offset;
    end = rx_end;
    slot = &rx_buf[end];

    next = (end + 1) & ALT_AVALON_DMA_NSLOTS_MSK;

    if (next == start)
    {
        return -ENOSPC;
    }

    slot->data = data;
    slot->len = len;
    slot->done = done;
    slot->handle = handle;

    rx_end = next;

    if (!active)
    {
        launch(priv);
    }

    return 0;
}

/*
 * alt_avalon_dma_space() returns the number of tranmit requests that can be
 * posted to the specified DMA transmit channel.
 *
 * A negative value indicates that the value could not be determined.
 */

int AlteraDma::space()
{
    ;

    uint32_t start;
    uint32_t end;

    start = dest_offset;
    end = tx_end;

    return (start > end) ? start - end - 1 :
        ALT_AVALON_DMA_NSLOTS + start - end - 1;
}

/*
 * alt_avalon_dma_send() posts a transmit request to a DMA transmit channel.
 * The input arguments are:
 *
 * dma: the channel to use.
 * from: a pointer to the start of the data to send.
 * length: the length of the data to send in bytes.
 * done: callback function that will be called once the data has been sent.
 * handle: opaque value passed to "done".
 *
 * The return value will be negative if the request cannot be posted, and
 * zero otherwise.
 */

int AlteraDma::send(const void* from,
    uint32_t          len,
    alt_txchan_done* done,
    void* handle)
{
    alt_avalon_dma_txslot* slot;
    uint32_t                start;
    uint32_t                end;
    uint32_t                align_mask;
    uint32_t                next;
    ;

    /*
     * Ensure that the data is correctly aligned, and not too large
     * for the device
     */

    align_mask = flags & ALT_AVALON_DMA_MODE_MSK;

    if ((((uint32_t)from) & align_mask) ||
        (len & align_mask) ||
        (len > tfr_length))
    {
        return -EINVAL;
    }

    start = dest_offset;
    end = tx_end;
    slot = &tx_buf[end];
    next = (end + 1) & ALT_AVALON_DMA_NSLOTS_MSK;

    if (next == start)
    {
        return -ENOSPC;
    }

    /* Fill in the descriptor */

    slot->from = from;
    slot->len = len;
    slot->done = done;
    slot->handle = handle;

    tx_end = next;

    if (!active)
    {
        launch(priv);
    }

    return 0;
}

/*
 * Process a received interrupt. Interrupts are generated when a transaction
 * completes.
 */
void AlteraDma::irq(void* context, uint32_t id)
{
    alt_avalon_dma_txslot* tx_slot;
    alt_avalon_dma_rxslot* rx_slot;
    uint32_t                dest_offset;
    uint32_t                src_offset;
    int                    stream_tx;
    int                    stream_rx;

    stream_tx = flags & ALT_AVALON_DMA_TX_STREAM;
    stream_rx = flags & ALT_AVALON_DMA_RX_STREAM;

    dest_offset = dest_offset;
    src_offset = src_offset;

    tx_slot = &tx_buf[dest_offset];
    rx_slot = &rx_buf[src_offset];

    /* Increment the descriptors */

    if (!stream_tx)
    {
        dest_offset = (dest_offset + 1) & ALT_AVALON_DMA_NSLOTS_MSK;
    }
    if (!stream_rx)
    {
        src_offset = (src_offset + 1) & ALT_AVALON_DMA_NSLOTS_MSK;
    }

    /* clear the interrupt */

    base[status_index] = 0;

    /* Dummy read to ensure IRQ is cleared prior to ISR completion*/
    base[status_index];

    /* launch the next transaction */

    launch(priv);

    /*
     * Make the callbacks
     *
     * Other interrupts are explicitly disabled if callbacks
     * are registered because there is no guarantee that they are
     * preemption-safe. This allows the DMA driver to support
     * interrupt preemption.
     */
    if (!stream_tx && tx_slot->done)
    {
        tx_slot->done(tx_slot->handle);
    }
    if (!stream_rx && rx_slot->done)
    {
        rx_slot->done(rx_slot->handle, rx_slot->data);
    }
}
#endif
/** @brief Constructor
* 
 * Initialise and register the transmit and receive channels for a given
 * physical DMA device.
 * @param dmaAddress Address of the DMA controller in user space.
 */

AlteraDma::AlteraDma(volatile void* dmaAddress)
{

    base = (volatile uint32_t *)dmaAddress;
    /* Halt any current transactions (reset the device) */
    base[control_index] =  ALTERA_AVALON_DMA_CONTROL_SOFTWARERESET_MSK;
    base[control_index] =  ALTERA_AVALON_DMA_CONTROL_SOFTWARERESET_MSK;

    /* Set the default mode of the device (32 bit block reads and writes from/to memory). */
    base[control_index] = 
        ALTERA_AVALON_DMA_CONTROL_WORD_MSK |
        ALTERA_AVALON_DMA_CONTROL_GO_MSK |
        ALTERA_AVALON_DMA_CONTROL_I_EN_MSK |
        ALTERA_AVALON_DMA_CONTROL_REEN_MSK |
        ALTERA_AVALON_DMA_CONTROL_WEEN_MSK |
        ALTERA_AVALON_DMA_CONTROL_LEEN_MSK;

    /* Clear any pending interrupts and the DONE flag */
    base[status_index] = 0;

    /*
     * Register the interrupt handler, and make the device available to the
     * system.
     */
    // TODO:
}
