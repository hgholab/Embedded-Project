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
 *     - converter_init() zeros the state.
 *     - converter_update() performs one simulation step.
 */

#include <stddef.h>

#include "converter.h"

#include "cli.h"
#include "pwm.h"

struct converter_model
{
        float x[STATES_NUM][1];
};
struct converter_model plant;

const char *const modes[MODES_NUM] = {"idle", "config", "mod"};
const char *const types[TYPES_NUM] = {
        "DC-DC ideal bridge", "inverter ideal bridge", "DC-DC H-bridge", "inverter H-bridge"};
const char *const types_id[TYPES_NUM] = {"0", "1", "2", "3"};
// The value for converter DC link is chosen arbitrarily
const float converter_dc_link_voltage = 50.0f;
// Initialize plant input voltage marked U_in in assignment description.
float u[INPUTS_NUM][1]                = {{0.0f}};
// Initialize plant output voltage marked U_3 in assignment description.
float y[OUTPUTS_NUM][1]               = {{0.0f}};

static converter_type_t converter_type = DC_DC_IDEAL;
static mode_t current_mode             = IDLE;

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
// clang-format on
static const float Bd[STATES_NUM][INPUTS_NUM] = {
        {0.0471f}, {0.0377f}, {0.4040f}, {0.0485f}, {0.0373f}, {0.0539f}};
static const float Cd[OUTPUTS_NUM][STATES_NUM] = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
static const float Dd[OUTPUTS_NUM][INPUTS_NUM] = {{0.0f}};

// Initialize a converter model with state vector of zero.
void converter_init(converter model)
{
        for (size_t i = 0; i < STATES_NUM; i++)
                (model->x)[i][0] = 0.0f;
}

void converter_update(converter model, float const u[INPUTS_NUM][1], float y[OUTPUTS_NUM][1])
{
        float x_next[STATES_NUM][1];

        // x_next = Ad * x + Bd * u
        for (size_t i = 0; i < STATES_NUM; i++)
        {
                float result = 0.0f;

                for (size_t j = 0; j < STATES_NUM; j++)
                {
                        result += (Ad[i][j] * model->x[j][0]);
                }
                for (size_t k = 0; k < INPUTS_NUM; k++)
                {
                        result += Bd[i][k] * u[k][0];
                }

                x_next[i][0] = result;
        }

        // y = Cd * x + Dd * u
        for (size_t i = 0; i < OUTPUTS_NUM; i++)
        {
                float result = 0.0f;

                for (size_t j = 0; j < STATES_NUM; j++)
                {
                        result += Cd[i][j] * model->x[j][0];
                }
                for (size_t k = 0; k < INPUTS_NUM; k++)
                {
                        result += Dd[i][k] * u[k][0];
                }

                y[i][0] = result;
        }

        // Update the model's state to the new state.
        for (size_t i = 0; i < STATES_NUM; i++)
        {
                (model->x)[i][0] = x_next[i][0];
        }
}

converter_type_t converter_get_type(void)
{
        return converter_type;
}

void converter_set_type(converter_type_t type)
{
        converter_type = type;

        // This function also needs to change the TIM2 counter mode to up-down for inverter H-brdige
        // converter type.
}

mode_t converter_get_mode(void)
{
        return current_mode;
}

void converter_set_mode(mode_t mode)
{
        current_mode = mode;

        // Confgiure mode LEDs.
        cli_configure_mode_LEDs(mode);
        // Configure terminal text color.
        cli_configure_text_color(mode);

        if (mode == IDLE || mode == CONFIG)
        {
                pwm_tim2_set_duty(0.0f);
                pwm_tim2_disable();
        }
        else
        {
                pwm_tim2_enable();
        }
}
