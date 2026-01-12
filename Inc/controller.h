#ifndef CONTROLLER_H
#define CONTROLLER_H

// Maximum reference value is the same as DC link voltage
#define REF_MAX 50.0f

void pid_init(float kp,
              float ki,
              float kd,
              float Ts,
              float int_out_min,
              float int_out_max,
              float controller_out_min,
              float controller_out_max);
float pid_update(float reference, float measurement);
void pid_set_kp(float kp);
float pid_get_kp(void);
void pid_set_ki(float ki);
float pid_get_ki(void);
void pid_set_kd(float kd);
float pid_get_kd(void);
void pid_set_ref(float new_ref);
float pid_get_ref(void);
void pid_clear_integrator(void);
void pid_clear_prev_error(void);

#endif