#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "stm32f4xx.h"

#include "timer.h"

#include "cli.h"
#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "gpio.h"
#include "pwm.h"
#include "scheduler.h"
#include "utils.h"

#define TIM2_CLK 10000UL // TIM2 clock frequency
#define TIM3_CLK 10000UL // TIM3 clock frequency

// TIM2 update event interrupt is used for updating the control loop and providing PWM for the LED.
void TIM2_IRQHandler(void)
{
        // Clear UIF flag.
        TIM2->SR &= ~TIM_SR_UIF;

        // Atomic modification of ready_flag_word to prevent race conditions.
        atomic_fetch_or(&ready_flag_word, TASK0);
}

// TIM3 update event interrupt is used for button debounce and command handling.
void TIM3_IRQHandler(void)
{
        // Clear UIF flag.
        TIM3->SR &= ~TIM_SR_UIF;

        // Atomic modification of ready_flag_word to prevent race conditions.
        atomic_fetch_or(&ready_flag_word, TASK2);
}

void tim2_init(uint32_t timer_freq)
{
        // Enable clock for TIM2.
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

        /*
         * PCLK1 = HCLK / 2 = 50 MHz (max allowed on STM32F411).
         * APB1 timer clock = 100 MHz (because APB1 prescaler = 2).
         * We want for timer 2 to run at frequency of TIM2_CLK = 10 kHz.
         */

        // Calculate the value of prescaler so timer 2 runs at 10 kHz.
        uint32_t prescaler            = (APB1_TIM_CLK / TIM2_CLK) - 1UL;
        // Auto-reload register value.
        uint32_t auto_reload_register = (TIM2_CLK / timer_freq) - 1UL;

        // Set prescaler and auto-reload.
        TIM2->PSC = prescaler;
        TIM2->ARR = auto_reload_register;

        // Set up-counting, edge-aligned mode.
        TIM2->CR1 &= ~(TIM_CR1_CMS | TIM_CR1_DIR);

        // Enable ARR preload (prescaler (PSC) is always buffered).
        TIM2->CR1 |= TIM_CR1_ARPE;

        // Enable update event interrupt.
        TIM2->DIER |= TIM_DIER_UIE;

        // Generate an update first to load preloaded value.
        TIM2->EGR |= TIM_EGR_UG;

        // Clear pending flags.
        TIM2->SR = 0;

        // Clear pending interrupt.
        NVIC_ClearPendingIRQ(TIM2_IRQn);
        // Set priority and disable TIM2 interrupt in NVIC for now.
        NVIC_SetPriority(TIM2_IRQn, 0);
        NVIC_DisableIRQ(TIM2_IRQn);
}

void tim3_init(uint32_t timer_freq)
{
        // Enable clock for TIM3.
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

        /*
         * PCLK1 = HCLK / 2 = 50 MHz (max allowed on STM32F411).
         * APB1 timer clock = 100 MHz (because APB1 prescaler = 2).
         * We want for timer 3 to run at frequency of TIM3_CLK = 10 kHz.
         */

        // Calculate the value of prescaler so timer 3 runs at 10 kHz.
        uint32_t prescaler            = (APB1_TIM_CLK / TIM3_CLK) - 1UL;
        // Auto-reload register value.
        uint32_t auto_reload_register = (TIM3_CLK / timer_freq) - 1UL;

        // Set prescaler and auto-reload.
        TIM3->PSC = prescaler;
        TIM3->ARR = auto_reload_register;

        // Enable ARR preload (prescaler (PSC) is always buffered).
        TIM3->CR1 |= TIM_CR1_ARPE;

        // Enable update event interrupt.
        TIM3->DIER |= TIM_DIER_UIE;

        // Generate an update first to load preloaded value.
        TIM3->EGR |= TIM_EGR_UG;

        // Clear pending flags.
        TIM3->SR = 0;

        // Clear pending interrupt.
        NVIC_ClearPendingIRQ(TIM3_IRQn);
        // Set priority and enable TIM3 interrupt in NVIC.
        NVIC_SetPriority(TIM3_IRQn, 2);
        NVIC_EnableIRQ(TIM3_IRQn);

        // Enable TIM3 counter.
        TIM3->CR1 |= TIM_CR1_CEN;
}

void tim2_update_loop(void)
{
        converter_type_t converter_type = converter_get_type();

        // The reference value chosen by the user.
        float ref = pid_get_ref();

        // Converter output voltage which is used for comparison with the reference value.
        float measurement = y[0][0];

        // LED PWM duty cycle.
        float duty;

        if (converter_type == DC_DC_IDEAL)
        {
                /*
                 * Based on the assignment instruction, in the basic version (ideal H-bridge), the
                 * converter model (plant) takes a reference directly from the controller output.
                 * The only difference between DC_DC_IDEAL and INVERTER_IDEAL converter types is
                 * that to get the desired output values for each type, the user should configure
                 * the controller appropriately for each type. The converter has been descritized
                 * with sampling time of 1/(50000 Hz) = 20 us. In order to be able to watch the
                 * changes, we should execute each update, including controller and converter
                 * update, in a slower rate. This slower rate here is TIM2 frequency.
                 */

                // Update the pid control which makes the input for the plant.
                u[0][0] = pid_update(ref, measurement);

                // update the converter state vector with pid controller output as the input.
                converter_update(u, y);

                /*
                 * The duty cycle for LED PWM in this type is calculated by normalizing the pid
                 * output (or plant input). Normalization is done with respect to the maximum value
                 * for the reference (REF_MAX) which is 50.
                 */
                duty = 100.0f * CLAMP((u[0][0] / REF_MAX), 0.0f, 1.0f);
        }
        else
        {
                // Calculate the reference voltage phase at this update instance.
                converter_ref_phase += converter_ref_dphi;

                if (converter_ref_phase >= 2.0f * PI)
                {
                        converter_ref_phase -= 2.0f * PI;
                }

                // Real reference value in this type is amplitude * sin(converter_ref_phase).
                ref = ref * sinf(converter_ref_phase);

                u[0][0] = pid_update(ref, measurement);

                // update the converter state vector with pid controller output as the plant input.
                converter_update(u, y);

                /*
                 * The duty cycle for LED PWM in this type is the same as DC_DC_IDEAL type.
                 * ABS_FLOAT function-like macro is used here to turn negative values of voltage
                 * into positive values. So the brightness of LED is highest when the output voltage
                 * is at either peak and LED is almost off when the output voltage is passing 0 V.
                 */
                duty = 100.0f * CLAMP(ABS_FLOAT(u[0][0] / REF_MAX), 0.0f, 1.0f);
        }

        // Next, we change the brightness of the green LED.
        pwm_tim2_set_duty(duty);
}

void tim3_read_button(void)
{
        // Detect change in button status to register it as one button press.
        bool button_is_pressed = gpio_button_is_pressed();
        if (button_is_pressed && !button_last_push_status)
        {
                cli_button_handler();
        }
        button_last_push_status = button_is_pressed;
}