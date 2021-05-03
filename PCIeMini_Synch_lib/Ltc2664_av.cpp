//
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
/** @file Ltc2664_av.cpp
 * @brief Synchro simulator class implementation.
 */

// Maintenance Log
//---------------------------------------------------------------------
// v1.0		7/23/2020	phf	Written
//---------------------------------------------------------------------

#include "Ltc2664_av.h"

/** @brief Constructor
 *
 * To be called only by the board open.
 * @param addr Base address of the board registers in user space.
 */
Ltc2664_av::Ltc2664_av(volatile void* addr)
{
    velocityReg = &((volatile uint32_t*)addr)[velocity_index];
    gain = (volatile int16_t*)addr;
}

/** @brief configure the sine amplitudes for a given angle
* 
* For the synchro resolver, the angle is expressed as the amplitude of 3 sine waves.<br>
* This function, if given an angle calculates the gains of the 3 DA channels and thus the amplitudes
* of the sine waves generated.
* @param angle A double between 0 and PI.
*/ 
void Ltc2664_av::setAngle(double angle)
{
    double s1_angle, s2_angle, s3_angle;
    double s1_sin, s2_sin, s3_sin;

    s1_angle = 360 - (angle + 30);
    s2_angle = s1_angle + 120;
    s3_angle = s1_angle + 240;

    s1_angle = s1_angle * (M_PI / 180);
    s2_angle = s2_angle * (M_PI / 180);
    s3_angle = s3_angle * (M_PI / 180);

    s1_sin = sin(s1_angle);
    s2_sin = sin(s2_angle);
    s3_sin = sin(s3_angle);

    //	printf("s1_sin %f -- s2_sin %f -- s3_sin %f\n", s1_sin, s2_sin, s3_sin);

    s1_sin = s1_sin * 32767;
    s2_sin = s2_sin * 32767;
    s3_sin = s3_sin * 32767;

    //	printf("s1_sin %f -- s2_sin %f -- s3_sin %f\n", s1_sin, s2_sin, s3_sin);

    setSimGain(0, s1_sin);
    setSimGain(1, s2_sin);
    setSimGain(2, s3_sin);

    //	printf("S1 %x -- S2 %x -- S3 %x\n", *S1, *S2, *S3);

}

/** @brief fixed point angle to double
 *
 * This function converts a 17-bit angle value as a radian value as double.
 * @param angle 17-bit fixed point angle.
 * @retval Double value equivalent
 */
double Ltc2664_av::angleToRadian(uint32_t angle)
{
    if (angle & 0x10000) angle |= 0xffff0000;
    double ret = (int)angle;
    return ret / 0x4000;
}

/** @brief radian angle to fixed point
 *
 * This function converts a radian value as double as a 17-bit angle value.
 * @param rad A double between -PI and +PI .
 * @retval 17-bit fixed point angle
 */
uint32_t Ltc2664_av::radianToAngle(double rad)
{
    return (uint32_t)(int)(rad * 0x4000);
}

/* @brief Get the simulator sinewave frequency
 * @return The frequency in Hertz.
 */
double Ltc2664_av::getFrequency(void)
{
    double freq = getAngularVelocity() / angVel2Freq();

    return freq;
}

/* @brief set the simulator sinewave frequency
 * @param freq The frequency in Hertz.
 */
void Ltc2664_av::setFrequency(double freq)
{
    double angularVel = freq * angVel2Freq();
    setAngularVelocity(angularVel);
}

/** @brief Set an individual gain register
 *
 * @param chanNbr Gain number register (0 to 2). If the number is invalid, the function does nothing.
 * @param val 16-bit value to put in the gain register.
 */
void Ltc2664_av::setSimGain(int chanNbr, int16_t val)
{
	switch (chanNbr) {
	case 0: gain[gain1_index] = val;
		break;
	case 1: gain[gain2_index] = val;
		break;
	case 2: gain[gain3_index] = val;
		break;
	default:
		break;
	}
}

/** @brief Get an individual gain register value
 *
 * @param chanNbr Gain number register (0 to 2). If the number is invalid, the function returns 0.
 * @retval 16-bit signed integer value
 */
int16_t Ltc2664_av::getSimGain(int chanNbr)
{
	switch (chanNbr) {
	case 0: return gain[gain1_index];
		break;
	case 1: return gain[gain2_index];
		break;
	case 2: return gain[gain3_index];
		break;
	default:
		return 0;
	}
}

