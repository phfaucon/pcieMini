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

#define QT_CORE_LIB
#include "SccChannel.h"
#include "AlphiBoard.h"
#include <iostream>

/** @brief Configuration structure constructor
    *
    * Initialize the structure with default values: 9600bps, 8-bit, '\n' as end of line, parity disabled, 1 stop bit, FIFOs enabled,
    * RS422.
*/
UartChannelConfig::UartChannelConfig()
{
    baudRate = 9600;
    dataBits = 8;
    eolChar = '\n';
    parity = 1;
    parityEnable = false;
    stopBits = 1;
    bufferConfig = LTC2872::RS_422_buffers;
    useRxFifo = true;
    useTxFifo = true;
    localLoopbackMode = false;
}

/*! \brief Constructor.
    *
    * Does a minimum of initialization. Does not reset the serial channel.
    * @param addr Base address of the channel interface in user space.
*/
SccChannel::SccChannel(volatile void *addr)
{
    baseAddress = addr;
    pollingDelay = 1;			// 10 milliseconds default = 20 characters @ 19200 baud
    memset(cachedRegs, 0, sizeof(cachedRegs));
    useFifoRcv = true;
    useFifoTrmt = true;
    buffer = new LTC2872((char *)addr + bufferCtrlOffset);
    rxFifoData = (RxFifoData*)((char*)addr + rxFifoDataOffset);
    rxFifo = new SccFifo((uint32_t*)((char*)addr + rxFifoCtrlOffset), rxFifoData);
    txFifo = new SccFifo((uint32_t*)((char*)addr + txFifoCtrlOffset));
    txFiFoData8bit = (volatile uint8_t*)((char*)addr + txFifoDataOffset);
    sccCtrlReg = (volatile uint8_t*)((char*)addr + sccCtrlRegOffset);
    sccDataReg = (volatile uint8_t*)((char*)addr + sccDataRegOffset);
    controlRegister = (volatile uint32_t*)((char*)addr + chanCtrlRegOffset);

    config(&scc_config);
}

/** @brief Reset FIFOs and disable DMAs */
void	SccChannel::resetChannel()
{
    disableRxDma();
    disableTxDma();
    rxFifo->reset();
    txFifo->reset();
}

/*! \brief Reset a channel pair.
 *
 * Reset a channel. Please note that because of hardware limitations, the two channels of the on board ESCC are reset at the same time
*/
PCIeMini_status	SccChannel::reset()
{
    uint8_t val;

    /* Reset the chip and FIFOs */

    *controlRegister |= sccControlMainResetMask;
    AlphiBoard::MsSleep(1);
    *controlRegister &= ~sccControlMainResetMask;

    sccRegisterWrite(R9, 0xc0);
    AlphiBoard::MsSleep(1);
    sccRegisterRead(0, &val);

    for (int i = 0; i < 31; i++)
        cachedRegs[i] = 0;

    /* Reset the FIFOs */
    resetChannel();
    sccRegisterWrite(R15, 0x00);

    return ERRCODE_NO_ERROR;
}

/*! \brief specify the serial port buffer configuration, RS232, RS422, or RS485.
 *
 * @param val The value to put in the buffer configuration register, as defined in the LTC2872 class.
*/
void SccChannel::set_serialBuffers(uint32_t val)
{
   buffer->setBuffer(val);
}

/*! \brief Load list or register number and value pairs in the 8530 after a reset.
 *
 * @param rtable Zero-terminated list of character containing register number and value to be loaded. Please
 *		note that the alternate registers are indicated by adding 16 to the register number they shadow: for instance
 *		7+16 refers to the register 7'. Please refer to the UART manual for more description.
*/
int SccChannel::channelLoad(uint8_t *rtable)
{
    int reg;

    while(*rtable!=255)
    {
        reg = *rtable++;
        if(reg>0x0F)
        {
                sccRegisterWrite(15, cachedRegs[15]|1);
        }
        sccRegisterWrite(reg, *rtable);
        if(reg>0x0F)
        {
                sccRegisterWrite(15, cachedRegs[15]&~1);
        }
        cachedRegs[reg]=*rtable++;
    }

    return ERRCODE_NO_ERROR;
}

