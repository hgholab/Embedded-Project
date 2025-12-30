#include <stdio.h>

#include "stm32f4xx.h"

#include "timer.h"

#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "gpio.h"
#include "pwm.h"
#include "terminal.h"
#include "utils.h"

#define TIM2_CLOCK_FREQUENCY 10000UL
#define TIM3_CLOCK_FREQUENCY 10000UL
#define ABS(x)               (((x) > 0.0f) ? (x) : (-1.0f * (x)))

volatile bool tim3_flag = false;

void TIM2_IRQHandler(void)
{
        // Clear UIF.
        TIM2->SR &= ~TIM_SR_UIF;

        // Every 5ms, update the control loop and converter state variables.
        u[0][0] = pid_update(&pid, y[0][0]);
        converter_update(&plant, u, y);

        /*
         * Next, we change the brightness of the green LED based on the output of the controller.
         * First we normalize the controller output. We assume a maximum reference voltage of
         * REF_Max which is 50, and based on that the controller output is normalized.
         */
        // Calculate the duty cycle in percentage.
        float duty = 100.0f * CLAMP(ABS(u[0][0]) / REF_MAX, 0.0f, 1.0f);
        // Set timer 2 channel 1 pwm duty cycle.
        pwm_tim2_set_duty(duty);
}

void TIM3_IRQHandler(void)
{
        // Clear UIF.
        TIM3->SR &= ~TIM_SR_UIF;

        // Detect change in button status to register it as one button press.
        bool button_is_pressed = gpio_button_is_pressed();
        if (button_is_pressed && !button_last_push_status)
        {
                printf("Button is pushed!!\r\n");
        }
        button_last_push_status = button_is_pressed;
}

// TIM2 update event interrupt is used for updating the control loop and providing pwm for the LED.
void tim2_init(uint32_t timer_freq)
{
        // Enable clock for TIM2.
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

        /*
         * PCLK1 = HCLK / 2 = 50 MHz (max allowed on STM32F411).
         * APB1 timer clock = 100 MHz (because APB1 prescaler = 2).
         * We want for timer 2 to run at frequency of TIM2_CLOCK_FREQUENCY = 10 kHz.
         */
        uint32_t tim2_clock           = TIM2_CLOCK_FREQUENCY;
        // Calculate the value of prescaler so timer 2 runs at 10 kHz.
        uint32_t prescaler            = (APB1_TIM_CLK / tim2_clock) - 1;
        // Auto-reload register value.
        uint32_t auto_reload_register = (tim2_clock / timer_freq) - 1;

        // Set prescaler and auto-reload.
        TIM2->PSC                     = prescaler;
        TIM2->ARR                     = auto_reload_register;

        // Enable ARR preload (prescaler (PSC) is always buffered).
        TIM2->CR1 |= TIM_CR1_ARPE;

        // Generate an update first to load preloaded value.
        TIM2->EGR |= TIM_EGR_UG;

        // Enable update event interrupt.
        TIM2->DIER |= TIM_DIER_UIE;

        // Clear pending flags.
        TIM2->SR = 0;

        // Clear pending interrupt.
        NVIC_ClearPendingIRQ(TIM2_IRQn);
        // Set priority and disable TIM2 interrupt in NVIC for now.
        NVIC_SetPriority(TIM2_IRQn, 0);
        NVIC_DisableIRQ(TIM2_IRQn);

        // Enable counter.
        TIM2->CR1 |= TIM_CR1_CEN;
}

// TIM3 update event interrupt is used for button debounce.
void tim3_init(uint32_t timer_freq)
{
        // Enable clock for TIM3.
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

        /*
         * PCLK1 = HCLK / 2 = 50 MHz (max allowed on STM32F411).
         * APB1 timer clock = 100 MHz (because APB1 prescaler = 2).
         * We want for timer 3 to run at frequency of TIM3_CLOCK_FREQUENCY = 10 kHz.
         */
        uint32_t tim3_clock           = TIM3_CLOCK_FREQUENCY;
        // Calculate the value of prescaler so timer 3 runs at 10 kHz.
        uint32_t prescaler            = (APB1_TIM_CLK / tim3_clock) - 1;
        // Auto-reload register value.
        uint32_t auto_reload_register = (tim3_clock / timer_freq) - 1;

        // Set prescaler and auto-reload.
        TIM3->PSC                     = prescaler;
        TIM3->ARR                     = auto_reload_register;

        // Enable ARR preload (prescaler (PSC) is always buffered).
        TIM3->CR1 |= TIM_CR1_ARPE;

        // Generate an update first to load preloaded value.
        TIM3->EGR |= TIM_EGR_UG;

        // Enable update event interrupt.
        TIM3->DIER |= TIM_DIER_UIE;

        // Clear pending flags.
        TIM3->SR = 0;

        // Clear pending interrupt.
        NVIC_ClearPendingIRQ(TIM3_IRQn);
        // Set priority and enable TIM3 interrupt in NVIC.
        NVIC_SetPriority(TIM3_IRQn, 2);
        NVIC_EnableIRQ(TIM3_IRQn);

        // Enable counter.
        TIM3->CR1 |= TIM_CR1_CEN;
}