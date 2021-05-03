// Copyright (c) 2020 Alphi Technology Corporation, Inc.  All Rights Reserved
//
// You are hereby granted a copyright license to use, modify and
// distribute this SOFTWARE so long as the entire notice is retained
// without alteration in any modified and/or redistributed versions,
// and that such modified versions are clearly identified as such.
// No licenses are granted by implication, estopple or otherwise under
// any patents or trademarks of Alphi Technology Corporation (Alphi).
//
// The SOFTWARE is provided on an "AS IS" basis and without warranty,
// to the maximum extent permitted by applicable law.
//
// ALPHI DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED, INCLUDING
// WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
// AND ANY WARRANTY AGAINST INFRINGEMENT WITH REGARD TO THE SOFTWARE
// (INCLUDING ANY MODIFIED VERSIONS THEREOF) AND ANY ACCOMPANYING
// WRITTEN MATERIAL.
//
// To the maximum extent permitted by applicable law, IN NO EVENT SHALL
// ALPHI BE LIABLE FOR ANY DAMAGE WHATSOEVER (INCLUDING WITHOUT LIMITATION,
// DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
// BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM THE USE
// OR INABILITY TO USE THE SOFTWARE.  GMS assumes no responsibility for
// for the maintenance or support of the SOFTWARE
//
/** @file Ltc2664_SPI.cpp
 * @brief D/A controller SPI class implementation
 */
#include "Ltc2664_SPI.h"

/** @brief Set an output value to one or several D/A channels
* @param code Value to convert to analog out.
* @param channel Channel number or all the channels.
* @param update Which output to update after updating the value. Default is CHANNEL_NONE.
* @param echo Determine if we are sending a 32-bit command (echo is true) so that we can get an echo. Default is false, a 24-bit command, and no echo.
* @retval ERRCODE_NO_ERROR if successful, ERRCODE_INVALID_VALUE if the parameters are incompatible.
*/
PCIeMini_status Ltc2664_SPI::setCode(int16_t code, ChannelNbr channel, ChannelNbr update, bool echo)
{
	PCIeMini_status ret = ERRCODE_NO_ERROR;

	reset();

	if (!isChannelValid(update) || !isChannelValid(channel) || channel == CHANNEL_NONE)
		return ERRCODE_INVALID_VALUE;

	int idx = echo ? 1 : 0;		// skip the first byte if we want the echo

	uint8_t command;
	if (channel == CHANNEL_ALL) {
		if (update == CHANNEL_NONE)
			command = CODE_ALL;
		else if (update == CHANNEL_ALL)
			command = CODE_ALL_UPDATE_ALL;
		else {
			command = CODE_ALL;
			ret = ERRCODE_INVALID_VALUE;
		}
	}
	else {
		if (update == CHANNEL_NONE)
			command = CODE + channel;
		else if (update == channel)
			command = CODE_UPDATE + channel;
		else if (update == CHANNEL_ALL)
			command = CODE_UPDATE_ALL + channel;
		else {
			command = CODE + channel;
			ret = ERRCODE_INVALID_VALUE;
		}
	}

//	buff_out[idx++] = command;
//	buff_out[idx++] = code >> 8;
//	buff_out[idx++] = code & 0xff;
	buff_out[0] = (command << 16) | code;
	sendSpiCommand(0,
		1, buff_out,
		0, buff_in,
		0);

	return ret;
}
