#ifndef CONVERTER_H
#define CONVERTER_H

#define STATES_NUM  6
#define INPUTS_NUM  1
#define OUTPUTS_NUM 1
#define MODES_NUM   3
#define TYPES_NUM   4

typedef enum
{
        IDLE,
        CONFIG,
        MOD
} converter_mode_t;

typedef enum
{
        DC_DC_IDEAL,
        INVERTER_IDEAL,
        DC_DC_H_BRIDGE,
        INVERTER_H_BRIDGE
} converter_type_t;

extern float converter_ref_dphi;
extern float converter_ref_phase;
extern const char *const modes[];
extern const char *const types[];
extern const char *const types_id[];
extern const float converter_dc_link_voltage;
extern float u[][1];
extern float y[][1];

void converter_init(void);
void converter_reset_state(void);
void converter_update(const float u[INPUTS_NUM][1], float y[OUTPUTS_NUM][1]);
converter_type_t converter_get_type(void);
void converter_set_type(converter_type_t type);
converter_mode_t converter_get_mode(void);
void converter_set_mode(converter_mode_t mode);

#endif