errno_t SccChannel::config(UartChannelConfig* config)
{
    UINT8 Reg3;
    UINT8 Reg4 = X16CLK;
    UINT8 Reg5 = Tx8;
    UINT8 divider;
    UINT8 Reg14;

    if (config == NULL)
    {
        scc_config.baudRate = 115200;
        scc_config.dataBits = 8;
        scc_config.eolChar = '\n';
        scc_config.parity = 1;
        scc_config.parityEnable = 0;
        scc_config.stopBits = 1;
        scc_config.bufferConfig = LTC2872::RS_422_buffers;
        scc_config.useRxFifo = true;
        scc_config.useTxFifo = true;
    }
    else
    {
        scc_config.baudRate = config->baudRate;
        scc_config.dataBits = config->dataBits;
        scc_config.eolChar = config->eolChar;
        scc_config.parity = config->parity;
        scc_config.parityEnable = config->parityEnable;
        scc_config.stopBits = config->stopBits;
        scc_config.bufferConfig = config->bufferConfig;
        scc_config.useRxFifo = config->useRxFifo;
        scc_config.useTxFifo = config->useTxFifo;
    }

    // receive data bit configuration
    switch (scc_config.dataBits)
    {
    case 5: Reg3 = Rx5;
        break;
    case 6: Reg3 = Rx6;
        break;
    case 7: Reg3 = Rx7;
        break;
    default: Reg3 = Rx8;
        break;
    }

    // transmit data bit configuration
    switch (scc_config.dataBits)
    {
    case 5: Reg5 = Tx5;
        break;
    case 6: Reg5 = Tx6;
        break;
    case 7: Reg5 = Tx7;
        break;
    default: Reg5 = Tx8;
        break;
    }

    if (scc_config.parityEnable)
    {
        Reg4 |= PAR_ENA;
    }

    if (!scc_config.parity)
    {
        Reg4 |= PAR_EVEN;
    }

    if (scc_config.stopBits == 2)
    {
        Reg4 |= SB2;
    }
    else
    {
        Reg4 |= SB1;
    }

    if (scc_config.baudRate <= 38400)
    {
        divider = brgDivLow(SCC_clocksource, 16, scc_config.baudRate);
        Reg14 = DTRREQ | BRSRC | BRENABL;		// BRSRC indicates that the source of the baud rate generator is PCLK
    }
    else
    {
        divider = brgDivLow(RTxcFrequency, 16, scc_config.baudRate);
        Reg14 = DTRREQ | BRENABL;				// BRSRC not asserted indicates that the source of the baud rate generator is the RTxC input
    }
    if (scc_config.localLoopbackMode) {
        Reg14 |= LOOPBAK;
    }
    UINT8 z85233_initList[] =
    {
            4,		Reg4,				/* ASYNC, NO PARITY, 1 STOP BIT */
            6,		0,					/* no sync char */
            7,		0,					/* no sync char */
            2,		0,					/* vector */
            1,		0,
            3,		Reg3,				/* 8 BIT, Rx disabled */
            5,		(uint8_t)(Reg5 | DTR),			/* 8 BIT, Tx disabled */
            9,		0,					/* interrupts disabled */
            10,		NRZ,				/* Encoding */
            11,		RCBR | TCBR,			/* Clocks */
            13,		0,
            12,		divider,
            14,		Reg14,				/* baud rate enabled, DMA signals enabled */
            15,		0,
            7 + 16,	0,					/* automatic RTS de-assert */
            9,		0,
            3,		(uint8_t)(Reg3 | RxENABLE),
            5,		(uint8_t)(Reg5 | TxENABLE),
            255
    };

    reset();
    // set UART speed
    channelLoad(z85233_initList);
    channelLoad(z85233_initList);

    // set other configurations
    set_serialBuffers(scc_config.bufferConfig);
    if (scc_config.useRxFifo)
    {
        enableRxDma();
    }
    else
    {
        disableRxDma();
    }

    if (scc_config.useTxFifo)
    {
        enableTxDma();
    }
    else
    {
        disableTxDma();
    }

    return ERRCODE_NO_ERROR;
}

/*! \brief Asserts RTS.
 *
*/
int SccChannel::enableRTS()
{
    uint8_t reg;
    reg = cachedRegs[5] | RTS;
    sccRegisterWrite(5, reg);
    return ERRCODE_NO_ERROR;
}

/*! \brief Negate RTS.
 *
*/
int SccChannel::disableRTS()
{
    uint8_t reg;
    reg = cachedRegs[5] | RTS;
    reg &= ~RTS;
    sccRegisterWrite(5, reg);
    return ERRCODE_NO_ERROR;
}

/*! \brief Utility baud rate calculator.
 *
 * Calculate a value based on the the requested baud rate and on the 8530 main divider.
 * the function uses the SCC clock frequency to do the calculation.
 * @return The value to put in the divider low byte.
*/
uint8_t SccChannel::brgDivLow(int SCC_clocksource, int div, int baudRate)
{
    return (uint8_t)((SCC_clocksource / (2 * div * baudRate) - 2) & 0xff);
}

uint32_t SccChannel::setSccControlRegister(uint32_t mask)
{
    *controlRegister |= mask;
    return *controlRegister;
}

uint32_t SccChannel::resetSccControlRegister(uint32_t mask)
{
    *controlRegister &= ~mask;
    return *controlRegister;
}

uint32_t SccChannel::getSccControlRegister()
{
    return *controlRegister;
}


