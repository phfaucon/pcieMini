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
#include "wdc_lib.h"
#include "wdc_defs.h"
#include "utils.h"
#include "AlphiErrorCodes.h"
#include "AlphiBoard.h"
#include "status_strings.h"
#include "wdc_diag_lib.h" 
#include "wdc_lib.h" 
#include <iostream>

using namespace std;

/*************************************************************
Internal definitions
*************************************************************/
/* WinDriver license registration string */
/* TODO: When using a registered WinDriver version, replace the license string
below with your specific WinDriver license registration string and
replace the driver name below with your driver's name. */
#define MINIPCIE_ARINC429_DEFAULT_LICENSE_STRING ((CHAR *)"872759db47d9ae7988a60332b89b6ea9c386010a7656b25cfdb4076053056c6f590d.WD1440_64_NL_Alphi_Technology_Corporation-DIS")
//#define MINIPCIE_ARINC429_DEFAULT_LICENSE_STRING ((CHAR *)"872759e7d000fb5af5a32f34187d67383cda3d1a517f41439aeab539ae35.WD1270_64_NL_Alphi_Technology")
#define MINIPCIE_ARINC429_DEFAULT_DRIVER_NAME WD_DEFAULT_DRIVER_NAME_BASE
/* Library initialization reference count */

DWORD AlphiBoard::LibInit(void)
{
	DWORD dwStatus;

	/* Increase the library's reference count; initialize the library only once
	*/
	if (++LibInit_count > 1)
		return WD_STATUS_SUCCESS;

	/* Set WDC library's debug options
	* (default: level=TRACE; redirect output to the Debug Monitor) */
	dwStatus = WDC_SetDebugOptions((WDC_DBG_OPTIONS)WDC_DBG_DEFAULT, (const CHAR *)0);
	if (WD_STATUS_SUCCESS != dwStatus)
	{
		ErrLog("Failed to initialize debug options for WDC library.\n"
			"Error 0x%lx - %s\n", dwStatus, Stat2Str(dwStatus));

		return dwStatus;
	}

	/* Open a handle to the driver and initialize the WDC library */
	dwStatus = WDC_DriverOpen(WDC_DRV_OPEN_DEFAULT,
			MINIPCIE_ARINC429_DEFAULT_LICENSE_STRING);
	if (WD_STATUS_SUCCESS != dwStatus)
	{
		ErrLog("Failed to initialize the WDC library. Error 0x%lx - %s\n",
			dwStatus, Stat2Str(dwStatus));

		return dwStatus;
	}

	return WD_STATUS_SUCCESS;
}

AlphiBoard::AlphiBoard(UINT16 vendorId, UINT16 deviceId)
{
	dwVendorId = vendorId;
	dwDeviceId = deviceId; 
	/* Initialize the minipcie_arinc429 library */
	LibInit_count = 0;
	LibInit();
	pDev = NULL;
	verbose = false;
}

//! Destructor
/*!
	Will close the connection to the board if needed.
*/
AlphiBoard::~AlphiBoard(void)
{
	Close();
}

//! Open a board
/*!
Establishes a connection to a board.
	@param brdNbr the board index to open.
	@return WD_DEVICE_NOT_FOUND if there is no board corresponding to the number
*/
HRESULT AlphiBoard::Open(int brdNbr)
{
    DWORD dwStatus;

	/* Find device */
    dwStatus = DeviceFind(brdNbr);
	if (WD_STATUS_SUCCESS != dwStatus) {    
		if (verbose) {
			cout << "DeviceFind for board#" << brdNbr << " failed with error: "<< getAlphiErrorMsg(dwStatus) << endl;
		}
		return WD_DEVICE_NOT_FOUND;
	}

    /* Open a device handle */
    return DeviceOpen();
}

/** @brief set the verbose flag
 *
 * The verbose value is used to send more information to the log file or console. It is only partially
 * implemented.
 @param vb Verbosity level.
 */
void AlphiBoard::setVerbose(int vb)
{
	verbose = verbose;
}

/** @brief Establishes a connection to a board.
* 
* Returns a pointer to an address space. Only BAR 0 and 2 are recognized.
	@param bar The number of the bar to access.
	@param addr The LinearAddress structure where to put the memory information.
	@return WD_STATUS_SUCCESS when the bar is accessible
		WD_NO_RESOURCES_ON_DEVICE if there is no corresponding BAR.
*/
DWORD AlphiBoard::Map(int bar, LinearAddress *addr)
{
	switch (bar) {
	case 0:
		addr->Address = bar0.Address;
		addr->Length = bar0.Length;
		break;
	case 2:
		addr->Address = bar2.Address;
		addr->Length = bar2.Length;
		break;
	default:
		return WD_NO_RESOURCES_ON_DEVICE;
	}
	return WD_STATUS_SUCCESS;
}

/** @brief Release a memory segment
 *
 * Not used with the Jungo driver
 * @retval WD_STATUS_SUCCESS
 */
