#ifndef CONVERTER_H
#define CONVERTER_H

#define STATES_NUM 6
#define INPUTS_NUM 1
#define OUTPUTS_NUM 1

typedef struct converter_model *converter;

extern struct converter_model plant;
extern float u[][1];
extern float y[][1];

void converter_init(converter model);
void converter_update(converter model, const float u[INPUTS_NUM][1], float y[OUTPUTS_NUM][1]);

#endif
