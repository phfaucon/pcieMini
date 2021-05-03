//#include <windows.h>
#include "stdio.h" 
#include "arinc429error.h" 

Dll char * getArinc429ErrorMsg(ARINC_status errCode)
{
	static char ErrMsg[256];
	switch (errCode) {
	case  ARINC429_NO_ERROR:
		strcpy(ErrMsg, "No error.");
		break;
	case  ARINC429_INTERNAL_ERROR:
		strcpy(ErrMsg, "Internal error.");
		break;
	case  ARINC429_INVALID_BOARD_NUM:
		strcpy(ErrMsg, "Invalid board number.");
		break;
	case  ARINC429_INVALID_HANDLE:
		strcpy(ErrMsg, "Invalid handle.");
		break;
	case  ARINC429_INVALID_CHANNEL_NUM:
		strcpy(ErrMsg, "Invalid channel number.");
		break;
	case  ARINC429_INVALID_SELF_TEST_ENABLE_VAL:
		strcpy(ErrMsg, "Invalid self-test-enable value.");
		break;
	case  ARINC429_INVALID_VALUE:
		strcpy(ErrMsg, "Invalid value.");
		break;
	case  ARINC429_INPUT_MODE:
		strcpy(ErrMsg, "Input mode error.");
		break;
	case  ARINC429_OUTPUT_MODE:
		strcpy(ErrMsg, "Output mode error.");
		break;
	case  ARINC429_INVALID_LOGIC_SEL:
		strcpy(ErrMsg, "Invalid logic selection.");
		break;
	case  ARINC429_INVALID_STROBE_MODE:
		strcpy(ErrMsg, "Invalid strobe mode.");
		break;
	case  ARINC429_INVALID_GROUP:
		strcpy(ErrMsg, "Invalid group.");
		break;
	case  ARINC429_INVALID_FREQUENCY:
		strcpy(ErrMsg, "Invalid frequency.");
		break;
	case  ARINC429_INVALID_INPUT_MODE:
		strcpy(ErrMsg, "Invalid input mode.");
		break;
	case  ARINC429_INVALID_MASK_VALUE:
		strcpy(ErrMsg, "Invalid mask value.");
		break;
	case  ARINC429_INVALID_MODE:
		strcpy(ErrMsg, "Invalid mode.");
		break;
	case  ARINC429_INVALID_SELF_TEST_DATA:	
		strcpy(ErrMsg, "Invalid self-test data.");
		break;
	case  ARINC429_SELF_TEST_DISABLE:
		strcpy(ErrMsg, "Self test disabled.");
		break;
	case  ARINC429_FAILED_SELF_TEST:
		strcpy(ErrMsg, "Failed self test.");
		break;
	case  ARINC429_INVALID_TIME_BOUNCE_VAL:	
		strcpy(ErrMsg, "Invalid time bounce value.");
		break;
	case  ARINC429_INT_ALREADY_ENABLED:
		strcpy(ErrMsg, "Interrupt already enabled.");
		break;
	case  ARINC429_INT_NOT_ENABLED:
		strcpy(ErrMsg, "Interrupt not enabled.");
		break;
	case  ARINC429_INVALID_TRANRECVSTS:
		strcpy(ErrMsg, "Invalid sts value for the clear FIFO.");
		break;
	case  ARINC429_INVALID_TRANSNUMBER:
		strcpy(ErrMsg, "Invalid transmitter number.");
		break;
	case  ARINC429_INVALID_RECVNUMBER:
		strcpy(ErrMsg, "Invalid receiver number.");
		break;
	case  ARINC429_INVALID_TRPAIR:
		strcpy(ErrMsg, "Invalid transmitter receiver pair.");
		break;
	case  ARINC429_INVALID_TRANSSTATUS:
		strcpy(ErrMsg, "Invalid transmitter status.");
		break;
	case  ARINC429_INVALID_LABELNUMBER:
		strcpy(ErrMsg, "Invalid label number.");
		break;
	case  ARINC429_INVALID_SDI:
		strcpy(ErrMsg, "Invalid sdi value.");
		break;
	case  ARINC429_INVALID_PARITY:
		strcpy(ErrMsg, "Invalid parity.");
		break;
	case  ARINC429_INVALID_DATARATE:
		strcpy(ErrMsg, "Invalid data rate.");
		break;
	case  ARINC429_INVALID_DATALENGTH:
		strcpy(ErrMsg, "Invalid data length.");
		break;
	case  ARINC429_INVALID_SSM:
		strcpy(ErrMsg, "Invalid SSM.");
		break;
	case  ARINC429_INVALID_COMMAND:
		strcpy(ErrMsg, "The command is not valid.");
		break;
	case  ARINC429_RX_UNDERFLOW:
		strcpy(ErrMsg, "There was no data in the Rx FIFO.");
		break;
	case ARINC429_TX_OVERFLOW:
		strcpy(ErrMsg, "Tx FIFO overflow.");
		break;
	default:
		sprintf(ErrMsg, "Unknown error 0x%08x.", errCode);
	}
	return ErrMsg;
}
