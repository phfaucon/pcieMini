#include "PCIeMini_Arinc429.h"
#include <stdio.h>
/* command mailbox 
*/
#define SPI_IDLE 			0x00
#define SPI_BRD_RESET		0x01
#define SPI_WRITE_1 		0x10
#define SPI_WRITE_0 		0x11
#define SPI_READ_1	 		0x12
#define SPI_STORE_LBL_0 	0x13
#define SPI_STORE_LBL_1 	0x14
#define SPI_WRITE_LBL_0 	0x15
#define SPI_WRITE_LBL_1 	0x16
#define SPI_READ_LBL_0		0x17
#define SPI_READ_LBL_1		0x18
#define SPI_WRITE_PRI_LBL	0x19
#define SPI_READ_PRI_LBL	0x1a
#define SPI_READ_RXFIFO_LVL	0x1b
#define SPI_READ_TXFIFO_LVL	0x1c
#define SPI_READ_RXFIFO_SZ	0x1d
#define SPI_READ_TXFIFO_SZ	0x1e

Arinc429version::Arinc429version(UINT32 * addr)
{
	address = addr;
}

UINT32 Arinc429version::getVersion()		//!<  Version, if there is one programmed on the board hardware. Typically 0.
{
	return address[0];
}

UINT32 Arinc429version::getTimeStamp()		//!<  Date when the board firmware was compiled.
{
	return address[1];
}

PCIeMini_Arinc429::PCIeMini_Arinc429() :
	AlphiBoard(0x13c5, 0x508)
{
	isOpen = false;
	for (int i = 0; i<nbrOfArincControllers; i++) {
		ArincController[i] = NULL;
		tx[i] = NULL;
	}
	for (int i = 0; i<nbrOfArincControllers*2; i++) {
		rx[i] = NULL;
	}

	// initialize all the local pointers to NULL until the board is opened
	uicr = NULL;
	sysid = NULL;
	spiController = NULL;
	dpr = NULL;
	interruptEnabled = 0;
	ledPio = NULL;

}

ARINC_status PCIeMini_Arinc429::open(int brdNbr)
{
	if (brdNbr < 0 || brdNbr>9)
		return ERRCODE_INVALID_BOARD_NUM;
	HRESULT result = AlphiBoard::Open(brdNbr);
	if (result != WD_STATUS_SUCCESS) {
		return result;
	}

	if (verbose) {
		std::cout << "bar #0 address = 0x" << std::hex << bar0.Address << std::endl;
		std::cout << "bar #2 address = 0x" << std::hex << bar2.Address << std::endl;
		std::cout << "bar #3 address = 0x" << std::hex << bar3.Address << std::endl;
	}


	// create the device objects
	ledPio = (UINT32 *)getBar2Address(led_pio_offset);
	dpr = (UINT32 *)getBar2Address(dpr_offset);
	cmdMb = new CommandMb(dpr + 0x30);

//		ArincController[0] = new Hi3593direct(devAddressBar2(hi3593_offset), 0);
	ArincController[0] = new Hi3593(0, getBar2Address(0), getBar2Address(dpr_offset), cmdMb);
	gpio[0] = new Hi3593Status(getBar2Address(gpio_0_offset));
	rx[0] = new PCIeMini_Arinc429_Rx(ArincController[0], gpio[0], 0);
	rx[1] = new PCIeMini_Arinc429_Rx(ArincController[0], gpio[0], 1);
	tx[0] = new PCIeMini_Arinc429_Tx(ArincController[0]);

	if (nbrOfArincControllers>1) {
		ArincController[1] = new Hi3593(1, getBar2Address(dpr_offset), getBar2Address(dpr_offset), cmdMb);
		gpio[1] = new Hi3593Status(getBar2Address(gpio_1_offset));
		rx[2] = new PCIeMini_Arinc429_Rx(ArincController[1], gpio[1], 0);
		rx[3] = new PCIeMini_Arinc429_Rx(ArincController[1], gpio[1], 1);
		tx[1] = new PCIeMini_Arinc429_Tx(ArincController[1]);
	}
	if (nbrOfArincControllers>2) {
		ArincController[2] = new Hi3593(2, getBar2Address(dpr_offset), getBar2Address(dpr_offset), cmdMb);
		gpio[2] = new Hi3593Status(getBar2Address(gpio_2_offset));
		rx[4] = new PCIeMini_Arinc429_Rx(ArincController[2], gpio[2], 0);
		rx[5] = new PCIeMini_Arinc429_Rx(ArincController[2], gpio[2], 1);
		tx[2] = new PCIeMini_Arinc429_Tx(ArincController[2]);
	}
	if (nbrOfArincControllers>3) {
		ArincController[3] = new Hi3593(3, getBar2Address(dpr_offset), getBar2Address(dpr_offset), cmdMb);
		gpio[3] = new Hi3593Status(getBar2Address(gpio_3_offset));
		rx[6] = new PCIeMini_Arinc429_Rx(ArincController[3], gpio[3], 0);
		rx[7] = new PCIeMini_Arinc429_Rx(ArincController[3], gpio[3], 1);
		tx[3] = new PCIeMini_Arinc429_Tx(ArincController[3]);
	}

	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429::close()
{
	isOpen = false;
	for (int i = 0; i<nbrOfArincControllers; i++) {
		delete ArincController[i];
		ArincController[i] = NULL;
		delete tx[i];
		delete gpio[i];
		tx[i] = NULL;
	}
	for (int i = 0; i<nbrOfArincControllers*2; i++) {
		delete rx[i];
		rx[i] = NULL;
	}
	AlphiBoard::Close();

	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429::reset()
{
	int i;
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_BRD_RESET, 0, 0, 0, 0));	// for all the controllers
	for (i=0; i<nbrOfArincControllers; i++) {
//			printf ("resetting 3593#%d\n", i);
		ArincController[i]->sendCommand(0x04, NULL, NULL);		// master reset
	}
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429::stop()
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_IDLE, 0, 0, 0, 0));
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429::start()
{
	cmdMb->commandMbSendCommand(CommandMb::composeCommandWord(SPI_READ_RXFIFO_SZ, 0, 0, 0, 0));
	return ARINC429_NO_ERROR;
}

ARINC_status PCIeMini_Arinc429::getDriverVersion(Arinc429version * version)
{
	return ARINC429_NO_ERROR;
}

UINT32 PCIeMini_Arinc429::getGpioInputValues(int gpioNumber)
{
	return gpio[gpioNumber]->getData();
}

void PCIeMini_Arinc429::setLedPio(UINT32 val)
{
	ledPio[0] = val;
}

UINT32 PCIeMini_Arinc429::getLedPio()
{
	return ledPio[0];
}


