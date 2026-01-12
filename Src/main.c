#include <stddef.h>
#include <stdio.h>

#include "stm32f4xx.h"

#include "cli.h"
#include "clock.h"
#include "controller.h"
#include "converter.h"
#include "fpu.h"
#include "gpio.h"
#include "iwdg.h"
#include "pwm.h"
#include "scheduler.h"
#include "systick.h"
#include "timer.h"
#include "uart.h"

int main(void)
{
        /* ---------- Start of initialization phase ---------- */
        // Initialize peripherals and utilities.
        fpu_enable();
        clock_init();
        systick_init();
        tim2_init(TIM2_FREQUENCY);
        pwm_tim2_init();
        tim3_init(TIM3_FREQUENCY);
        gpio_init();
        uart2_init();
        scheduler_init();

        // Initialize the plant (converter) and the pid controller.
        converter_init();

        // Initialize the PID controller (Coefficients are set to 0 and should be set by the user).
        pid_init(0.0f,        // kp
                 0.0f,        // ki
                 0.0f,        // kd
                 0.000020f,   // Ts
                 -50.000000f, // int_out_min (controller integral term minimum value)
                 50.000000f,  // int_out_max (controller integral term maximum value)
                 -60.000000f, // controller_out_min (controller output minimum value)
                 60.000000f); // controller_out_max (controller output maximum value)

        // Disable buffering for stdout so that printf outputs immediately.
        setbuf(stdout, NULL);

        // Initialize the CLI.
        cli_init();

        // Initialize the independent watchdog.
        iwdg_init();
        /* ---------- End of initialization phase ---------- */

        // Background loop. Run the prioritized cooperative scheduler.
        scheduler_run(); // This function never returns.
}