#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx.h"

#include "fpu.h"
#include "clock.h"
#include "systick.h"
#include "gpio.h"
#include "pwm.h"
#include "uart.h"

#include "terminal.h"
#include "converter.h"
#include "controller.h"
#include "cli.h"


#define CLI_BUFFER_LEN         64


//static char cli_buffer[CLI_BUFFER_LEN];

PID_controller_t controller;
Converter_model_t plant;
float u[N_INPUTS][1] = {{0.0f}};         // plant input (control signal)
float y[N_OUTPUTS][1] = {{0.0f}};        // plant output
float v_ref = 10.0f;

int main(void)
{
    fpu_enable();
    clock_init();
    gpio_init();
    uart2_init();
    systick_init(SYSTICK_PROCESSOR_CLOCK, true);

    // Disable buffering for stdout so printf outputs immediately.
    setbuf(stdout, NULL);
    // Clear terminal screen.
    uint32_t delay = 1000UL;
    while(true)
    {
        systick_delay_ms(delay);
        printf("%lu ms has passed!", delay);
        insert_new_line();
    }
//    show_cli_startup_menu();
    /*



    // Initialize converter.
    init_converter_model(&plant);
    // Initialize controller.
    init_PID(&controller,
        0.000637f,       // Kp
        63.721106f,      // Ki
        0.000000f,       // Kd
        0.000020f,       // Ts
        -10.000000f,     // int_out_min (controller integral term minimum value)
        10.000000f,      // int_out_max (controller integral term maximum value)
        -12.000000f,     // controller_out_min (controller output minimum value)
        12.000000f);     // controller_out_max (controller output maximum value)

float last_model_tick = 0;
//float last_print_tick = 0;
    // Loop forever.
    while (1)
{
    uint32_t ticks = systick_get_ticks();

    // Run model every 50 ticks
    if (ticks - last_model_tick >= 20)
    {
        last_model_tick = ticks;

        u[0][0] = update_PID(&controller, v_ref, y[0][0]);
        converter_model_step(&plant, u, y);
    }

    // Print every 500 ticks
    if (ticks >= 500)
    {
        last_model_tick = 0;
        systick_clear_ticks();
        printf("The output voltage of the model: %05.2f V\r\n", y[0][0]);
//        last_model_tick = 0;
//        last_print_tick = 0;
//        ticks = 0;
    }

}
*/
}
//    while (1)
//    {
//
//        write_UART2_string("Enter the frequency: ");
//        int len = read_terminal_line(cli_buffer, CLI_BUFFER_LEN);
//        uint32_t freq = arr_to_num(len, cli_buffer, 0, 10000000);
//
//        write_UART2_string("Enter the duty cycle: ");
//        len = read_terminal_line(cli_buffer, CLI_BUFFER_LEN);
//        uint32_t duty = arr_to_num(len, cli_buffer, 0, 100);
//
//        uint32_t arr = (APB1_TIM_CLK / freq) - 1;
//        TIM2->ARR = arr;
//        set_TIM2_duty((duty));
//
//        printf("The PWM frequency is now %lu Hz and the duty cycle has changed "
//                "to %lu.\r\n", freq, duty);
//
////        insert_new_line();
////        if (len > 0) {
////            cli_handle_line(cli_buf);
////        }
////        if (!(GPIOC->IDR & (1UL << 13)))
////            write_UART2_string("Hey! You pressed the push button.\r\n");
//    }
