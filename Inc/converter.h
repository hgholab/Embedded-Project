#ifndef CONVERTER_H
#define CONVERTER_H

#define N_STATES         6
#define N_INPUTS         1
#define N_OUTPUTS        1

typedef struct
{
    float x[N_STATES][1];
} Converter_model_t;

void init_converter_model(Converter_model_t *model);
void converter_model_step(Converter_model_t *model, const float u[N_INPUTS][1],
    float y[N_OUTPUTS][1]);

#endif