DWORD AlphiBoard::Unmap(LinearAddress &Address)
{
	return WD_STATUS_SUCCESS;
}

/** Validate device information 
 * @retval true if the device is valid
 */
bool AlphiBoard::DeviceValidate()
{
    DWORD i, dwNumAddrSpaces = pDev->dwNumAddrSpaces;

    /* TODO: You can modify the implementation of this function in order to
             verify that the device has all expected resources. */

    /* Verify that the device has at least one active address space */
    for (i = 0; i < dwNumAddrSpaces; i++)
    {
        if (WDC_AddrSpaceIsActive(pDev, i))
            return true;
    }

    /* No valid address spaces */
    if (verbose) TraceLog("Device does not have any active memory or I/O address spaces\n");
    return false;
}

/* -----------------------------------------------
    Device find, open and close
   ----------------------------------------------- */
/** @brief Find a minipcie_arinc429 device 
 * @param devNbr Device number between 0 and 9
 * @retval A Jungo error code
*/
DWORD  AlphiBoard::DeviceFind(int devNbr)
{
    DWORD dwStatus;
    DWORD i, dwNumDevices;
    WDC_PCI_SCAN_RESULT scanResult;

     /* Scan PCI devices */
    BZERO(scanResult);
    dwStatus = WDC_PciScanDevices(dwVendorId, dwDeviceId, &scanResult);
    if (WD_STATUS_SUCCESS != dwStatus)
    {
        return dwStatus;
    }

    dwNumDevices = scanResult.dwNumDevices;
    if (devNbr > (int)dwNumDevices)
    {
        return WD_DEVICE_NOT_FOUND;
    }

	if (verbose) {
		/* Display matching devices information */
		cout << "\nFound " << dwNumDevices << " matching device" << (dwNumDevices > 1 ? "s " : " ") <<
			"[Vendor ID 0x" << hex << dwVendorId << "], Device ID 0x" << hex << dwDeviceId << "]" << endl;

		for (i = 0; i < dwNumDevices; i++)
		{
			cout << endl << i + 1 << ". Vendor ID: 0x"<< hex <<
				scanResult.deviceId[i].dwVendorId << ", Device ID : 0x" <<
				hex << scanResult.deviceId[i].dwDeviceId;
		}
		cout << endl;
	}

    /* Select device */

    slot = scanResult.deviceSlot[devNbr];

    return WD_STATUS_SUCCESS;
}

/** @brief Open a handle to a minipcie_arinc429 device 
 * @retval Error code
 */
DWORD AlphiBoard::DeviceOpen()
{
    DWORD dwStatus;
	DWORD retCode = WD_STATUS_SUCCESS;
 
	/* Retrieve the device's resources information */
    BZERO(deviceInfo);
    deviceInfo.pciSlot = slot;
    dwStatus = WDC_PciGetDeviceInfo(&deviceInfo);
    if (WD_STATUS_SUCCESS != dwStatus && verbose)
    {
		cout << "WDC_PciGetDeviceInfo failed!" << endl;
        return dwStatus;
    }

    /* NOTE: You can modify the device's resources information here, if
       necessary (mainly the deviceInfo.Card.Items array or the items number -
       deviceInfo.Card.dwItems) in order to register only some of the resources
       or register only a portion of a specific address space, for example. */

    /* Open a handle to the device */
    /* Open a WDC device handle */
    dwStatus = WDC_PciDeviceOpen(&hDev, &deviceInfo, &devCtx);
    if (dwStatus == WD_FAILED_KERNEL_MAPPING)
    {
        DWORD i;

        for (i = 0; i < deviceInfo.Card.dwItems; i++)
        {
            /* We do not need to map to kernel because there is no access to
             * resource in kernel mode */
            if (deviceInfo.Card.Item[i].item == ITEM_MEMORY)
                deviceInfo.Card.Item[i].I.Mem.dwOptions =
                    WD_ITEM_MEM_DO_NOT_MAP_KERNEL;
        }
        dwStatus = WDC_PciDeviceOpen(&hDev, &deviceInfo, &devCtx);
    }
    if (WD_STATUS_SUCCESS != dwStatus)
    {
		if (verbose)
			cout << "error returned by WDC_PciDeviceOpen: " << Stat2Str(dwStatus)<< endl;

		retCode = dwStatus;
        goto Error;
    }
	pDev = (PWDC_DEVICE)hDev;

	// retrieve bar information
	for (uint32_t i = 0; i < pDev->dwNumAddrSpaces; i++) {
		WDC_ADDR_DESC *desc = &pDev->pAddrDesc[i];
		if (verbose) {
			printAddressSpace(desc);
		}
		if (desc->dwAddrSpace == 0 && desc->fIsMemory) {
			bar0.Address = (void*)desc->pUserDirectMemAddr;
			bar0.Length = desc->qwBytes;
		}
		if (desc->dwAddrSpace == 2 && desc->fIsMemory) {
			bar2.Address = (void*)desc->pUserDirectMemAddr;
			bar2.Length = desc->qwBytes;
		}

	}
	
	/* Validate device information */
	if (!DeviceValidate()){ 
		retCode = WD_NO_RESOURCES_ON_DEVICE;
		goto Error;
	}

    /* Return handle to the new device */
    if (verbose)
		TraceLog("DeviceOpen: Opened a minipcie device (handle 0x%p)\n",
			hDev);
    return WD_STATUS_SUCCESS;

Error:
    if (hDev)
        Close();

    return retCode;
}

