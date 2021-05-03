#include <string>
#include <iostream>
#include <time.h>
#include "commandMb.h"

CommandMb::CommandMb(UINT32* mb)
{
	cmdMb = (CommandMailBox *)mb;
	// initialize the command mailbox
	cmdMb->status = 0;		// empty the command mailbox
}

ARINC_status CommandMb::commandMbSendCommand(UINT32 data, int timeOut)
{
	// wait for the interface not to be busy
	while (isCommandMbBusy() && timeOut-- > 0)
		Sleep(1);

	// check for timeout
	if (timeOut<=0)
	{
		std::cout << "Error: commandMbSendCommand: ARINC429_BUSY!!!!\n";
		return ARINC429_BUSY;
	}

	// send the command
	cmdMb->commandWord = data;
	cmdMb->status = 1;
	cmdMb->data = 0x55555555;		// place marker since we send at the same time as we receive

	// wait for the transfer to complete
	timeOut = 1500;
	while (isCommandMbBusy()) {
		if (timeOut-- <= 0) {
			std::cout << "Error: commandMbSendCommand: Command time-out!!!!" << std::hex << data << ", cmdDb:" << (UINT32)cmdMb << "\n";
			return ARINC429_TIMEOUT;
		}
		Sleep(1);
	}
	//	Success!!!!
	return ARINC429_SUCCESS;
}

ARINC_status CommandMb::commandMbSendCommand(UINT32 data)
{
	return commandMbSendCommand(data, 10000);
}
	