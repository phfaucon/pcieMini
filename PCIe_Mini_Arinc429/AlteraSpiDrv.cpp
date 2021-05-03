//#include <windows.h>
#include <stdio.h>
#include "AlphiDll.h"
#include "AlteraSpiDrv.h"
//#include "Hi3593Driver.h"
#include "altera_spi.h"

AlteraSpiDrv::AlteraSpiDrv(UINT32 *spiController, UINT8 cs)
{
	spiCtrlBase = spiController;
	spiCtrlDeviceNbr = cs;
}

void	AlteraSpiDrv::sendByteCommand(UINT8 value)
{
	spiCommand(
		1, &value,	// length and address of the write buffer
		0, NULL,					// length and address of the read buffer
		0);						// no flag
}

// ------------------------------------------------------------------
// Write SPI Command with a Value to HI-3110
// ------------------------------------------------------------------
void AlteraSpiDrv::setRegister(UINT8 cmd, UINT8 value){
	UINT8 buffer[] = { cmd, value };

	spiCommand(
		sizeof(buffer), buffer,	// length and address of the write buffer
		0, NULL,					// length and address of the read buffer
		0);						// no flag

}

/* ------------------------------------------------------------------
/  Read HI-3110 Register Read Function
/  ------------------------------------------------------------------
Argument(s):  Register to read
Return:  8-bit Register Value
*/
UINT8 AlteraSpiDrv::getRegister(UINT8 cmd){
	UINT8 w_buffer[] = { cmd };
	UINT8 r_buffer[2];

	spiCommand(
		1, w_buffer,	// length and address of the write buffer
		1, r_buffer,	// length and address of the read buffer
		0);			// no flag
	return r_buffer[0];

}

// ------------------------------------------------------
// MultiByteRead
// Inputs:  ReadCommand=SPI Command
//          count=number of bytes to read
//
// Output:  *passedArray = pointer to the array that gets loaded
// ------------------------------------------------------
int AlteraSpiDrv::MultiByteRead(UINT8 ReadCommand, UINT8 count, unsigned char *passedArray)
{
	UINT8 w_buffer[] = { ReadCommand };

	return spiCommand(
		1, 	w_buffer,			// write buffer
		count, 	passedArray,	// read buffer
		0);						// no flag
}

// ------------------------------------------------------
// MultiByteWrite
// Inputs:  ReadCommand=SPI Command
//          count=number of bytes to write
//			*passedArray = pointer to the array that contains the data
// ------------------------------------------------------
void AlteraSpiDrv::MultiByteWrite(UINT8 ReadCommand, UINT8 count, unsigned char *passedArray)
{
	UINT8 w_buffer[1000];
	w_buffer[0] = ReadCommand;
	for (int i=0; i<count; i++)
		w_buffer[i+1] = passedArray[i];

	spiCommand(
		count + 1, 	w_buffer,	// write buffer
		0, NULL,				// not reading anything
		0);						// no flag
}

/* This is a very simple routine which performs one SPI master transaction.
* It would be possible to implement a more efficient version using interrupts
* and sleeping threads but this is probably not worthwhile initially.
*/

int	AlteraSpiDrv::spiCommand(UINT32 write_length, const UINT8 * write_data,
		UINT32 read_length, UINT8 * read_data,
		UINT32 flags)
/*int alt_avalon_spi_command(UINT32 *base, UINT32 slave,
	UINT32 write_length, const UINT8 * write_data,
	UINT32 read_length, UINT8 * read_data,
	UINT32 flags)*/
{
	const UINT8 * write_end = write_data + write_length;
	UINT8 * read_end = read_data + read_length;

	UINT32 write_zeros = read_length;
	UINT32 read_ignore = write_length;
	UINT32 status;

	/* We must not send more than two bytes to the target before it has
	* returned any as otherwise it will overflow. */
	/* Unfortunately the hardware does not seem to work with credits > 1,
	* leave it at 1 for now. */
	INT32 credits = 1;

	/* Warning: this function is not currently safe if called in a multi-threaded
	* environment, something above must perform locking to make it safe if more
	* than one thread intends to use it.
	*/

	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(spiCtrlBase, 1 << spiCtrlDeviceNbr);

	/* Set the SSO bit (force chipselect) only if the toggle flag is not set */
	if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0) {
		IOWR_ALTERA_AVALON_SPI_CONTROL(spiCtrlBase, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	}

	/*
	* Discard any stale data present in the RXDATA register, in case
	* previous communication was interrupted and stale data was left
	* behind.
	*/
	IORD_ALTERA_AVALON_SPI_RXDATA(spiCtrlBase);

	/* Keep clocking until all the data has been processed. */
	for (;;)
	{

		do
		{
			status = IORD_ALTERA_AVALON_SPI_STATUS(spiCtrlBase);
		} while (((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0 || credits == 0) &&
			(status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);

		if ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) != 0 && credits > 0)
		{
			credits--;

			if (write_data < write_end)
				IOWR_ALTERA_AVALON_SPI_TXDATA(spiCtrlBase, *write_data++);
			else if (write_zeros > 0)
			{
				write_zeros--;
				IOWR_ALTERA_AVALON_SPI_TXDATA(spiCtrlBase, 0);
			}
			else
				credits = -1024;
		};

		if ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) != 0)
		{
			UINT32 rxdata = IORD_ALTERA_AVALON_SPI_RXDATA(spiCtrlBase);

			if (read_ignore > 0)
				read_ignore--;
			else
				*read_data++ = (UINT8)rxdata;
			credits++;

			if (read_ignore == 0 && read_data == read_end)
				break;
		}

	}

	/* Wait until the interface has finished transmitting */
	do
	{
		status = IORD_ALTERA_AVALON_SPI_STATUS(spiCtrlBase);
	} while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0);

	/* Clear SSO (release chipselect) unless the caller is going to
	* keep using this chip
	*/
	if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0)
		IOWR_ALTERA_AVALON_SPI_CONTROL(spiCtrlBase, 0);

	return read_length;
}
