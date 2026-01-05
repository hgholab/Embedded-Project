/*
 * fpu.c
 *
 * Description:
 *    Enable full access to the Floating Point Unit (FPU).
 *
 *    Enables full access to coprocessors CP10 and CP11 by
 *    configuring the CPACR register. A data and instruction
 *    synchronization barrier are issued to ensure the configuration
 *    takes effect before any floating-point instructions are executed.
 */

#include "stm32f4xx.h"

#include "fpu.h"

void fpu_enable(void)
{
        SCB->CPACR |= (0xF << 20);
        __DSB();
        __ISB();
}