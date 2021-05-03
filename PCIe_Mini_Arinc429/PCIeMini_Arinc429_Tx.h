#pragma once
#include <iostream>
using namespace std;
#include "AlphiDll.h"

#include "Hi3593.h"
#include "arinc429error.h" 


//! This class represents the properties and behaviors of a transmit channel.
class DLL PCIeMini_Arinc429_Tx{
public:
	Hi3593* dev;			// HI 3593 object

	//! Constructor for the transmitter channel.  
	/*!
		This constructor is called when creating a connection to a board using the open method of the PCIeMiniArinc429 structure.
		It should not be called at any other time.
		@param controller A pointer to the HI-3593 controller object.
	*/
	PCIeMini_Arinc429_Tx(Hi3593 *controller);

	//! Configure a transmit channel 
	/*!
		\param config The structure containing the transmitter configuration information.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setConfig(Tx_configuration *config);

	//! Read back the configuration of a transmit channel 
	/*!
		\param config A pointer to where the structure will be stored.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status getConfig(Tx_configuration *config);

	//! Send an ARINC word to the transmit FIFO 
	/*!
		\param	w A pointer from where the ARINC429 word will be retrieved.
		\param nbr_requested size of the buffer in words.
		\param w nbr_transmitted number of words actually transmitted.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status transmit(UINT32 *w, int nbr_requested, int *nbr_transmitted);

	//! Check if the transmit FIFO is full 
	/*!
		@return  true if the FIFO is full.
	*/
	bool isFifoFull();

	//! Check if the transmit FIFO is half full 
	/*!
		@return  true if the FIFO is at least half full.
	*/
	bool isFifoHalfFull();

	//! Check if the transmit FIFO is empty 
	/*!
		@return  true if the FIFO is at empty.
	*/
	bool isFifoEmpty();	

	// register access
	//! Get the status register of a transmit channel 
	/*!
		\return  Returns the 8-bit status register value.
	*/
	UINT8 getStatusReg();

	//! Read the control register
	/*!
		@return  the value read from the control register.
	*/
	UINT8 getCtrlReg();

	//! Set the transmitter control register
	/*!
		@param  value the value to write in the control register.
		@return  ARINC429_NO_ERROR if the operation was successful.
	*/
	void setCtrlReg(UINT8 value);

private:
	//! Send an ARINC word to the transmit FIFO 
	/*!
		\param	w A pointer from where the ARINC429 word will be retrieved.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status transmitWord(UINT32 w);

};

