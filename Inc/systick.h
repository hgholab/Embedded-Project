#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
        SYSTICK_EXTERNAL_CLOCK  = 0,
        SYSTICK_PROCESSOR_CLOCK = 1
} systick_clock_source_t;

extern volatile uint16_t print_counter;

void systick_init(systick_clock_source_t systick_clock_source, bool enable_interrupt);
void systick_disable_interrupt(void);
void systick_enable_interrupt(void);
void systick_print_output(void);
uint32_t systick_get_ticks(void);

#endif
