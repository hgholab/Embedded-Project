#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#include "stm32f4xx.h"

#include "timer.h"

#include "cli.h"
#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "gpio.h"
#include "pwm.h"
#include "scheduler.h"
#include "systick.h"
#include "terminal.h"
#include "utils.h"

#define TIM2_CLK 100000UL // TIM2 clock frequency
#define TIM3_CLK 10000UL  // TIM3 clock frequency

float timer_sine_ref = 0.0f;

static bool tim2_update_event_is_pending = false;
static bool tim2_ccr_event_is_pending    = false;
static bool tim2_dir = false; // false means upcounting and true means downcounting

// TIM2 update event interrupt is used for updating the control loop and providing PWM for the LED.
void TIM2_IRQHandler(void)
{
        if (TIM2->SR & TIM_SR_UIF) // An update event interrupt has happend.
        {
                // Clear UIF flag.
                TIM2->SR &= ~TIM_SR_UIF;

                // Set the update event flag.
                tim2_update_event_is_pending = true;
        }

        if (TIM2->SR & TIM_SR_CC1IF) // An interrupt has happened becasue CCR = CNT.
        {
                TIM2->SR &= ~TIM_SR_CC1IF;

                tim2_ccr_event_is_pending = true;
        }

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
         * We want for timer 2 to run at frequency of TIM2_CLK = 100 kHz.
         */

        // Calculate the value of prescaler so timer 2 runs at 100 kHz.
        uint32_t prescaler            = (APB1_TIM_CLK / TIM2_CLK) - 1UL;
        // Auto-reload register value.
        uint32_t auto_reload_register = (TIM2_CLK / timer_freq) - 1UL;

        // Set prescaler and auto-reload.
        TIM2->PSC = prescaler;
        TIM2->ARR = auto_reload_register;

        // Set up-counting, edge-aligned mode
        TIM2->CR1 &= ~(TIM_CR1_CMS | TIM_CR1_DIR);

        // Enable ARR preload (prescaler (PSC) is always buffered).
        TIM2->CR1 |= TIM_CR1_ARPE;

        // Generate an update first to load preloaded value.
        TIM2->EGR |= TIM_EGR_UG;

        // Clear pending flags.
        TIM2->SR = 0;

        // Clear pending interrupt.
        NVIC_ClearPendingIRQ(TIM2_IRQn);
        // Set priority and disable TIM2 interrupt in NVIC for now.
        NVIC_SetPriority(TIM2_IRQn, 0);
        NVIC_DisableIRQ(TIM2_IRQn);

        // Enable TIM2 counter.
        TIM2->CR1 |= TIM_CR1_CEN;
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
        uint32_t tim3_clock           = TIM3_CLK;
        // Calculate the value of prescaler so timer 3 runs at 10 kHz.
        uint32_t prescaler            = (APB1_TIM_CLK / tim3_clock) - 1UL;
        // Auto-reload register value.
        uint32_t auto_reload_register = (tim3_clock / timer_freq) - 1UL;

        // Set prescaler and auto-reload.
        TIM3->PSC = prescaler;
        TIM3->ARR = auto_reload_register;

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

        switch (converter_type)
        {
        case DC_DC_IDEAL:
                /*
                 * Based on the assignment instruction, in the basic version, the converter model
                 * (plant) takes a reference directly from the controller output. The only
                 * difference between DC_DC_IDEAL and INVERTER_IDEAL converter types is that to get
                 * the desired output values for each type, the user should configure the controller
                 * appropriately for each type. The converter has been descritized with sampling
                 * time of 1/(50000 Hz) = 20 us. In order to be able to watch the changes, we should
                 * execute each update, including controller and converter update, in a slower rate.
                 * This slower rate here is TIM2 frequency (updates happen at TIM2 update event
                 * instant). TIM2 in this type, INVERTER_IDEAL, and DC_DC_H_BRIDGE is up-counting.
                 */

                // Update the pid control which makes the input for the plant.
                u[0][0] = pid_update(ref, measurement);

                // update the converter state vector with pid controller output as the input.
                converter_update(u, y);

                /*
                 * The duty cycle for LED PWM in this type is calculated by normalizing the pid
                 * output (or plant input). Normalization is done with respect to the maximum value
                 * for the reference.
                 */
                duty = 100.0f * CLAMP((u[0][0] / REF_MAX), 0.0f, 1.0f);

                break;

        case INVERTER_IDEAL:

                converter_ref_phase += converter_ref_dphi;

                if (converter_ref_phase >= 2.0f * PI)
                {
                        converter_ref_phase -= 2.0f * PI;
                }

                ref = ref * sinf(converter_ref_phase);

                /*
                 * Here, we generate a sinusoidal reference, so that we can subtract the output
                 * voltage (measurement) from it
                 */

                u[0][0] = pid_update(ref, measurement);

                // update the converter state vector with pid controller output as the plant input.
                converter_update(u, y);

                /*
                 * The duty cycle for LED PWM in this type is the same as DC_DC_IDEAL type.
                 * ABS_FLOAT function-like macro is used here to turn negative values of voltage
                 * into positive values.
                 */
                duty = 100.0f * CLAMP(ABS_FLOAT(u[0][0] / REF_MAX), 0.0f, 1.0f);

                break;

        case DC_DC_H_BRIDGE:
                if (tim2_update_event_is_pending)
                {
                        /*
                         * Update event interrupt (end of the switching period). If we show the DC
                         * link voltage as v_link, here bridge output voltage goes from -v_link to
                         * v_link.
                         */

                        // Clear the pending update event flag.
                        tim2_update_event_is_pending = false;

                        uint32_t arr = TIM2->ARR;
                        TIM2->CCR1 =
                                (uint32_t)(CLAMP(pid_update(ref, measurement), 0.0f, (float)arr));
                        uint32_t ccr = TIM2->CCR1;

                        duty    = (100.0f * ccr) / (arr + 1);
                        u[0][0] = converter_dc_link_voltage;
                        converter_update(u, y);
                }
                else if (tim2_ccr_event_is_pending)
                {
                        /*
                         * Counter match interrupt (somewhere in the middle of the switching
                         * period). Here bridge output voltage goes from v_link to -v_link.
                         */

                        // Clear the counter match event flag.
                        tim2_ccr_event_is_pending = false;

                        u[0][0] = -1.0f * converter_dc_link_voltage;
                        converter_update(u, y);
                }

                break;

        case INVERTER_H_BRIDGE:
                if (tim2_update_event_is_pending)
                {
                        /*
                         * Update event interrupt (end of the switching period). If we show the DC
                         * link voltage as v_link, here bridge output voltage goes from -v_link to
                         * v_link.
                         */

                        // Clear the pending update event flag.
                        tim2_update_event_is_pending = false;

                        // Does this cause distortion? if yes should I handle this in CLAMP below?
                        // ref = ref * (0.5f + 0.5f * sine_lut_value(sine_lut_get_ref_index()));
                        // sine_lut_increment_ref_index();

                        uint32_t arr = TIM2->ARR;
                        TIM2->CCR1 =
                                (uint32_t)(CLAMP(pid_update(ref, measurement), 0.0f, (float)arr));
                        uint32_t ccr = TIM2->CCR1;

                        duty    = (100.0f * ccr) / (arr);
                        u[0][0] = converter_dc_link_voltage;
                        converter_update(u, y);
                }
                else if (tim2_ccr_event_is_pending && !(TIM2->CR1 & TIM_CR1_DIR))
                {
                        /*
                         * Counter match interrupt while counting up. Here bridge output voltage
                         * goes from v_link to -v_link.
                         */

                        // Clear the counter match event flag.
                        tim2_ccr_event_is_pending = false;

                        u[0][0] = -1.0f * converter_dc_link_voltage;
                        converter_update(u, y);
                }
                else if (tim2_ccr_event_is_pending && (TIM2->CR1 & TIM_CR1_DIR))
                {
                        /*
                         * Counter match interrupt while counting down. Here bridge output voltage
                         * goes from -v_link to v_link.
                         */

                        // Clear the counter match event flag.
                        tim2_ccr_event_is_pending = false;

                        u[0][0] = converter_dc_link_voltage;
                        converter_update(u, y);
                }

                break;
        }

        /*
         * Next, we change the brightness of the green LED.
         * In the ideal H-bridge where PWM signal is directly fed into the converter model, we
         * normalize the absolute value of this voltage by REF_MAX which is chosen to be 50. In the
         * bonus task, where we simulate a H-bridge, the story is a bit different. In the DC-DC
         * version, controller output is compared to a repeating sequence (TIM2 up-counter here) and
         * generates the PWM for the LED. In the inverter version of H-bridge, we have chosen to
         * implement the bipolar modulation of a full-bridge single-phase inverter, and controller
         * output is comapared to a traingular carrier (TIM2 up-down counter here) which generates
         * the PWM. The following function call sets the duty for TIM2 CH1 PWM.
         */
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