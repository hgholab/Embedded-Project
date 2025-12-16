#ifndef PWM_H
#define PWM_H

#include <stdint.h>


void pwm_init(uint32_t pwm_freq_hz);
void pwm_set_duty(float duty);
void pwm_disable(void);
void pwm_enable(void);

#endif
