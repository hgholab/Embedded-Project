#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool tim3_flag;

void tim3_init(uint32_t pwm_freq_hz);
void tim2_init(uint32_t pwm_freq_hz);

#endif