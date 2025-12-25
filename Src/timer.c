#include <stdio.h>

#include "stm32f4xx.h"

#include "timer.h"

#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "terminal.h"

volatile bool tim3_flag = false;

static uint8_t output_line_counter = 0;

void TIM3_IRQHandler(void)
{
        // if (TIM3->SR & TIM_SR_UIF)
        // {
        // clear UIF
        TIM3->SR &= ~TIM_SR_UIF;

        printf("Output voltage: %05.2f V", y[0][0]);

        if (++output_line_counter == 100)
        {
                output_line_counter = 0;
                terminal_clear();
        }
        else
        {
                terminal_insert_new_line();
        }
        // }
}

void tim3_init(uint32_t pwm_freq_hz)
{
        // Enable clock for TIM5.
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

        /*
         * PCLK1 = HCLK / 2 = 50 MHz (max allowed on STM32F411).
         * APB1 timer clock = 100 MHz (because APB1 prescaler = 2).
         * We want for timer 3 to run at APB1_TIM_CLK / 10000UL = 10 kHz.
         */
        uint32_t psc_clk = APB1_TIM_CLK / 10000UL;
        // Calculate the value of prescaler so APB1 timers run at APB1_TIM_CLK.
        uint32_t prescaler = (APB1_TIM_CLK / psc_clk) - 1;
        // Auto-reload register value.
        uint32_t arr = (psc_clk / pwm_freq_hz) - 1;

        // Set prescaler and auto-reload.
        TIM3->PSC = prescaler;
        TIM3->ARR = arr;

        // Enable ARR preload.
        TIM3->CR1 |= TIM_CR1_ARPE;

        // Generate an update first to load preload.
        TIM3->EGR |= TIM_EGR_UG;

        // Enable update interrupt
        TIM3->DIER |= TIM_DIER_UIE;

        // Set priority and disable TIM3 interrupt in NVIC for now.
        NVIC_SetPriority(TIM3_IRQn, 1);
        NVIC_DisableIRQ(TIM3_IRQn);

        // Enable counter.
        TIM3->CR1 |= TIM_CR1_CEN;
}