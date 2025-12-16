#ifndef CONTROLLER_H
#define CONTROLLER_H

typedef struct
{
    float Kp;
    float Ki;
    float Kd;
    float Ts;
    float prev_error;
    float integral;
    float int_out_min;
    float int_out_max;
    float controller_out_min;
    float controller_out_max;
} PID_controller_t;

void init_PID(PID_controller_t *pid,
              float Kp, float Ki, float Kd, float Ts,
              float int_out_min, float int_out_max,
              float controller_out_min, float controller_out_max);
float update_PID(PID_controller_t *pid, float ref, float measurement);

#endif