/*! \brief Read a UART read register.
 *
 * This function read the register not the cached copy of the write register. It does not have provisions to read alternate registers.
 * The difference between this function and registerRead8 is that it is intended to access the SCC registers and handle the
 * data/address multiplexed access.
 * @param regNbr The read register number to read.
 * @param val The pointer to where to put the value.
 * @retval Error status (always ERRCODE_NO_ERROR)
*/
int32_t	SccChannel::sccRegisterRead(uint8_t regNbr, volatile uint8_t* val)
{
    *val = *sccCtrlReg;
    if (regNbr)
        *sccCtrlReg = regNbr;

    *val = *sccCtrlReg;

    return ERRCODE_NO_ERROR;
}

/*! \brief Write to a UART write register.
 *
 * This function write to the UART register and store a cached copy. It does not have provisions to read alternate registers.
 * The difference between this function and registerWrite8 is that it is intended to access the SCC registers and handle the
 * data/address multiplexed access.
 * @param regNbr The read register number to read.
 * @param val The value to write.
 * @retval Error status (always ERRCODE_NO_ERROR)
*/
int32_t	SccChannel::sccRegisterWrite(uint8_t regNbr, uint8_t val)
 {
//    volatile uint8_t dummy = *sccCtrlReg;
    if (regNbr)
        *sccCtrlReg = regNbr;
    *sccCtrlReg = val;
    cachedRegs[regNbr] = val;

    return ERRCODE_NO_ERROR;
}

/*! \brief Read the UART receive register.
 *
 * This function should not be used if the DMA is enabled, since it might create a race condition.
 * @param val The pointer to where to put the value.
 * @retval Error status (always ERRCODE_NO_ERROR)
*/
int32_t	SccChannel::sccDataRead(uint8_t* val)
{
    *val = *sccDataReg;
    return ERRCODE_NO_ERROR;
}

/*! \brief Write to the UART transmit register.
 *
 * This function should not be used if the DMA is enabled.
 * @param val The value to write.
 * @retval Error status (always ERRCODE_NO_ERROR)
*/
int32_t	SccChannel::sccDataWrite(uint8_t val)
{
    *sccDataReg = val;
    return ERRCODE_NO_ERROR;
}

/*******************************************************************/
/* @brief Enables ESCC interrupts in R1 and R9
 * @param mask Bits corresponding to interrupts to enable.
 * @retval Error status (always ERRCODE_NO_ERROR)
*/

PCIeMini_status SccChannel::enableIntSCC(uint8_t mask)
{
    cachedRegs[R1] |= mask;
    sccRegisterWrite(R1, cachedRegs[R1]);		/* enable transmit interrupts */
    cachedRegs[R9] |= MIE;
    sccRegisterWrite(R9, cachedRegs[R9]);	/* INT. ENABLED */

    return ERRCODE_NO_ERROR;
}

/*******************************************************************/
/** @brief This subroutine disables ESCC interrupts 
 * @param mask Bits corresponding to interrupts to disable.
 * @retval Error status (always ERRCODE_NO_ERROR)
*/
PCIeMini_status SccChannel::disableIntSCC(uint8_t mask)
{
    cachedRegs[R1] &= ~mask;
    sccRegisterWrite(R1, cachedRegs[R1]);            /* disable all interrupts */
    cachedRegs[R9] &= ~(MIE);
    sccRegisterWrite(R9, cachedRegs[R9]);            /* INT. DISABLED */

    return ERRCODE_NO_ERROR;
}

static char* readRegNames[] = {
    "RR0: Status reg.  ","RR1               ","RR2: Irq Vect reg.","RR3: Irq Pending. ","RR4: Clk+config   ","RR5               ",
    "RR6               ","RR7               ","RR8: Rcv Data reg.","RR9               ","RR10              ","RR11              ",
    "RR12              ","RR13              ","RR14              ","RR15              "
};

static char* writeRegNames[] = {
    "WR0: Cmd Reg.     ","WR1: Irq Enable   ","WR2: Irq Vect reg.","WR3: Recv Parm.   ","WR4: Misc. Parm.  ","WR5: Tx Parm.     ",
    "WR6: Sync Char 1. ","WR7  Sync Char 2. ","WR8: Tx Data reg. ","WR9: Master Ctrl. ","WR10: Misc Ctrl.  ","WR11: Clock Ctrl. ",
    "WR12: BRGL        ","WR13: BRGH        ","WR14: Misc Bits   ","WR15: Irq Ctrl.   "
};

/** @brief Dump the content of the SCC registers on stdout 
 */
void SccChannel::dump_regs()
{
    int i;
    uint8_t rega;

    for (i = 0; i < 16; i++) {
        sccRegisterRead(i, &rega);
        if (rega != 0)
            std::cout << readRegNames[i] << std::hex << " - 0x" << std::hex << (int)rega << std::endl;
    }

    for (i = 0; i < 32; i++) {
        if (cachedRegs[i] != 0)
            std::cout << writeRegNames[i] << std::hex << " - 0x" << std::hex << (int)cachedRegs[i] << std::endl;
    }
    std::cout << "SCC Control register: 0x" << std::hex << *controlRegister << std::endl;
    std::cout << "SCC I/O Control register: 0x" << std::hex << buffer->getBuffer() << std::endl;
}
