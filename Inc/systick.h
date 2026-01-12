#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

extern uint16_t systick_print_counter;

void systick_init(void);
uint32_t systick_get_ticks(void);
void systick_print_output(void);

#endif
