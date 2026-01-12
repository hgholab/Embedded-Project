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
};

static float reference = 40.0f; // Value of the reference at the start-up.
static struct pid_controller pid;

void pid_init(float kp,
              float ki,
              float kd,
              float Ts,
              float int_out_min,
              float int_out_max,
              float controller_out_min,
              float controller_out_max)
{
        pid.kp                 = kp;
        pid.ki                 = ki;
        pid.kd                 = kd;
        pid.Ts                 = Ts; // Sampling time in seconds
        pid.prev_error         = 0.0f;
        pid.integral           = 0.0f; // Accumulated integral term
        pid.int_out_min        = int_out_min;
        pid.int_out_max        = int_out_max;
        pid.controller_out_min = controller_out_min;
        pid.controller_out_max = controller_out_max;
}

float pid_update(float reference, float measurement)
{
        // Compute error.
        float error = reference - measurement;

        // Calculate proportional term.
        float p = pid.kp * error;

        // Calculate integral term.
        pid.integral += (pid.ki * pid.Ts * error);

        // limit integral term to avoid windup.
        pid.integral = CLAMP(pid.integral, pid.int_out_min, pid.int_out_max);

        float i = pid.integral;

        // Calculate derivative term.
        float d = (error - pid.prev_error) * (pid.kd / pid.Ts);

        // Calculate PID controller output.
        float output = p + i + d;

        // Apply output saturation.
        output = CLAMP(output, pid.controller_out_min, pid.controller_out_max);

        // Save state for next call.
        pid.prev_error = error;

        return output;
}

void pid_set_kp(float kp)
{
        pid.kp = kp;
}

float pid_get_kp()
{
        return pid.kp;
}

void pid_set_ki(float ki)
{
        pid.ki = ki;
}

float pid_get_ki()
{
        return pid.ki;
}

void pid_set_kd(float kd)
{
        pid.kd = kd;
}

float pid_get_kd()
{
        return pid.kd;
}

void pid_set_ref(float new_ref)
{
        reference = new_ref;
}

float pid_get_ref(void)
{
        return reference;
}

void pid_clear_integrator(void)
{
        pid.integral = 0;
}

void pid_clear_prev_error(void)
{
        pid.prev_error = 0;
}