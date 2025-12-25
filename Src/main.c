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
#include "systick.h"
#include "terminal.h"
#include "timer.h"
#include "uart.h"

int main(void)
{
        fpu_enable();
        clock_init();
        systick_init(SYSTICK_PROCESSOR_CLOCK, false);
        tim3_init(4UL);
        gpio_init();
        uart2_init();

        // Disable buffering for stdout so printf outputs immediately.
        setbuf(stdout, NULL);
        terminal_clear();

        converter_init(&plant);
        pid_init(&pid,
                 0.000362f,    // Kp
                 36.281488f,   // Ki
                 0.0f,         // Kd
                 0.000020f,    // Ts
                 -50.000000f,  // int_out_min (controller integral term minimum value)
                 50.000000f,   // int_out_max (controller integral term maximum value)
                 -100.000000f, // controller_out_min (controller output minimum value)
                 100.000000f,  // controller_out_max (controller output maximum value)
                 10.0f);       // reference

        cli_init();

        for (;;)
        {
                if (uart_rx_new_char_available)
                {
                        cli_process_rx_byte(uart_read_char);
                }
        }
}