#include "converter.h"

static const float Ad[N_STATES][N_STATES] =
{
    {0.9652f, -0.0172f, 0.0057f, -0.0058f, 0.0052f, -0.0251f},
    {0.7732f, 0.1252f, 0.2315f, 0.0700f, 0.1282f, 0.7754f},
    {0.8278f, -0.7522f, -0.0956f, 0.3299f, -0.4855f, 0.3915f},
    {0.9948f, 0.2655f, -0.3848f, 0.4212f, 0.3927f, 0.2899f},
    {0.7648f, -0.4165f, -0.4855f, -0.3366f, -0.0986f, 0.7281f},
    {1.1056f, 0.7587f, -0.1179f, 0.0748f, -0.2192f, 0.1491f},
};

static const float Bd[N_STATES][N_INPUTS] =
{
    {0.0471f},
    {0.0377f},
    {0.4040f},
    {0.0485f},
    {0.0373f},
    {0.0539f}
};

static const float Cd[N_OUTPUTS][N_STATES] =
{
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}
};

static const float Dd[N_OUTPUTS][N_INPUTS] =
{
    {0.0f}
};

void init_converter_model(Converter_model_t *model)
{
    for (int i = 0; i < N_STATES; i++)
        model->x[i][0] = 0.0f;
}

void converter_model_step(Converter_model_t *model, const float u[N_INPUTS][1],
    float y[N_OUTPUTS][1])
{
    float x_next[N_STATES][1];

    // x_next = Ad * x + Bd * u
    for (int i = 0; i < N_STATES; i++)
    {
        float result = 0.0f;
        for (int j = 0; j < N_STATES; j++)
        {
             result += (Ad[i][j] * model->x[j][0]);
        }
        for (int k = 0; k < N_INPUTS; k++)
        {
            result += Bd[i][k] * u[k][0];
        }
        x_next[i][0] = result;
    }

    // y = Cd * x + Dd * u
    for (int i = 0; i < N_OUTPUTS; i++)
    {
        float result = 0.0f;
        for (int j = 0; j < N_STATES; j++)
        {
            result += Cd[i][j] * model->x[j][0];
        }
        for (int k = 0; k < N_INPUTS; k++)
        {
            result += Dd[i][k] * u[k][0];
        }

        y[i][0] = result;
    }

    // Update the state to the new state.
    for (int i = 0; i < N_STATES; i++)
    {
        model->x[i][0] = x_next[i][0];
    }
}