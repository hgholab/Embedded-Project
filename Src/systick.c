/*
 * systick.c
 *
 * Description:
 *     Configures and manages the Cortex-M4 SysTick timer.
 *     This file provides:
 *     - Initialization of SysTick with selectable clock source
 *     - A 1 ms system tick interrupt
 *     - An increasing tick counter
 *     - A blocking millisecond delay function
 *
 *     SysTick can be configured to use:
 *     - Processor clock (HCLK)
 *     - Processor clock divided by 8 (HCLK / 8)
 *
 *     The SysTick interrupt increments a tick counter at a
 *     fixed frequency, which can be used for timing and delays.
 */

#include "stm32f4xx.h"

#include "systick.h"

#include "clock.h"
#include "controller.h"
#include "converter.h"

// SysTick frequency in Hz
#define SYSTICK_FREQUENCY 1000UL

static volatile uint32_t systick_ticks = 0;

static void systick_set_freq(uint32_t systick_clock, uint32_t systick_freq);

void SysTick_Handler(void)
{
        systick_ticks++;

        // Every 5ms, update the control loop and converter state variables.
        if (systick_ticks == 5UL)
        {
                u[0][0] = pid_update(&pid, y[0][0]);
                converter_update(&plant, u, y);
                systick_clear_ticks();
        }
}

void systick_init(Systick_clock_source_t systick_clock_source, bool enable_interrupt)
{
        uint32_t systick_clock;

        // First, disable SysTick timer before configuration.
        SysTick->CTRL = 0UL;

        // Choose SysTick interrupt behavior.
        if (enable_interrupt)
                SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

        // Set SysTick clock source.
        if (systick_clock_source == SYSTICK_EXTERNAL_CLOCK)
        {
                SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;
                // External clock here means HCLK / 8;
                systick_clock = HCLK / 8;
        }
        else
        {
                SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
                systick_clock = HCLK;
        }

        // This function sets the frequency and enables the SysTick timer.
        systick_set_freq(systick_clock, SYSTICK_FREQUENCY);
}

void systick_disable_interrupt(void)
{
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

void systick_enable_interrupt(void)
{
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

void systick_delay_ms(uint32_t delay_ms)
{
        uint32_t start = systick_get_ticks();
        uint32_t ticks = delay_ms * (SYSTICK_FREQUENCY / 1000UL);

        while ((systick_get_ticks() - start) < ticks)
                ;
}

uint32_t systick_get_ticks(void)
{
        return systick_ticks;
}

void systick_clear_ticks(void)
{
        systick_ticks = 0;
}

static void systick_set_freq(uint32_t systick_clock, uint32_t systick_freq)
{
        // Disable SysTick timer.
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        // Set reload value.
        SysTick->LOAD = systick_clock / systick_freq - 1;

        // Clear current value so it starts counting from LOAD immediately.
        SysTick->VAL = 0;

        // Enable SysTick timer.
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}