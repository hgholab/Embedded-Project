#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

enum
{
        TASK0 = (1UL << 0U), // control loop update task (TIM2)
        TASK1 = (1UL << 1U), // UART command task (UART2)
        TASK2 = (1UL << 2U), // button command task (push button, TIM3)
        TASK3 = (1UL << 3U)  // output print task (SysTick)
};

extern volatile uint32_t ready_flag_word;

void scheduler_init(void);
void scheduler_run(void);

#endif