/** @brief Output an Address space information struct values on the console
* @param desc A pointer to a Jungo address descriptor
* @retval A Jungo return code
*/
DWORD AlphiBoard::printAddressSpace(WDC_ADDR_DESC* desc)
{
	cout << "Address space number = " << (int)desc->dwAddrSpace << endl;            /* Address space number */
	cout << "Item index = " << desc->dwItemIndex;            /* Index of address space in the pDev->cardReg.Card.Item array */
	if (desc->fIsMemory)
	{
		cout << "Address space type = Memory, BAR #" << desc->dwAddrSpace << endl;
	}
	else {
		cout << "Address space type = I/O";
	}
	cout << "Size = " << desc->qwBytes << endl;			    /* Memory address for direct user-mode access */
	cout << "kernel mem address = " << desc->pAddr << endl; /* I/O / memory kernel mapped address -- for
			//							   WD_Transfer(), WD_MultiTransfer(), or direct
			//							   kernel access */
	cout << "Local address = " << desc->pUserDirectMemAddr << endl;        /* Memory address for direct user-mode access */

	return WD_STATUS_SUCCESS;
}

/** @brief Close a device handle 
	@return status, a Jungo status code
*/
DWORD AlphiBoard::Close()
{
	DWORD dwStatus;
//	PWDC_DEVICE pDev = (PWDC_DEVICE)hDev;
//	PMINIPCIE_DEV_CTX pDevCtx;

	TraceLog("DeviceClose entered. Device handle: 0x%p\n", hDev);

	/* Validate the WDC device handle */
	if (!hDev)
	{
		ErrLog("Error - NULL device handle\n");
		return WD_DEVICE_NOT_FOUND;
	}

//	pDevCtx = (PMINIPCIE_DEV_CTX)WDC_GetDevContext(pDev);

	/* Disable interrupts (if enabled) */
	if (WDC_IntIsEnabled(hDev))
	{
		dwStatus = DisableInterrupts();
		if (WD_STATUS_SUCCESS != dwStatus)
		{
			ErrLog("Failed disabling interrupts. Error 0x%lx - %s\n",
				dwStatus, Stat2Str(dwStatus));
		}
	}

	/* Close the device */
	dwStatus = WDC_PciDeviceClose(hDev);
	if (WD_STATUS_SUCCESS != dwStatus)
	{
		ErrLog("Failed closing a WDC device handle (0x%p). Error 0x%lx - %s\n",
			hDev, dwStatus, Stat2Str(dwStatus));
	}

	return dwStatus;
}

/** @brief Validate a WDC device handle 
 * @param sFunc C-string with name of the function e.g. "IntEnable" 
 * @retval true if the device context exists.
 */
bool AlphiBoard::IsValidDevice(const CHAR *sFunc)
{
	void *userContext = NULL;
	if (!pDev)
		userContext = WDC_GetDevContext(pDev);

	if (userContext != NULL)
	{
		snprintf(gsMINIPCIE_ARINC429_LastErr, sizeof(gsMINIPCIE_ARINC429_LastErr)-1, "%s: NULL "
			"device %s\n", sFunc, !pDev ? "handle" : "context");
		ErrLog(gsMINIPCIE_ARINC429_LastErr);
		return FALSE;
	}

	return TRUE;
}

/** @brief Return a pointer to an object in BAR 0
 *
 * @param offset Offset in BAR0
 * @retval Pointer to the object
 */
volatile void* AlphiBoard::getBar0Address(size_t offset)
{
	if (offset >= bar0.Length) return NULL;

	return (void*)((char*)bar0.Address + offset);
}

/** @brief Return a pointer to an object in BAR 2
 *
 * @param offset Offset in BAR2
 * @retval Pointer to the object
 */
volatile void* AlphiBoard::getBar2Address(size_t offset)
{
	if (offset >= bar2.Length) return NULL;

	return (void*)((char*)bar2.Address + offset);
}

BoardVersion::BoardVersion(volatile uint32_t* addr)
{
	address = addr;
}

uint32_t BoardVersion::getVersion()		//!<  Version, if there is one programmed on the board hardware. Typically 0.
{
	return address[0];
}

time_t BoardVersion::getTimeStamp()		//!<  Date when the board firmware was compiled.
{
	return address[1];
}

