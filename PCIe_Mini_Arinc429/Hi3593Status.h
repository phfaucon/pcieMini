#pragma once
#include "arinc429error.h" 
#include "ParallelInput.h"

//! HI-3593 hardware status and interrupts
/*! This class allows to specify which status output pins of the HI-3593 can be used to generate interrupts
	and to verify the interrupt request status */
class Hi3593Status :
	public ParallelInput
{
public:
	/*! This constructor only takes address information to access the GPIO and does not do any any initialization of the chip itself
		it is called during the board object construction and should not be called by the user program
	@param gpio Pointer to the GPIO controller in the user address space
	 */
	Hi3593Status(volatile void *gpio);

	~Hi3593Status(void);

	//! Check if the receive priority mailboxes contain new data
	/*!
			@param channel "0" is channel 1, else channel 2
			@param nbr Maibox number: 0,1,2
			@return  true if the mailbox is not empty
	*/
	bool checkPriMb(UINT8 channel, UINT8 nbr);

	//! Check receive flag bit
	/*!		The meaning of the flag bit is set with the setRxFlagEnable method
			@param channel "0" is channel 1, else channel 2
			@return  true if the flag is set
	*/
	bool checkRxFlag(UINT8 channel);

	//! Check receive interrupt bit
	/*!		The meaning of the flag bit is set with the setRxIntEnable method
			@param channel "0" is channel 1, else channel 2
			@return  true if the flag is set 
	*/
	bool checkRxInt(UINT8 channel);

	//! Enable/Disable receiver interrupts
	/*!
			@param channel "0" is channel 1, else channel 2
			@param irqEnabled Enable or disable the interrupt request when the pin is pulsing
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setRxIntEnable(UINT8 channel, bool irqEnabled);

	//! Enable/Disable FIFO level interrupts
	/*!
			@param channel "0" is channel 1, else channel 2
			@param irqEnabled Enable or disable the interrupt request when the pin is steady high
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setRxFlagEnable(UINT8 channel, bool irqEnabled);

	//! Enable/Disable Mailbox interrupts
	/*!
			@param channel "0" is channel 1, else channel 2
			@param mbMask The 3 lower bits represent mailboxes 1 to 3, when the bit is set, the interrupt for that mailbox is enabled
			@return  ARINC429_NO_ERROR if successful. 
				ARINC429_INVALID_VALUE if the mask is incorrect (>7).
	*/
	ARINC_status setRxMbIrqEnable(UINT8 channel, UINT8 mbMask);

	//! Enable/Disable transmitter interrupts
	/*!
			@param fifoLevel "1" for empty, "2" for full, "3" for empty or full
			@param irqEnabled Enable or disable the interrupt request when the pin is pulsing
			@return  ARINC429_NO_ERROR if successful.
	*/
	ARINC_status setTxIntEnable(UINT8 fifoLevel, bool irqEnabled);

	ARINC_status setInterruptEnableMask(UINT32 irqEnable)
	{
		base[irqEnable_index] = irqEnable;
		return  ARINC429_NO_ERROR;
	}

	// interrupt mask
	static const int priLblMb1Pos = 0;
	static const int priLblMb1Mask = 0x007;
	static const int priLblMb2Pos = 3;
	static const int priLblMb2Mask = 0x038;
	static const int r1IntPos = 6;
	static const int r1IntMask = 0x040;
	static const int r1FlagPos = 7;
	static const int r1FlagMask = 0x080;
	static const int r2IntPos = 8;
	static const int r2IntMask = 0x100;
	static const int r2FlagPos = 9;
	static const int r2FlagMask = 0x200;
	static const int tEmptyPos = 10;
	static const int tEmptyMask = 0x400;
	static const int tFullPos = 11;
	static const int tFullMask = 0x800;

};

