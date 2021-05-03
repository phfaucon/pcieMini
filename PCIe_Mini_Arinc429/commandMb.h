#ifndef _COMMAND_MB_H
#define _COMMAND_MB_H

#include "AlphiDll.h"
#include "arinc429error.h"

typedef struct CommandMB
{
	volatile UINT32 commandWord;
	volatile UINT32 status;
	volatile UINT32 data;
} CommandMailBox;

class CommandMb
{
public:
	CommandMb(UINT32* mb);
	ARINC_status commandMbSendCommand(UINT32 data, int timeout);
	ARINC_status commandMbSendCommand(UINT32 data);

	inline bool CommandMb::isCommandMbBusy()
	{
		return cmdMb->status != 0;
	}


	static inline UINT32 composeCommandWord(UINT8 transferType, UINT8 cs, UINT8 chNbr, UINT8 cmd, UINT8 value)
	{
		return ((transferType << 24) | (cs << 16) | (chNbr << 20) | (cmd << 8) | value);
	}

	static inline UINT32 composeCommandWord(UINT8 transferType, UINT8 cs, UINT8 chNbr, UINT16 value)
	{
		return ((transferType << 24) | (cs << 16) | (chNbr << 20) | value);
	}

	inline UINT32 getData()
	{
		return cmdMb->data;
	}

	inline void setData(UINT32 val)
	{
		cmdMb->data = val;
	}

private:
	volatile CommandMailBox *cmdMb;
};

#endif