/*
 * pwm.c
 *
 * Description:
 *     Configures TIM2 Channel 1 to generate a PWM signal.
 *     Output pin:
 *     - PA5 as TIM2_CH1 (AF01)
 *
 * Note:
 *     This module assumes that PA5 is already configured
 *     as alternate function AF01 for TIM2_CH1.
 */

#include <stdint.h>

#include "stm32f4xx.h"

#include "pwm.h"

#include "utils.h"

#define TIM_CCMR1_OC1M_PWM1 (6UL << TIM_CCMR1_OC1M_Pos)

/*
 * Initialize the PWM subsystem.
 *
 * Configures the timer 2 channel 1 to generate a fixed-frequency PWM signal.
 * The PWM frequency is set by the auto-reload register, while the duty cycle
 * is initially set to 0%.
 */
void pwm_tim2_init(void)
{
        // Disable CH1 output first.
        pwm_tim2_disable();

        /*
         * Clear OC1M (mode bits) and OC1PE (preload enable) first. Then we set the
         * channel one on PWM mode 1 and also before enabling the channel output, we
         * enable the preload.
         */
        TIM2->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE);
        // PWM mode 1 on CH1.
        TIM2->CCMR1 |= TIM_CCMR1_OC1M_PWM1;

        // Clear CC1P to make the output active high
        TIM2->CCER &= ~TIM_CCER_CC1P;

        // Configure CCR1 (start with 0% duty cycle).
        TIM2->CCR1 = 0U;

        // Enable CCR1 preload.
        TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

        // Enable output for CH1.
        pwm_tim2_enable();
}

void pwm_tim2_set_duty(float duty)
{
        // duty is in percentage (0 - 100)
        duty         = CLAMP(duty, 0.0f, 100.0f);
        uint32_t arr = TIM2->ARR;
        uint32_t ccr = (uint32_t)(((arr + 1UL) * duty + 50.0f) / 100.0f);
        TIM2->CCR1   = ccr;
}

void pwm_tim2_disable(void)
{
        TIM2->CCER &= ~TIM_CCER_CC1E;
}

void pwm_tim2_enable(void)
{
        TIM2->CCER |= TIM_CCER_CC1E;
}