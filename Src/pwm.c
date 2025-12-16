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


#include "stm32f4xx.h"

#include "pwm.h"

#include "clock.h"
#include "utils.h"


#define TIM_CCMR1_OC1M_PWM1        (6UL << TIM_CCMR1_OC1M_Pos)


/*
 * Initialize the PWM subsystem.
 *
 * Configures the timer 2 channel 1 to generate a fixed-frequency PWM signal.
 * The PWM frequency is set by the auto-reload register, while the duty cycle
 * is initially set to 0%.
 */
void pwm_init(uint32_t pwm_freq_hz)
{
    // Enable clock for TIM2.
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // Disable CH1 output first.
    pwm_disable();

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

    /*
     * PCLK1 = HCLK / 2 = 50 MHz (max allowed on STM32F411).
     * APB1 timer clock = 100 MHz (because APB1 prescaler = 2).
     * We want for timer 2 to run at APB1_TIM_CLK, since we want the highest
     * resolution.
     */
    uint32_t psc_clk = APB1_TIM_CLK;
    // Calculate the value of prescaler so APB1 timers run at APB1_TIM_CLK.
    uint32_t prescaler = (APB1_TIM_CLK / psc_clk) - 1;
    // Auto-reload register value.
    uint32_t arr = (psc_clk / pwm_freq_hz) - 1;

    // Set prescaler and auto-reload.
    TIM2->PSC = prescaler;
    TIM2->ARR = arr;

    // Configure CCR1 (start with 0% duty cycle).
    TIM2->CCR1 = 0;

    // Enable ARR preload.
    TIM2->CR1 |= TIM_CR1_ARPE;
    // Enable CCR1 preload.
    TIM2->CCMR1 |= TIM_CCMR1_OC1PE;

    // Generate an update first to load preloads.
    TIM2->EGR |= TIM_EGR_UG;

    // Enable counter.
    TIM2->CR1 |= TIM_CR1_CEN;

    // Enable output for CH1.
    pwm_enable();
}

void pwm_set_duty(float duty)
{
    duty = CLAMP(duty, 0.0f, 100.0f);

    uint32_t arr = TIM2->ARR;
    uint32_t ccr = (uint32_t) (((arr + 1UL) * duty + 50.0f) / 100.0f);

    TIM2->CCR1 = ccr;
}

void pwm_disable(void)
{
    TIM2->CCER &= ~TIM_CCER_CC1E;
}

void pwm_enable(void)
{
    TIM2->CCER |= TIM_CCER_CC1E;
}