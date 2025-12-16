#include "utils.h"
#include "controller.h"

void init_PID(PID_controller_t *pid,
              float Kp, float Ki, float Kd, float Ts,
              float int_out_min, float int_out_max,
              float controller_out_min, float controller_out_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->Ts = Ts;               // Sampling time in seconds
    pid->prev_error = 0.0f;
    pid->integral= 0.0f;        // Accumulated integral term
    pid->int_out_min = int_out_min;
    pid->int_out_max = int_out_max;
    pid->controller_out_min = controller_out_min;
    pid->controller_out_max = controller_out_max;
}

float update_PID(PID_controller_t *pid, float ref, float measurement)
{
    // Compute error.
    float error = ref - measurement;

    // Calculate proportional term.
    float P = pid->Kp * error;

    // Calculate integral term.
    pid->integral += pid->Ki * pid->Ts * error;

    // limit integral term to avoid windup.
    pid->integral = CLAMP(pid->integral, pid->int_out_min, pid->int_out_max);

    float I = pid->integral;

    // Calculate derivative term.
    float derivative = (error - pid->prev_error) / pid->Ts;
    float D = pid->Kd * derivative;

    // Calculate PID controller output.
    float output = P + I + D;

    // Apply output saturation.
    output = CLAMP(output, pid->controller_out_min, pid->controller_out_max);

    // Save state for next call.
    pid->prev_error = error;

    return output;
}