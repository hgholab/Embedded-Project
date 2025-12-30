#ifndef CONTROLLER_H
#define CONTROLLER_H

#define REF_MAX 50.0f

typedef struct pid_controller *controller;

extern struct pid_controller pid;

void pid_init(controller pid,
              float kp,
              float ki,
              float kd,
              float Ts,
              float int_out_min,
              float int_out_max,
              float controller_out_min,
              float controller_out_max,
              float reference);
float pid_update(controller pid, float measurement);
void pid_set_kp(controller pid, float kp);
float pid_get_kp(controller pid);
void pid_set_ki(controller pid, float ki);
float pid_get_ki(controller pid);
void pid_set_kd(controller pid, float kd);
float pid_get_kd(controller pid);
void pid_set_ref(controller pid, float new_ref);
float pid_get_ref(controller pid);
void pid_clear_integrator(controller pid);

#endif
