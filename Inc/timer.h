#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * TIM2 frequency determines the rate of the update of the control loop and converter state
 * vector. It is also the frequency of PWM signal driving the green LED, because it uses
 * channel 1 of TIM2. TIM2 update event frequency is loop update frequency meaning each second model
 * goes TIM2_FREQUENCY steps ahead where each step is h = 1/Fs = 1/50e3 = 2e-5 in real time.
 *
 * TIM3 frequency determines the time for debounce of the push-button. Here the frequency is
 * chosen to be 50 Hz so that a valid press button signal should last for at least 20 ms.
 */
#define TIM2_FREQUENCY 200UL
#define TIM3_FREQUENCY 50UL

void tim3_init(uint32_t timer_freq);
void tim2_init(uint32_t timer_freq);
void tim2_update_loop(void);
void tim3_read_button(void);

#endif