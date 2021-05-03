#pragma once
#include <iostream>
using namespace std;
#include "AlphiDll.h"
#include "Hi3593.h"
#include "Hi3593Status.h"
#include "arinc429error.h" 

//! This class represents the properties and behaviors of a receive channel.
class DLL PCIeMini_Arinc429_Rx{
public:
	//! Constructor for a receiver channel.  
	/*!
		This constructor is called when creating a connection to a board using the open method of the PCIeMiniArinc429 structure.
		It should not be called at any other time.
		@param controller A pointer to the HI-3593 controller object.
		@param gpio A pointer to the GPIO reading the status output signals from the HI-3593 and generating interrupts if so programmed.
		@param channelNbr The device number on the SPI bus corresponding to the controller.
	*/
	PCIeMini_Arinc429_Rx(Hi3593 *controller, Hi3593Status *gpio, int channelNbr);

	//! Configure a receive channel 
	/*!
		\param config The structure containing the receiver configuration information.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setConfig(Rx_configuration *config);

	//! Update the acceptance of one label in the label filter 
	/*!
		Allows updating the label filter without updating the complete configuration. The label filter table
		is used only is programmed as such in the control register.
		\param labelNbr Number of the label to update.
		\param onOff "0" if the label is now rejected, else the label is accepted.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setLabel(UINT8 labelNbr, UINT8 onOff);

	//! Update the acceptance of one label in the label filter 
	/*!
		Allows updating the label filter without updating the complete configuration. The label filter table
		is used only is programmed as such in the control register.
		\param labelNbr Number of the label to update.
		\param labelTable Table of labels: 32 8-bit values, each bit corresponding to a label. "0" if the label is now rejected, else the label is accepted.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setLabels(UINT8 labelNbr, UINT8 *labelTable);

	//! Read back the configuration of a receive channel 
	/*!
		\param config A pointer to where the structure will be stored.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status getConfig(Rx_configuration *config);

	//! Receive from the receive FIFO 
	/*!
		\param w A pointer to where the ARINC429 words will be stored.
		\param nbr_requested size of the buffer in words.
		\param w nbr_received number of words actually received.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status receive(UINT32 *w, int nbr_requested, int *nbr_received);

	//! Receive from the receive FIFO 
	/*!
		\param w A pointer to where the ARINC429 words will be stored.
		\param nbr_requested size of the buffer in words.
		\param w nbr_received number of words actually received.
		\param time_out milliseconds to wait for missing words.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status receive(UINT32 *w, int nbr_requested, int *nbr_received, int time_out);

	// status registers
	//! Check if the receive FIFO is full 
	/*!
		@return  true if the FIFO is full.
	*/
	bool isFifoFull();

	//! Check if the receive FIFO is half full 
	/*!
		@return  true if the FIFO is at least half full.
	*/
	bool isFifoHalfFull();

	//! Check if the receive FIFO is empty 
	/*!
		@return  true if the FIFO is at empty.
	*/
	bool isFifoEmpty();

	// register access
	//! Read the status register
	/*!
		@return  the value read from the status register.
	*/
	UINT8 getStatusReg();

	//! Read the control register
	/*!
		@return  the value read from the control register.
	*/
	UINT8 getCtrlReg();

	//! Set the control register
	/*!
		@param  value the value to write in the control register.
		@return  ARINC429_NO_ERROR if the operation was successful.
	*/
	void setCtrlReg(UINT8);

	// Status PIO programmation
	//! Check if the receive priority mailboxes contain new data
	/*!
			@param nbr Maibox number: 0,1,2
			@return  true if the mailbox is not empty
	*/
	bool checkPriMb(UINT8 nbr);

	//! Check receive flag bit
	/*!		The meaning of the flag bit is set with the setRxFlagEnable method
			@return  true if the flag is set
	*/
	bool checkRxFlag();

	//! Check receive interrupt bit
	/*!		The meaning of the flag bit is set with the setRxIntEnable method
			@return  true if the flag is set 
	*/
	bool checkRxInt();

	//! Program the Int output type and Enable/Disable receiver interrupts
	/*!
			Receiver mask to select which condition will generate an interrupt when a word received: 
			- bit 0: when set, an interrupt will be generated when a word is received in the FIFO
			- bit 1: when set, interrupt will be generated when a word is received in mailbox #1
			- bit 2: when set, interrupt will be generated when a word is received in mailbox #2
			- bit 3: when set, interrupt will be generated when a word is received in mailbox #3
			@param mask Receiver mask 
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setRxIntEnable(UINT8 mask);

	//! Program the Flag output type and Enable/Disable FIFO level interrupts
	/*!
			@param flagType Select FIFO level that will generate an interrupt
			@param irqEnabled Enable or disable the interrupt request when the pin is steady high
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setRxFlagEnable(Hi3593::rxFifoLevel flagType, bool irqEnabled);

	private:
	Hi3593 *dev;
	Hi3593Status *hi3593Status;
	int chNbr;
	//! Read a word from the receive FIFO 
	/*!
		\param w A pointer to where the ARINC429 word will be stored.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status receiveWord(UINT32 *w);

	//! Read a word from the receive FIFO with timeout
	/*!
		\param w A pointer to where the ARINC429 word will be stored.
		\return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status receiveWord(UINT32 *w, int timeout);
};

