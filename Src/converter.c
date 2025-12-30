#include "converter.h"

struct converter_model
{
        float x[STATES_NUM][1];
};

struct converter_model plant;

// plant input voltage marked U_in in assignment description.
float u[INPUTS_NUM][1] = {{0.0f}};
// plant output voltage marked U_3 in assignment description.
float y[OUTPUTS_NUM][1] = {{0.0f}};

// State-space matrices definitions
// clang-format off
static const float Ad[STATES_NUM][STATES_NUM] = {
        {0.9652f, -0.0172f,  0.0057f, -0.0058f,  0.0052f, -0.0251f},
        {0.7732f,  0.1252f,  0.2315f,  0.0700f,  0.1282f,  0.7754f},
        {0.8278f, -0.7522f, -0.0956f,  0.3299f, -0.4855f,  0.3915f},
        {0.9948f,  0.2655f, -0.3848f,  0.4212f,  0.3927f,  0.2899f},
        {0.7648f, -0.4165f, -0.4855f, -0.3366f, -0.0986f,  0.7281f},
        {1.1056f,  0.7587f, -0.1179f,  0.0748f, -0.2192f,  0.1491f},
};
// clang-format on
static const float Bd[STATES_NUM][INPUTS_NUM]  = {{0.0471f}, {0.0377f}, {0.4040f},
                                                  {0.0485f}, {0.0373f}, {0.0539f}};
static const float Cd[OUTPUTS_NUM][STATES_NUM] = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};
static const float Dd[OUTPUTS_NUM][INPUTS_NUM] = {{0.0f}};

void converter_init(converter model)
{
        for (int i = 0; i < STATES_NUM; i++)
                (model->x)[i][0] = 0.0f;
}

void converter_update(converter model, float const u[INPUTS_NUM][1], float y[OUTPUTS_NUM][1])
{
        float x_next[STATES_NUM][1];

        // x_next = Ad * x + Bd * u
        for (int i = 0; i < STATES_NUM; i++)
        {
                float result = 0.0f;

                for (int j = 0; j < STATES_NUM; j++)
                {
                        result += (Ad[i][j] * model->x[j][0]);
                }
                for (int k = 0; k < INPUTS_NUM; k++)
                {
                        result += Bd[i][k] * u[k][0];
                }

                x_next[i][0] = result;
        }

        // y = Cd * x + Dd * u
        for (int i = 0; i < OUTPUTS_NUM; i++)
        {
                float result = 0.0f;

                for (int j = 0; j < STATES_NUM; j++)
                {
                        result += Cd[i][j] * model->x[j][0];
                }
                for (int k = 0; k < INPUTS_NUM; k++)
                {
                        result += Dd[i][k] * u[k][0];
                }

                y[i][0] = result;
        }

        // Update the state to the new state.
        for (int i = 0; i < STATES_NUM; i++)
        {
                (model->x)[i][0] = x_next[i][0];
        }
}