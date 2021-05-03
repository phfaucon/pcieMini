#include "LTC2984.h"

void LTC2984::spi_init(void)
{
	spi->setSpiControl(SpiOpenCore::SPI_CTRL_TX_NEGEDGE | 0x08);	// 8-bit transfers
	spi->setSpiDivider(0x05); 								// SPI f_max = 10 MHz.  62.5 MHz / ((5 + 1) * 2 ) = 5.2 MHz
	spi->selectSpiSlave(0); 								// de-assert the chip select
}

uint8_t LTC2984::transfer_byte(uint8_t ram_read_or_write, uint16_t start_address, uint8_t input_data)
{
	uint32_t txData, rxData;

	spi->setSpiControl(SpiOpenCore::SPI_CTRL_RX_NEGEDGE | SpiOpenCore::SPI_CTRL_TX_NEGEDGE | SpiOpenCore::SPI_CTRL_ASS | 0x20);	// 32-bit transfers
	spi->selectSpiSlave(1);
	txData = (ram_read_or_write<<24) | (start_address << 8) | input_data;
	rxData = spi->rw(txData);
	return rxData & 0xff;
}

uint8_t LTC2984::read_byte(uint16_t start_address)
{
	return transfer_byte(READ_FROM_RAM, start_address, 0);
}

uint8_t LTC2984::write_byte(uint16_t start_address, uint8_t input_data)
{
	return transfer_byte(WRITE_TO_RAM, start_address, input_data);
}

// *********************
// SPI RAM data transfer
// *********************
// To write to the RAM, set ram_read_or_write = WRITE_TO_RAM.
// To read from the RAM, set ram_read_or_write = READ_FROM_RAM.
// input_data is the data to send into the RAM. If you are reading from the part, set input_data = 0.
#if 0
uint32_t LTC2984::transfer_four_bytes(uint8_t ram_read_or_write, uint16_t start_address, uint32_t input_data)
{
	uint32_t mosi_word_h, mosi_word_l;
	uint32_t miso_word_h, miso_word_l;
	uint32_t returnData;
	mosi_word_h = (ram_read_or_write<<16) | start_address << 8;
	mosi_word_l = input_data;
	spi->setSpiControl(SpiOpenCore::SPI_CTRL_TX_NEGEDGE | SpiOpenCore::SPI_CTRL_ASS | 24);	// 32-bit transfers
	spi->setSpiTxData((ram_read_or_write<<16) | start_address << 8;);
	spi->setSpiTxData(1, mosi_word_h);
	spi->selectSpiSlave(1);

	spi->startTransfer();

	while(spi->isBusy()); // wait for no busy
	miso_word_h = spi->getSpiRxData(1);
	miso_word_l = spi->getSpiRxData(0);
	returnData = miso_word_l;
	spi->selectSpiSlave(0);
	return returnData;
}

#else
uint32_t LTC2984::transfer_four_bytes(uint8_t ram_read_or_write, uint16_t start_address, uint32_t input_data)
{
	uint32_t mosi_word_h, mosi_word_l;
	uint32_t miso_word_h, miso_word_l;
	uint32_t returnData;
	mosi_word_h = (ram_read_or_write<<16) | start_address;
	mosi_word_l = input_data;
	spi->setSpiControl(SpiOpenCore::SPI_CTRL_RX_NEGEDGE | SpiOpenCore::SPI_CTRL_TX_NEGEDGE | SpiOpenCore::SPI_CTRL_ASS | 0x20);	// 32-bit transfers
	spi->setTransferWidth(56);	// 56-bit = 7-byte transfers
	spi->setSpiTxData(0, mosi_word_l);
	spi->setSpiTxData(1, mosi_word_h);
	spi->selectSpiSlave(1);

	spi->startTransfer();

	while(spi->isBusy()); // wait for no busy
	miso_word_h = spi->getSpiRxData(1);
	miso_word_l = spi->getSpiRxData(0);
//	printf("received: word1= 0x%08x, word2=0x%08x\n", miso_word_h, miso_word_l);
	returnData = miso_word_l;
	spi->selectSpiSlave(0);
	return returnData;
}
#endif

uint32_t LTC2984::read_four_bytes(uint16_t start_address)
{
	return transfer_four_bytes(READ_FROM_RAM, start_address, 0);
}

uint32_t LTC2984::write_four_bytes(uint16_t start_address, uint32_t input_data)
{
	return transfer_four_bytes(WRITE_TO_RAM, start_address, input_data);
}

void LTC2984::write_custom_table(struct table_coeffs coefficients[64], uint16_t start_address, uint8_t table_length)
{
	int8_t i;
	uint32_t txData;

	spi->setSpiControl(SpiOpenCore::SPI_CTRL_RX_NEGEDGE | SpiOpenCore::SPI_CTRL_TX_NEGEDGE | SpiOpenCore::SPI_CTRL_ASS | 0x20);	// 32-bit transfers
	spi->setTransferWidth(24);	// 56-bit = 7-byte transfers
	txData = (WRITE_TO_RAM << 16) | start_address;
	spi->setSpiTxData(txData);

	// transmit the header
	spi->selectSpiSlave(1);
	spi->startTransfer();
	while(spi->isBusy()); // wait for no busy

	for (i=0; i< table_length; i++)
	{
		spi->setSpiTxData(coefficients[i].measurement);
		spi->startTransfer();
		while(spi->isBusy()); // wait for no busy

		spi->setSpiTxData(coefficients[i].temperature);
		spi->startTransfer();
		while(spi->isBusy()); // wait for no busy
	}
	spi->selectSpiSlave(0);
}


void LTC2984::write_custom_steinhart_hart(uint32_t steinhart_hart_coeffs[6], uint16_t start_address)
{
	int8_t i;
	uint32_t txData;

	spi->selectSpiSlave(1);

	spi->setSpiControl(SpiOpenCore::SPI_CTRL_RX_NEGEDGE | SpiOpenCore::SPI_CTRL_TX_NEGEDGE | SpiOpenCore::SPI_CTRL_ASS | 0x20);	// 32-bit transfers
	spi->setTransferWidth(24);	// 24-bit header
	txData = (WRITE_TO_RAM << 16) | start_address;
	spi->setSpiTxData(txData);

	// transmit the header
	spi->selectSpiSlave(1);
	spi->startTransfer();
	while(spi->isBusy()); 		// wait for no busy
	spi->setTransferWidth(32);	// 32-bit transfers

	for (i = 0; i < 6; i++)
	{
		spi->setSpiTxData(steinhart_hart_coeffs[i]);
		spi->startTransfer();
		while(spi->isBusy()); // wait for no busy
	}
	spi->selectSpiSlave(0xff);
}



