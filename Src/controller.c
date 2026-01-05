/*
 * controller.c
 *
 * Description:
 *     Discrete-time PID controller implementation.
 *
 * Notes:
 *     - pid_init() initializes controller parameters and state.
 *     - pid_update() computes one control step.
 *     - Gain and reference setter/getter functions are provided.
 */

#include "controller.h"

#include "utils.h"

struct pid_controller
{
        float kp;
        float ki;
        float kd;
        float Ts;
        float prev_error;
        float integral;
        float int_out_min;
        float int_out_max;
        float controller_out_min;
        float controller_out_max;
        float reference;
};

struct pid_controller pid;

void pid_init(controller pid,
              float kp,
              float ki,
              float kd,
              float Ts,
              float int_out_min,
              float int_out_max,
              float controller_out_min,
              float controller_out_max,
              float reference)
{
        pid->kp                 = kp;
        pid->ki                 = ki;
        pid->kd                 = kd;
        pid->Ts                 = Ts; // Sampling time in seconds
        pid->prev_error         = 0.0f;
        pid->integral           = 0.0f; // Accumulated integral term
        pid->int_out_min        = int_out_min;
        pid->int_out_max        = int_out_max;
        pid->controller_out_min = controller_out_min;
        pid->controller_out_max = controller_out_max;
        pid->reference          = reference;
}

// float pid_update(controller pid, float measurement)
// {
//         // Compute error.
//         float error = pid->reference - measurement;

//         // Calculate proportional term.
//         float p     = pid->kp * error;

//         // Calculate integral term.
//         pid->integral += (pid->ki * pid->Ts * error);

//         // limit integral term to avoid windup.
//         pid->integral    = CLAMP(pid->integral, pid->int_out_min, pid->int_out_max);

//         float i          = pid->integral;

//         // Calculate derivative term.
//         float derivative = (error - pid->prev_error) / pid->Ts;
//         float d          = pid->kd * derivative;

//         // Calculate PID controller output.
//         float output     = p + i + d;

//         // Apply output saturation.
//         output           = CLAMP(output, pid->controller_out_min, pid->controller_out_max);

//         // Save state for next call.
//         pid->prev_error  = error;

//         return output;
// }

float pid_update(controller pid, float measurement)
{
        float e = pid->reference - measurement;

        // P
        float p = pid->kp * e;

        // I contribution uses previous integrator state (z^-1 convention)
        float i = pid->integral;

        // D
        float d = pid->kd * (e - pid->prev_error) / pid->Ts;

        // Output uses I[k-1]
        float output = p + i + d;

        // Update integrator after computing output: I[k] = I[k-1] + Ki*Ts*e[k]
        pid->integral += pid->ki * pid->Ts * e;

        pid->prev_error = e;
        return output;
}

void pid_set_kp(controller pid, float kp)
{
        pid->kp = kp;
}

float pid_get_kp(controller pid)
{
        return pid->kp;
}

void pid_set_ki(controller pid, float ki)
{
        pid->ki = ki;
}

float pid_get_ki(controller pid)
{
        return pid->ki;
}

void pid_set_kd(controller pid, float kd)
{
        pid->kd = kd;
}

float pid_get_kd(controller pid)
{
        return pid->kd;
}

void pid_set_ref(controller pid, float new_ref)
{
        pid->reference = new_ref;
}

float pid_get_ref(controller pid)
{
        return pid->reference;
}

void pid_clear_integrator(controller pid)
{
        pid->integral = 0;
}