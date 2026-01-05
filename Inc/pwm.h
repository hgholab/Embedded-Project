#ifndef PWM_H
#define PWM_H

void pwm_tim2_init(void);
void pwm_tim2_set_duty(float duty);
void pwm_tim2_disable(void);
void pwm_tim2_enable(void);

#endif
