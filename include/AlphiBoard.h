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
/** @file AlphiBoard.h
* @brief Base PCIe board class with Jungo driver and Altera PCIe hardware
*/

// Maintenance Log
//---------------------------------------------------------------------
//---------------------------------------------------------------------

#ifndef _ALPHI_BOARDS_H
#define _ALPHI_BOARDS_H
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include "AlphiDll.h"
#include <wdc_defs.h>
#include "PcieCra.h"
#include "AlteraDma.h"

//typedef void * WDC_DEVICE_HANDLE;
#define ErrLog printf
#define TraceLog printf

/** @brief minipcie_arinc429 diagnostics interrupt handler function type */
typedef void (*MINIPCIE_INT_HANDLER)(void *pIntData);

/** @brief minipcie_arinc429 diagnostics plug-and-play and power management events handler function
 * type */
typedef void (*MINIPCIE_EVENT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    DWORD dwAction);

/** @brief Minipcie Device Information Structure */
typedef struct {
    MINIPCIE_INT_HANDLER    funcDiagIntHandler;    ///< Interrupt handler routine 
    MINIPCIE_EVENT_HANDLER  funcDiagEventHandler;  ///< Event handler routine 
	void* userData;								   ///< Data passed to the interrupt routine
} MINIPCIE_DEV_CTX, *PMINIPCIE_DEV_CTX;
/* TODO: You can add fields to store additional device-specific information. */

/** @brief Memory Segment Descriptor */
typedef struct LinearAddress {
	void* Address;			///< Linear address.
	size_t   Length;		///< Length of the mapping.
} LinearAddress;

/** @brief Interrupt result information structure */
typedef struct
{
	DWORD dwCounter; ///< Number of interrupts received 
	DWORD dwLost;    ///< Number of interrupts not yet handled 
	WD_INTERRUPT_WAIT_RESULT waitResult; ///< See WD_INTERRUPT_WAIT_RESULT values in windrvr.h
	DWORD dwEnabledIntType; ///< Interrupt type that was actually enabled (MSI/MSI-X/Level Sensitive/Edge-Triggered) 
	DWORD dwLastMessage; /**< Message data of the last received MSI/MSI-X
							(Windows Vista and higher); N/A to line-based
							interrupts) */
} MINIPCIE_INT_RESULT;

/** @brief Board Hardware identification and version */
class DLL BoardVersion {
public:
	BoardVersion(volatile uint32_t* addr);

	uint32_t getVersion();		//!<  Version, if there is one programmed on the board hardware. Typically 0.
	time_t getTimeStamp();		//!<  Date when the board firmware was compiled.
private:
	volatile uint32_t* address;
};

#define CLOCK_REALTIME 0

/*
* Table 5-14: Avalon-MM to PCI Express Interrupt Enable Register, 0x0050
Bits	Name				Access	Description
[31:24] Reserved			N/A		N/A
[23:16] A2P_MB_IRQ			RW		Enables generation of PCI Express interrupts when a specified mailbox is
									written to by an external Avalon-MM master.
[15:0]	AVL_IRQ[15:0]				RW Enables generation of PCI Express interrupts when a specified Avalon-MM
									interrupt signal is asserted. Your system may have as many as 16 individual
									input interrupt signals
*/
/*
Table 5-13: Avalon-MM to PCI Express Interrupt Status Register, 0x0040
Bit		Name				Access	Description
[31:24] Reserved			N/A		N/A
[23]	A2P_MAILBOX_INT7	RW1C	Set to 1 when the A2P_MAILBOX7 register is written to
[22]	A2P_MAILBOX_INT6	RW1C	Set to 1 when the A2P_MAILBOX6 register is written to
[21]	A2P_MAILBOX_INT5	RW1C	Set to 1 when the A2P_MAILBOX5 register is written to
[20]	A2P_MAILBOX_INT4	RW1C	Set to 1 when the A2P_MAILBOX4 register is written to
[19]	A2P_MAILBOX_INT3	RW1C	Set to 1 when the A2P_MAILBOX3 register is written to
[18]	A2P_MAILBOX_INT2	RW1C	Set to 1 when the A2P_MAILBOX2 register is written to
[17]	A2P_MAILBOX_INT1	RW1C	Set to 1 when the A2P_MAILBOX1 register is written to
[16]	A2P_MAILBOX_INT0	RW1C	Set to 1 when the A2P_MAILBOX0 register is written to
[15:0]	AVL_IRQ_ASSRTD		RO		Current value of the Avalon-MM interrupt
									(IRQ) input ports to the Avalon-MM RX master port:
										• 0—Avalon-MM IRQ is not being signaled.
										• 1—Avalon-MM IRQ is being signaled.
									A PCIe variant may have as many as 16 distinct IRQ input ports.
									Each AVL_IRQ_ASSERTED[] bit reflects the value on the
									corresponding IRQ input port.
*/

