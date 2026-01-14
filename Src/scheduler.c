#include <stddef.h>

#include "stm32f4xx.h"

#include "scheduler.h"

#include "cli.h"
#include "iwdg.h"
#include "systick.h"
#include "timer.h"

#define TASKS_NUM 4

typedef void (*task_handler)(void);

_Atomic uint32_t ready_flag_word;

static task_handler task_arr[TASKS_NUM];

void scheduler_init(void)
{
        // Tasks ordered based on their priority (index 0 has the highest priority).
        task_arr[0] = tim2_update_loop;
        task_arr[1] = cli_process_rx_byte;
        task_arr[2] = tim3_read_button;
        task_arr[3] = systick_print_output;
}

// This function implements a prioritized scheduler.
void scheduler_run(void)
{
        for (;;)
        {
                task_handler task = NULL;
                for (uint8_t p = 0U; p < TASKS_NUM; p++) // p is task priority
                {
                        __disable_irq();
                        if (ready_flag_word & (1UL << p))
                        {
                                ready_flag_word &= ~(1UL << p);
                                __enable_irq();

                                task = task_arr[p];
                                break;
                        }
                        __enable_irq();
                }
                if (task != NULL)
                {
                        (*task)();
                }
                iwdg_pet_the_dog();
        }
}