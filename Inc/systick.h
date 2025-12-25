#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    SYSTICK_EXTERNAL_CLOCK = 0,
    SYSTICK_PROCESSOR_CLOCK = 1
} Systick_clock_source_t;

void systick_init(Systick_clock_source_t systick_clock_source,
                  bool enable_interrupt);
void systick_disable_interrupt(void);
void systick_enable_interrupt(void);
void systick_delay_ms(uint32_t delay_ms);
uint32_t systick_get_ticks(void);
void systick_clear_ticks(void);

#endif
