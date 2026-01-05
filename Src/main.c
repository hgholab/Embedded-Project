#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx.h"

#include "cli.h"
#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "fpu.h"
#include "gpio.h"
#include "pwm.h"
#include "scheduler.h"
#include "sine_lut.h"
#include "systick.h"
#include "terminal.h"
#include "timer.h"
#include "uart.h"

int main(void)
{
        uint32_t timer2_freq = 25UL;
        uint32_t timer3_freq = 50UL;

        // Initialization phase
        fpu_enable();
        clock_init();
        systick_init(SYSTICK_PROCESSOR_CLOCK, false);
        sine_lut_init();
        tim2_init(timer2_freq);
        pwm_tim2_init();
        tim3_init(timer3_freq);
        gpio_init();
        uart2_init();

        // Disable buffering for stdout so printf outputs immediately.
        setbuf(stdout, NULL);

        /*
         * Initialize the plant (converter) and the pid controller. Variables plant and pid are
         * defined in converter.c and controller.c, respectively.
         */
        converter_init(&plant);
        pid_init(&pid,
                 0.000362f,   // kp
                 36.281488f,  // ki
                 0.0f,        // kd
                 0.000020f,   // Ts
                 -50.000000f, // int_out_min (controller integral term minimum value)
                 50.000000f,  // int_out_max (controller integral term maximum value)
                 -60.000000f, // controller_out_min (controller output minimum value)
                 60.000000f,  // controller_out_max (controller output maximum value)
                 10.0f);      // reference

        // Initialize the CLI.
        cli_init();

        scheduler_init();
        scheduler_run(); // does not return
}