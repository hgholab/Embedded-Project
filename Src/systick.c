/*
 * systick.c
 *
 * Description:
 *     Configures and manages the Cortex-M4 SysTick timer.
 *     This file provides:
 *     - Initialization of SysTick with selectable clock source
 *     - A 1 ms system tick interrupt
 *     - An increasing tick counter
 *
 *     SysTick can be configured to use:
 *     - Processor clock (HCLK)
 *     - Processor clock divided by 8 (HCLK / 8)
 */
#include <stdio.h>

#include "stm32f4xx.h"

#include "systick.h"

#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "scheduler.h"
#include "terminal.h"

// SysTick frequency in Hz
#define SYSTICK_FREQUENCY 1000UL

// systick_ticks is incremented SYSTICK_FREQUENCY times every second in SysTick_Handler ISR.
static volatile uint32_t systick_ticks = 0UL;
// print_counter counts the printed lines in terminal, so we clean it after 100 prints.
static volatile uint16_t print_counter = 0U;

static void systick_set_freq(uint32_t systick_clock, uint32_t systick_freq);

/*
 * SysTick interrupt is used to print the output voltage of the converter when stream is on.
 *
 * Note: SysTick interrupt priority is the lowest on Nucleo F411RE by default. That is what
 * we want because printing the output voltage value is not as important as mode change by
 * button handled by TIM3 or updating the control loop and the converter state vector by
 * TIM2.
 */
void SysTick_Handler(void)
{
        systick_ticks++;
        ready_flag_word |= TASK3;
}

void systick_init(systick_clock_source_t systick_clock_source, bool enable_interrupt)
{
        uint32_t systick_clock;

        // Disable SysTick timer before configuration.
        SysTick->CTRL = 0U;

        // Choose SysTick interrupt behavior.
        if (enable_interrupt)
        {
                SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
        }
        else
        {
                SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
        }

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

void systick_print_output(void)
{
        // Every 500ms, print the output voltage of the converter and reference value.
        if (systick_ticks == 500UL)
        {
                printf("Output Voltage: %05.2f V", y[0][0]);
                terminal_insert_new_line();
                printf("Reference Voltage: %05.2f V", pid_get_ref(&pid));
                terminal_insert_new_line();

                print_counter++;
                if (print_counter == 100U)
                {
                        print_counter = 0U;
                        terminal_clear();
                }
        }
}

// This function sets the frequency and enables the SysTick timer.
static void systick_set_freq(uint32_t systick_clock, uint32_t systick_freq)
{
        // Disable SysTick timer.
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        // Set reload value.
        SysTick->LOAD = systick_clock / systick_freq - 1;

        // Clear current value so it starts counting from LOAD immediately.
        SysTick->VAL = 0U;

        // Enable SysTick timer.
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}