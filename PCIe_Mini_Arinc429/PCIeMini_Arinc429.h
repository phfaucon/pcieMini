#pragma once
#include <iostream>
using namespace std;
#include "AlphiDll.h"
#include "AlphiBoard.h"
#include "Hi3593.h"
#include "Hi3593Status.h"
#include "arinc429error.h" 
#include "PCIeMini_Arinc429_Rx.h"
#include "PCIeMini_Arinc429_Tx.h"
#include "commandMb.h"

//! This class contains information about the board version
class DLL Arinc429version {
public:
	/*! This constructor reads the chip register to initialize the data. It is called by the open and should not be called by the user.
	@param addr Pointer to the sysid controller in the user address space
	 */
	Arinc429version(UINT32 * addr);

	UINT32 getVersion();		//!<  Version, if there is one programmed on the board hardware. Typically 0.
	UINT32 getTimeStamp();		//!<  Date when the board firmware was compiled.
private:
	UINT32 *address;
};

//! This class defines a Arinc429 controller board object
class DLL PCIeMini_Arinc429 : public AlphiBoard
{
public:
	/*! The constructor does not take any parameter. The board is not actually
	 usable until the open method connects it to real hardware.
	 */
	PCIeMini_Arinc429();

	//! Open: connect to an actual board
	/*!
		\param brdNbr The board number is actually system dependent but if you have only one board, it should be 0.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status open(int brdNbr);

	//! Close the connection to a board object and free the resources
	/*!
			\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status close();

	//! Reset the board ARINC 429 controllers
	/*!
			\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status reset();

	//! Return the version of the board
	/*!
			@param version Pointer to a Arinc429version structure
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status getDriverVersion(Arinc429version * version);

	//! Return a text description corresponding to an error code
	/*!
			\return  A pointer to a null terminated character string.
	*/
	static char * getErrorMsg(ARINC_status errorNbr);

	//! Read the values of the lines out of the Hi3593.
	/*!
		debug only.
			@return  the input value.
	*/
	UINT32 getGpioInputValues(int gpioNumber);

	ARINC_status stop();
	ARINC_status start();


	void setLedPio(UINT32);
	UINT32 getLedPio();

	static const int nbrOfArincControllers = 4;						//!<  Number of HI-3593 Arinc controllers
	static const int nbrOfTxChannels = nbrOfArincControllers;		//!<  Number of transmitter channels
	static const int nbrOfRxChannels = nbrOfArincControllers * 2;	//!<  Number of receiver channels
	PCIeMini_Arinc429_Rx* rx[8];									//!< array of receiver objects, 8 in total
	PCIeMini_Arinc429_Tx* tx[4];									//!< array of transmitter objects, 4 in total
//	Hi3593Status	*irqStatus[nbrOfArincControllers];
	Hi3593Status *gpio[nbrOfArincControllers];

	volatile UINT32		*ledPio;

private:
	// Board configuration
	static const UINT32	led_pio_offset	= 0x040;
	static const UINT32	gpio_0_offset	= 0x080;
	static const UINT32	gpio_1_offset	= 0x0a0;
	static const UINT32	gpio_2_offset	= 0x0c0;
	static const UINT32	gpio_3_offset	= 0x0e0;
	static const UINT32	hi3593_offset	= 0x200;
	static const UINT32	dpr_offset		= 0x4000;


	Hi3593			*ArincController[nbrOfArincControllers];
	bool			isOpen;
	UINT32			*dpr;
	CommandMb		*cmdMb;
	UINT32			*spiController;
	MINIPCIE_INT_HANDLER uicr;
	bool interruptEnabled;
};
