/*
 * converter.c
 *
 * Description:
 *     Discrete-time state-space plant model for the converter.
 *
 *     Implements:
 *         x[k+1] = Ad*x[k] + Bd*u[k]
 *         y[k]   = Cd*x[k] + Dd*u[k]
 *
 * Notes:
 *     - State vector is stored inside the model instance.
 *     - converter_init() zeros the state and set the type to DC-DC ideal.
 *     - converter_reset_state() zeros the state.
 *     - converter_update() performs one simulation step.
 */

#include <stdatomic.h>
#include <stddef.h>

#include "stm32f4xx.h"

#include "converter.h"

#include "cli.h"
#include "controller.h"
#include "pwm.h"
#include "scheduler.h"
#include "utils.h"

#define SAMPLING_FREQUENCY 50000.0f
#define SINE_FREQUENCY     50.0f

struct converter_model
{
        float x[STATES_NUM][1];
};

// Phase change for reference in one time-step.
const float converter_ref_dphi = 2.0f * PI * SINE_FREQUENCY * (1.0f / SAMPLING_FREQUENCY);
float converter_ref_phase      = 0.0f; // Reference phase at each instant.

const char *const modes[MODES_NUM]    = {"idle", "config", "mod"};
const char *const types[TYPES_NUM]    = {"DC-DC ideal bridge", "inverter ideal bridge"};
const char *const types_id[TYPES_NUM] = {"0", "1"};

// Define and initialize plant input voltage marked U_in in assignment description.
float u[INPUTS_NUM][1]  = {{0.0f}};
// Define and initialize plant output voltage marked U_3 in assignment description.
float y[OUTPUTS_NUM][1] = {{0.0f}};

static struct converter_model plant;
static converter_type_t converter_type = DC_DC_IDEAL;
static converter_mode_t current_mode   = IDLE;

// clang-format off
// State-space matrices definitions
static const float Ad[STATES_NUM][STATES_NUM] = {
        {0.9652f, -0.0172f,  0.0057f, -0.0058f,  0.0052f, -0.0251f},
        {0.7732f,  0.1252f,  0.2315f,  0.0700f,  0.1282f,  0.7754f},
        {0.8278f, -0.7522f, -0.0956f,  0.3299f, -0.4855f,  0.3915f},
        {0.9948f,  0.2655f, -0.3848f,  0.4212f,  0.3927f,  0.2899f},
        {0.7648f, -0.4165f, -0.4855f, -0.3366f, -0.0986f,  0.7281f},
        {1.1056f,  0.7587f, -0.1179f,  0.0748f, -0.2192f,  0.1491f},
};
static const float Bd[STATES_NUM][INPUTS_NUM] = {{0.0471f},
                                                 {0.0377f}, 
                                                 {0.4040f}, 
                                                 {0.0485f}, 
                                                 {0.0373f}, 
                                                 {0.0539f}};
static const float Cd[OUTPUTS_NUM][STATES_NUM] = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
static const float Dd[OUTPUTS_NUM][INPUTS_NUM] = {{0.0f}};
// clang-format on

/*
 * This function initialize a converter model with the state vector of zero and set the type to
 * DC-DC ideal and mode to idle.
 */
void converter_init(void)
{
        converter_reset_state();
        converter_set_type(DC_DC_IDEAL);
        converter_set_mode(IDLE);
}

void converter_reset_state(void)
{
        for (size_t i = 0; i < STATES_NUM; i++)
                (plant.x)[i][0] = 0.0f;
}

void converter_update(float const u[INPUTS_NUM][1], float y[OUTPUTS_NUM][1])
{
        float x_next[STATES_NUM][1];

        // x_(n+1) = Ad * x_n + Bd * u_n
        for (size_t i = 0; i < STATES_NUM; i++)
        {
                float result = 0.0f;

                for (size_t j = 0; j < STATES_NUM; j++)
                {
                        result += (Ad[i][j] * (plant.x)[j][0]);
                }
                for (size_t k = 0; k < INPUTS_NUM; k++)
                {
                        result += Bd[i][k] * u[k][0];
                }

                x_next[i][0] = result;
        }

        // Update the model's state to the new state (x_n -> x_(n+1))
        for (size_t i = 0; i < STATES_NUM; i++)
        {
                (plant.x)[i][0] = x_next[i][0];
        }

        /*
         * y_(n+1) = Cd * x_(n+1) (state and output should be at the same time index after this
         * function returns). Here Dd is zero and it is not included in the calculations.
         */
        for (size_t i = 0; i < OUTPUTS_NUM; i++)
        {
                float result = 0.0f;

                for (size_t j = 0; j < STATES_NUM; j++)
                {
                        result += Cd[i][j] * (plant.x)[j][0];
                }
                y[i][0] = result;
        }
}

converter_type_t converter_get_type(void)
{
        return converter_type;
}

void converter_set_type(converter_type_t type)
{
        converter_type = type;
}

converter_mode_t converter_get_mode(void)
{
        return current_mode;
}

void converter_set_mode(converter_mode_t mode)
{
        if (mode == IDLE || mode == CONFIG)
        {
                /*
                 * Stop updating the control loop, and converter state
                 * vector by disabling timer 2 interrupt.
                 */
                NVIC_DisableIRQ(TIM2_IRQn);

                // Disable TIM2 counter.
                TIM2->CR1 &= ~TIM_CR1_CEN;

                // Reset TIM2 counter register.
                TIM2->CNT = 0U;

                // Clear PID controller integral accumulative term and previous error.
                pid_clear_integrator();
                pid_clear_prev_error();

                // Set plant's input and output to 0.
                u[0][0] = 0.0f;
                y[0][0] = 0.0f;

                // Reset converter state vector.
                converter_reset_state();

                /*
                Remove TASK0 from the scheduler so that we do not update the loop accidentally after
                coming out of MOD mode.
                */
                atomic_fetch_and(&ready_flag_word, ~TASK0);

                // Turn off TIM2 PWM so that green LED turns off.
                pwm_tim2_set_duty(0.0f);
                pwm_tim2_disable();

                // Disable stream to make sure it is off in idle and config modes
                cli_stream_is_on = false;
        }
        else
        {
                /*
                 * In modulation mode. Start updating the control loop and converter state
                 * vector in modulation mode by enabling timer 2 interrupt.
                 */
                NVIC_EnableIRQ(TIM2_IRQn);

                // Enable TIM2 counter.
                TIM2->CR1 |= TIM_CR1_CEN;

                // Turn on TIM2 PWM so that green LED turns on.
                pwm_tim2_enable();
        }

        current_mode = mode;
        // Confgiure mode LEDs.
        cli_configure_mode_LEDs(current_mode);
        // Configure terminal text color.
        cli_configure_text_color(current_mode);
}