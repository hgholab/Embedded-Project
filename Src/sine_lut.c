/*
 * sine_lut.c
 *
 * Description:
 *     Generates and initializes a sine wave look-up table (LUT).
 *
 *     This module:
 *     - Defines a fixed-size sine look-up table with LUT_SIZE samples
 *     - Stores the values as single-precision floating-point numbers
 *
 * Notes:
 *     - The LUT is initialized at runtime by calling sine_lut_init().
 *     - The sine values are computed using sinf() for single-precision math.
 *     - The table represents one full cycle with uniform angular spacing.
 */

#include <math.h>
#include <stddef.h>

#include "sine_lut.h"

#define LUT_SIZE 256
#define PI       3.141592f

float sine_lut[LUT_SIZE];

// This function initializes a sine function look-up table which is used as a sinusoidal reference.
void sine_lut_init(void)
{
        for (size_t i = 0; i < LUT_SIZE; i++)
        {
                sine_lut[i] = sinf((2.0f * PI * i) / LUT_SIZE);
        }
}