//! Base class implementing a PCIe board and the Jungo driver.
class DLL AlphiBoard
{
public:
	LinearAddress bar0;		///< Memory descriptor for the BAR0 in user memory
	LinearAddress bar2;		///< Memory descriptor for the BAR2 in user memory
	LinearAddress bar3;		///< Memory descriptor for the BAR3 in user memory
	PcieCra* cra;			///< PCIe Interface instance
	BoardVersion* sysid;	///< Board identification
	WD_DMA* pDma;			///< Jungo DMA structure.

	AlphiBoard(UINT16 vendorId, UINT16 deviceId);

	~AlphiBoard(void);

	HRESULT Open(int brdNbr);

	/** @brief reset some of the board resources
	 *
	 */
	inline DWORD reset()
	{
		cra->reset();
		return WD_STATUS_SUCCESS;
	}

	uint32_t getFpgaID();
	time_t getFpgaTimeStamp();

	void setVerbose(int verbose);
	bool IsValidDevice(const CHAR*sFunc);

	DWORD hookInterruptServiceRoutine(uint32_t mask, MINIPCIE_INT_HANDLER uicr, void* userData);
	DWORD hookInterruptServiceRoutine(MINIPCIE_INT_HANDLER uicr);
	DWORD getIntResults(MINIPCIE_INT_RESULT* intResult);

	DWORD unhookInterruptServiceRoutine();

	DWORD enableInterrupts(uint16_t mask = 0xffff);
	DWORD disableInterrupts();

	DWORD Close();

	volatile void* getBar0Address(size_t offset);
	volatile void* getBar2Address(size_t offset);
	volatile void* getBar3Address(size_t offset);

	bool DMARoutine(DWORD dwDMABufSize,
		uint32_t u32LocalAddr, bool fPolling, bool fToDev,
		TransferDesc* tfrDesc);
	PCIeMini_status DMAOpen(uint32_t u32LocalAddr,
		DWORD dwDMABufSize, bool fToDev, TransferDesc* tfrDesc);
	void DMAClose(bool fPolling);
	void DMATransfer(TransferDesc* tfrDesc, bool fPolling);

	virtual void hwDMAStart(TransferDesc* tfrDesc);
	virtual bool hwDMAWaitForCompletion(TransferDesc* tfrDesc, bool fPolling);
	virtual bool hwDMAInterruptEnable(MINIPCIE_INT_HANDLER MyDmaIntHandler, void* pDMA);
	virtual void hwDMAInterruptDisable();
	virtual void hwDMAProgram(
		WD_DMA_PAGE* Page,
		DWORD dwPages,
		bool fToDev,
		uint32_t u32LocalAddr,
		TransferDesc* tfrDesc);

	/** @brief Millisecond Delay Function */
	static inline void MsSleep(int ms)
	{
		Sleep(ms);
	}

	int verbose;		///< Flag used by various functions to determine the amount of messages to generate
	DWORD libStatus;	///< Status returned when trying to open the Jungo library. If it is not WD_STATUS_SUCCESS, the initialization failed.

private:
	// jungo specific data
	WDC_DEVICE_HANDLE hDev;
	PWDC_DEVICE pDev;
	WD_PCI_SLOT slot;
	WD_PCI_CARD_INFO deviceInfo;
	MINIPCIE_DEV_CTX devCtx;

	static const UINT16 defaultVendorId = 0x13c5;
	static const UINT16 defaultDeviceId = 0x0508;
	DWORD dwVendorId;
	DWORD dwDeviceId;
 
	CHAR gsMINIPCIE_ARINC429_LastErr[256];
	DWORD LibInit_count;

	DWORD LibInit(void);
	bool  DeviceValidate();
	DWORD DeviceFind(int nbr);
	DWORD DeviceOpen();

	DWORD printAddressSpace(WDC_ADDR_DESC* desc);

	// Board configuration
	static const uint32_t	sysid_offset = 0x0000;		// 0x0000_0007
	static const uint32_t	cra_address = 0x0000;		// in BAR 0
};

#endif
