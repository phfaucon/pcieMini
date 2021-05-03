#include "Arinc3717Test.h"

void Arinc3717Test::BoardTest(void) 
{
	uint32_t v = sysid->getVersion();		//!<  Version, if there is one programmed on the board hardware. Typically 0.
	time_t ts = sysid->getTimeStamp();		//!<  Date when the board firmware was compiled.

}

void Arinc3717Test::PeriphInit(void) {}
void Arinc3717Test::Timer_Init(void) {}
void Arinc3717Test::InitInterrupts(void) {}
void Arinc3717Test::EnableInterrupts(void) {}
void Arinc3717Test::DisableInterrupts(void) {}
