#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

void tim3_init(uint32_t timer_freq);
void tim2_init(uint32_t timer_freq);
void tim2_update_loop(void);
void tim3_read_button(void);

#endif