#include "CanFdTest.h"

int CanFdTest::testPCIeToBrdDma(TransferDesc* tfrDesc)
{
	uint32_t* pBuf;					///< buffer allocated by the open in user space
	uint32_t tfrLengthWord = 256;
	uint32_t localAddress;			///< address of the FPGA object, also the offset in BAR 2
	int dataVerificationErrors = 0;
	printf("\nTesting DMA - Host to device: ");

	localAddress = dut->dpr_offset; // DPR

	pBuf = tfrDesc->userSpaceBuffer;
	//	if (pBuf == tfrDesc.userSpaceBuffer) printf("Addresses match!\n");
		// Initialize source memory
	for (uint32_t i = 0; i < tfrLengthWord; i++) ((uint32_t*)pBuf)[i] = 11000 + i;
	// initialize the DPR content
	for (int i = 0; i < dut->dpr_length / sizeof(uint32_t); i++)
	{
		dut->dpr[i] = i;
	}

	QueryPerformanceCounter(&StartingTime);

	// do the DMA transfer
	tfrDesc->src_offset = tfrDesc->txs_offset;
	tfrDesc->dest_offset = localAddress;
	tfrDesc->tfr_length = tfrLengthWord * sizeof(uint32_t);
	dut->DMATransfer(tfrDesc, true);		// program the DMA controller and purge the caches

	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	int delay = (int)ElapsedMicroseconds.QuadPart;
	double bandwidth = delay;
	bandwidth = ((double)tfrLengthWord * 160) / bandwidth;

	printf("write: %d bytes in %d microseconds (%f MB/s)", tfrLengthWord * 160, delay, bandwidth);

	dut->dma->print("");
	printf("Verifying transfer success\n");
	for (uint32_t i = 0; i < tfrLengthWord; i++) {
		uint32_t src = ((uint32_t*)pBuf)[i];
		uint32_t dest = dut->dpr[i];
		if (src != dest) {
			printf("Error at word#%d, wrote %d, read %d\n", i, src, dest);
			dataVerificationErrors++;
		}
	}
	if (dataVerificationErrors == 0) {
		printf("Success!\n");
	}
	else {
		printf("%d errors\n", dataVerificationErrors);
	}

	return dataVerificationErrors;
}

int CanFdTest::testBrdToPCIeDma(TransferDesc* tfrDesc)
{
	uint32_t* pBuf;					///< buffer allocated by the open in user space
	uint32_t tfrLengthWord = 256;
	uint32_t localAddress;			///< address of the FPGA object, also the offset in BAR 2
	int dataVerificationErrors = 0;
	printf("\nTesting DMA - Device to Host: ");

	localAddress = dut->dpr_offset; // DPR

	pBuf = tfrDesc->userSpaceBuffer;
	//	if (pBuf == tfrDesc.userSpaceBuffer) printf("Addresses match!\n");
		// Initialize dest memory
	for (uint32_t i = 0; i < tfrLengthWord; i++) ((uint32_t*)pBuf)[i] = i;
	// initialize the DPR content
	for (int i = 0; i < dut->dpr_length / sizeof(uint32_t); i++)
	{
		dut->dpr[i] = 11000 + i;
	}

	QueryPerformanceCounter(&StartingTime);

	// do the DMA transfer
	tfrDesc->src_offset = localAddress;
	tfrDesc->dest_offset = tfrDesc->txs_offset;
	tfrDesc->tfr_length = tfrLengthWord * sizeof(uint32_t);
	dut->DMATransfer(tfrDesc, true);		// program the DMA controller and purge the caches

	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
	int delay = (int)ElapsedMicroseconds.QuadPart;
	double bandwidth = delay;
	bandwidth = ((double)tfrLengthWord * 160) / bandwidth;

	printf("write: %d bytes in %d microseconds (%f MB/s)", tfrLengthWord * 160, delay, bandwidth);

	dut->dma->print("");
	printf("Verifying transfer success\n");
	for (uint32_t i = 0; i < tfrLengthWord; i++) {
		uint32_t dest = ((uint32_t*)pBuf)[i];
		uint32_t src = dut->dpr[i];
		if (src != dest) {
			printf("Error at word#%d, wrote %d, read %d\n", i, src, dest);
			dataVerificationErrors++;
		}
	}
	if (dataVerificationErrors == 0) {
		printf("Success!\n");
	}
	else {
		printf("%d errors\n", dataVerificationErrors);
	}

	return dataVerificationErrors;
}

int CanFdTest::testPCIeDma()
{
	DWORD dwDMABufSize = 16384;		///< size of the buffer to be allocated
	TransferDesc tfrDesc;
	bool fPolling = true;
	uint32_t localAddress;			///< address of the FPGA object, also the offset in BAR 2

	localAddress = dut->dpr_offset; // DPR

	int errNbr = 0;
	PCIeMini_status st = dut->DMAOpen(localAddress, dwDMABufSize, true, &tfrDesc);
	if (st != ERRCODE_NO_ERROR) {
		printf("%s\n", getAlphiErrorMsg(st));
		return 1;
	}

	errNbr += testPCIeToBrdDma(&tfrDesc);
	errNbr += testBrdToPCIeDma(&tfrDesc);

	// close the memory space
	dut->DMAClose(false);
	return errNbr;
}

int CanFdTest::testLocalBlockDma(uint32_t tfrLengthWord)
{
	int errNbr = 0;
	printf("Testing Block DMA: ");
	uint32_t localSource = dut->dpr_offset;
	uint32_t localDest = dut->dpr_offset + dut->dpr_length / 2;
	volatile uint32_t* src = dut->dpr;
	volatile uint32_t* dest = &dut->dpr[dut->dpr_length / 8];

	dut->dma->reset();

	// initialize source + destination
	for (uint32_t i = 0; i < tfrLengthWord; i++) {
		src[i] = i;
		dest[i] = i + 10000;
	}

	// verify source + destination
	for (uint32_t i = 0; i < tfrLengthWord; i++) {
		if (src[i] != i)
		{
			if (errNbr < 5) {
				printf("Error writing the DPR at index %d: wrote %d, read %d\n",
					i, i, src[i]);
				errNbr++;
			}
		}
		if (dest[i] != i + 10000)
		{
			if (errNbr < 5) {
				printf("Error writing the DPR at index %d: wrote %d, read %d\n",
					i, i + 10000, dest[i]);
				errNbr++;
			}
		}
	}

	// start DMA
	TransferDesc desc;

	desc.tfr_length = tfrLengthWord * sizeof(uint32_t);
	desc.src_offset = localSource;
	desc.dest_offset = localDest;

	dut->dma->print("After reset");
	dut->dma->launch_bidir(&desc);
	dut->dma->print("After launch");
	Sleep(100);
	printf("After delay %d bytes left out of %lld",
		dut->dma->getLength(), tfrLengthWord * sizeof(uint32_t));
	dut->dma->print("");
	// verify
	// verify source + destination
	for (uint32_t i = 0; i < tfrLengthWord; i++)
		if (src[i] != i)
		{
			if (errNbr < 5) {
				printf("Error source changed at address %d: wrote %d, read %d\n",
					i, i, src[i]);
				errNbr++;
			}
		}

	for (uint32_t i = 0; i < tfrLengthWord; i++)
		if (dest[i] != i)
		{
			if (errNbr < 10) {
				printf("Error destination at address %d: wrote %d, read %d\n",
					i, i, dest[i]);
				errNbr++;
			}
		}

	if (errNbr == 0) {
		printf("OK\n");
	}
	else {
		printf("Failed\n");
	}
	return errNbr;
}
