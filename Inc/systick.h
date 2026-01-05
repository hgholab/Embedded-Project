#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdbool.h>

typedef enum
{
        SYSTICK_EXTERNAL_CLOCK  = 0,
        SYSTICK_PROCESSOR_CLOCK = 1
} systick_clock_source_t;

void systick_init(systick_clock_source_t systick_clock_source, bool enable_interrupt);
void systick_disable_interrupt(void);
void systick_enable_interrupt(void);
void systick_print_output(void);

#endif
