#pragma once
#include "PCIe_mini_ARINC717_IO.h"

class Arinc3717Test : public PCIe_mini_ARINC717_IO
{
public:
	inline Arinc3717Test(uint8_t brdNbr) 
	{
		boardNbr = brdNbr;
	}

	void BoardTest(void);
	void PeriphInit(void);
	void Timer_Init(void);
	void InitInterrupts(void);
	void EnableInterrupts(void);
	void DisableInterrupts(void);

	void SerialCommands(void);

	void Transmit_SPECIAL_Mode(void);
	void Transmit_Receive_Mode(const uint8_t SELFTEST);

	void TxWord(unsigned int ArincWord);
	void GoodSubFrame0(unsigned int sync);
	void GoodSubFrame1(unsigned int sync);
	void GoodSubFrameC(unsigned int sync);
	void GoodFrame0(void);
	void GoodFrame1(void);
	void GoodFrameC(void);

	void TransmittAllZeros();
	void TransmitAll1();
	void TransmitFrameSpecial();
	void SftSync_Mode_Demo(const uint8_t SELFTEST);
	void Receive_Mode(const uint8_t compare);

	void Transmitter2(void);

	void DisplayStatusRegisters(void);
	void displayOneDataWord(unsigned int ArincWord);
	void getRegStatus(void);
	unsigned char Data717Compare(uint16_t* data);
	void print3717DataWord(unsigned int index, uint16_t* array);
	void print3717Data(unsigned int index, unsigned char* array);

private:
	uint8_t boardNbr;
};