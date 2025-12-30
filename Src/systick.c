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
#include <stdio.h>

#include "stm32f4xx.h"

#include "systick.h"

#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "terminal.h"

// SysTick frequency in Hz
#define SYSTICK_FREQUENCY 1000UL

// systick_ticks is incremented SYSTICK_FREQUENCY times every second in SysTick_Handler ISR.
static volatile uint32_t systick_ticks      = 0;
// output_line_counter counts the printed lines in terminal, so we clean it after 100 lines.
static volatile uint8_t output_line_counter = 0;

static void systick_set_freq(uint32_t systick_clock, uint32_t systick_freq);

/*
 * SysTick interrupt is used to print the output voltage of the converter when stream is on.
 * Note: SysTick interrupt priority is the lowest on Nucleo F411RE by default. That is what we want
 * because printing the output voltage value is not as important as mode change by button handled by
 * timer 3 or updating the control loop and the converter state vector by timer 2.
 */
void SysTick_Handler(void)
{
        systick_ticks++;

        // Every 500ms, print the output voltage of the converter.
        if (systick_ticks == 500UL)
        {
                printf("Output voltage: %05.2f V", y[0][0]);
                systick_clear_ticks();

                if (++output_line_counter == 100)
                {
                        output_line_counter = 0;
                        terminal_clear();
                }
                else
                {
                        terminal_insert_new_line();
                }
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
        SysTick->VAL  = 0;

        // Enable SysTick timer.
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}