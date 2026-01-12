/*
 * systick.c
 *
 * Description:
 *     Configures and manages the Cortex-M4 SysTick timer.
 *     This file provides:
 *     - Initialization of SysTick
 *     - A 1 ms system tick interrupt
 *     - An increasing tick counter
 */
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>

#include "stm32f4xx.h"

#include "systick.h"

#include "cli.h"
#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "scheduler.h"
#include "terminal.h"
#include "timer.h"

// SysTick frequency in Hz
#define SYSTICK_FREQUENCY 1000UL

// systick_print_counter counts the printed lines in terminal, so we clean it after 100 prints.
uint16_t systick_print_counter = 0U;

// systick_ticks is incremented SYSTICK_FREQUENCY times every second in SysTick_Handler ISR.
static volatile uint32_t systick_ticks = 0UL;

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

        /*
         * Every 200ms, add systick_print_output function to the scheduler so that it print the
         * output voltage of the converter and reference value.
         */
        if (systick_ticks % 200UL == 0)
        {
                // Atomic modification of ready_flag_word to prevent race conditions.
                atomic_fetch_or(&ready_flag_word, TASK3);
        }
}

void systick_init(void)
{
        // Disable SysTick timer.
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        // Enable SysTick interrupt.
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

        // Set SysTick clock source to cpu clock (HCLK).
        SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

        // Set reload value.
        SysTick->LOAD = HCLK / SYSTICK_FREQUENCY - 1;

        /*
         * Clear current value so it starts counting from LOAD immediately (SysTick is a 24-bit
         * down-counter).
         */
        SysTick->VAL = 0U;

        // Enable SysTick timer.
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

uint32_t systick_get_ticks(void)
{
        return systick_ticks;
}

void systick_print_output(void)
{
        if (cli_stream_is_on)
        {
                converter_type_t converter_type = converter_get_type();

                printf("  Output Voltage: %6.2f V, ", y[0][0]);
                if (converter_type == DC_DC_IDEAL || converter_type == DC_DC_H_BRIDGE)
                {
                        printf("Reference Voltage: %6.2f V", pid_get_ref());
                }
                else
                {
                        printf("Reference Voltage: %6.2f V",
                               pid_get_ref() * sinf(converter_ref_phase));
                }
                terminal_insert_new_line();

                systick_print_counter++;
                if (systick_print_counter == 100U)
                {
                        systick_print_counter = 0U;
                        terminal_clear();
                }
        }
}