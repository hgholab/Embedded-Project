/*
 * cli.c
 *
 * Description:
 *     Implements a UART-based Command Line Interface (CLI) for interacting
 *     with the converter control system.
 *
 *     This module:
 *     - Receives and buffers user input via UART
 *     - Tokenizes and validates CLI commands
 *     - Runs commands using a lookup table
 *     - Manages system operating modes (IDLE, CONFIG, MOD)
 *     - Provides runtime configuration of PID parameters (Kp, Ki, Kd, reference)
 *     - Prints system status, menus, and help information to the terminal
 *
 *     The CLI supports:
 *         - Mode switching and system inspection
 *         - Tuning of controller parameters
 *         - Safe command handling with input validation
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f4xx.h"

#include "cli.h"

#include "controller.h"
#include "converter.h"
#include "gpio.h"
#include "systick.h"
#include "terminal.h"
#include "uart.h"
#include "utils.h"

#define SEPERATOR_1    "==============================================="
#define SEPERATOR_2    "  -----------------------------------------------"
#define CLI_BUFFER_LEN 32
#define MAX_ARG_NUM    2
#define MODES_NUM      3

typedef enum
{
        IDLE,
        CONFIG,
        MOD
} mode_t;

typedef struct
{
        int argc;
        char *argv[MAX_ARG_NUM];
} command_t;

/*
 * argv[0] points to the command string, and argv[1] points to the possible
 *argument. If argv[1] points to NULL, it means the command has no argument.
 */
typedef int (*cli_cmd_fn)(command_t command);

typedef struct
{
        const char *name;
        cli_cmd_fn handler;
} cli_command_t;

volatile bool cli_stream_is_on = false;

static uint8_t cli_buffer[CLI_BUFFER_LEN];

/**
 * The following LED colors represent each mode:
 * - blue: idle
 * - yellow: config
 * - white: mod
 *
 * Also two green LEDs, one on the Nucleo board and the other on the breadboard,
 * show the PID controller output. In the basic task, this signal is the input
 * to the converter but in the bonus task 2, this signal makes the PWM for
 * H-brdige switches and LEDs. Although, even in the basic task, we have to
 * use the PID controller output to make the PWM signal for the green LEDs.
 */
static const char *modes[MODES_NUM] = {"idle", "config", "mod"};
static mode_t current_mode          = IDLE;

static int cli_show_help_and_notes_handler(command_t command);
static int cli_execute_command(command_t command);
static void cli_show_help_and_notes(void);
static int cli_show_status_handler(command_t command);
static int cli_set_mode_handler(command_t command);
static int cli_stream_handler(command_t);
static int cli_set_kp_handler(command_t command);
static int cli_set_ki_handler(command_t command);
static int cli_set_kd_handler(command_t command);
static int cli_set_ref_handler(command_t command);
static int cli_exit_command_handler(command_t command);
static void cli_set_mode(mode_t mode);
static mode_t cli_get_mode(void);
static void cli_configure_mode_LEDs(mode_t mode);
static command_t cli_tokenize_command(uint8_t *cmd_str);
void cli_show_startup_menu(void);
void cli_show_system_status(mode_t mode, float kp, float ki, float kd, float reference);
void cli_show_config_menu(void);

static const cli_command_t cli_command_table[] = {{"help", cli_show_help_and_notes_handler},
                                                  {"status", cli_show_status_handler},
                                                  {"mode", cli_set_mode_handler},
                                                  {"stream", cli_stream_handler},
                                                  {"kp", cli_set_kp_handler},
                                                  {"ki", cli_set_ki_handler},
                                                  {"kd", cli_set_kd_handler},
                                                  {"ref", cli_set_ref_handler},
                                                  {"exit", cli_exit_command_handler}};

void cli_init(void)
{
        cli_show_startup_menu();

        // Turn on blue LED to show that the mode is idle at startup.
        cli_configure_mode_LEDs(IDLE);
}

static int cli_execute_command(command_t command)
{
        /*
         * All commands have at most two command line arguments, namely the command
         * function like mode and its possible argument like idle (mode idle).
         */
        if (command.argc == 0)
        {
                printf("You did not enter a command! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
        else if (command.argc >= 3)
        {
                printf("Command has too many arguments! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
        else
        {
                int i;

                // Convert the command to lower case before processing it.
                for (i = 0; i < command.argc; i++)
                {
                        str_to_lower(command.argv[i]);
                }

                int cli_number_of_commands = ARRAY_LEN(cli_command_table);
                for (i = 0; i < cli_number_of_commands; i++)
                {
                        if (strcmp(command.argv[0], (cli_command_table[i]).name) == 0)
                        {
                                return ((cli_command_table[i]).handler)(command);
                        }
                }

                if (i == cli_number_of_commands)
                {
                        printf("Command not found! Try again.");
                        terminal_insert_new_line();
                        terminal_print_arrow();
                        return -1;
                }
        }
        return -1;
}

/* ==================== CLI Command Handler Functions ==================== */
static int cli_show_help_and_notes_handler(command_t command)
{
        cli_show_help_and_notes();
        terminal_insert_new_line();
        terminal_print_arrow();

        return 0;
}

static int cli_show_status_handler(command_t command)
{
        float kp    = pid_get_kp(&pid);
        float ki    = pid_get_ki(&pid);
        float kd    = pid_get_kd(&pid);
        float ref   = pid_get_ref(&pid);
        mode_t mode = cli_get_mode();

        cli_show_system_status(mode, kp, ki, kd, ref);

        terminal_print_arrow();

        return 0;
}

static int cli_set_mode_handler(command_t command)
{
        if (command.argc < 2)
        {
                printf("No mode was specified! The correct command to change the mode is of the "
                       "form \"mode <value>\". Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }

        for (int i = 0; i < MODES_NUM; i++)
        {
                if (strcmp(modes[i], command.argv[1]) == 0)
                {
                        // i == (0: idle mode, 1: config mode, 2: mod mode)
                        if (i == 0)
                        {
                                /*
                                 * In idle and config mode, we put the controller output
                                 * to 0, put the plant state vector to 0, we disable the systick
                                 * interrupt which is responsible for updating the control loop
                                 * and plant state vector, and finally, we clear the pid
                                 * accumulative integral term.
                                 */
                                if (cli_get_mode() != IDLE)
                                {
                                        cli_set_mode(IDLE);

                                        /*
                                         * Stop updating the control loop, and converter state
                                         * variables by disabling timer 2 interrupt.
                                         */
                                        NVIC_DisableIRQ(TIM2_IRQn);

                                        // Clear PID controller integral accumulative term.
                                        pid_clear_integrator(&pid);

                                        // Set plant's input and output to 0
                                        u[0][0] = 0.0f;
                                        y[0][0] = 0.0f;
                                        // Set state vector to 0 by reseting converter state vector.
                                        converter_init(&plant);

                                        printf("In idle mode. The Converter is off.");
                                        terminal_insert_new_line();
                                        terminal_print_arrow();
                                        return 0;
                                }
                                else
                                {
                                        printf("Already in idle mode.");
                                        terminal_insert_new_line();
                                        terminal_print_arrow();
                                        return 0;
                                }
                        }
                        else if (i == 1)
                        {
                                if (cli_get_mode() != CONFIG)
                                {
                                        cli_set_mode(CONFIG);

                                        /*
                                         * Stop updating the control loop, and converter state
                                         * variables by disabling timer 2 interrupt.
                                         */
                                        NVIC_DisableIRQ(TIM2_IRQn);

                                        // Clear PID controller integral accumulative term.
                                        pid_clear_integrator(&pid);

                                        // Set plant's input and output to 0
                                        u[0][0] = 0.0f;
                                        y[0][0] = 0.0f;
                                        // Set state vector to 0 by reseting converter state vector.
                                        converter_init(&plant);

                                        terminal_set_text_color(TERM_COLOR_YELLOW);
                                        printf("In configuration mode. You can configure the "
                                               "controller.");
                                        terminal_reset_text_color();

                                        terminal_insert_new_line();
                                        cli_show_config_menu();
                                        terminal_print_arrow();
                                        return 0;
                                }
                        }
                        else
                        {
                                if (cli_get_mode() != MOD)
                                {
                                        cli_set_mode(MOD);
                                        /*
                                         * Start updating the control loop, and converter state
                                         * variables by enabling timer 2 interrupt.
                                         */
                                        NVIC_EnableIRQ(TIM2_IRQn);

                                        printf("In modulation mode. The converter is operating.");
                                        terminal_insert_new_line();
                                        terminal_print_arrow();
                                        return 0;
                                }
                        }
                }
        }

        printf("The mode was not found! Try again.");
        terminal_insert_new_line();
        terminal_print_arrow();
        return -1;
}

static int cli_stream_handler(command_t)
{
        systick_enable_interrupt();
        cli_stream_is_on = true;
        return 0;
}

static int cli_set_kp_handler(command_t command)
{
        if (command.argc < 2)
        {
                printf("No value was specified! The correct command to change kp "
                       "is of the form \"kp <value>\". Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }

        if (cli_get_mode() == CONFIG)
        {
                pid_set_kp(&pid, str_to_float(command.argv[1]));
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You can modify kp only in config mode!");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_ki_handler(command_t command)
{
        if (command.argc < 2)
        {
                printf("No value was specified! The correct command to change ki "
                       "is of the form \"ki <value>\". Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }

        if (cli_get_mode() == CONFIG)
        {
                pid_set_ki(&pid, str_to_float(command.argv[1]));
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You can modify ki only in config mode!");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_kd_handler(command_t command)
{
        if (command.argc < 2)
        {
                printf("No value was specified! The correct command to change kd "
                       "is of the form \"kd <value>\". Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }

        if (cli_get_mode() == CONFIG)
        {
                pid_set_kd(&pid, str_to_float(command.argv[1]));
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You can modify kd only in config mode!");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_ref_handler(command_t command)
{
        if (command.argc < 2)
        {
                printf("No value was specified! The correct command to change ref "
                       "is of the form \"ref <value>\". Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }

        if (cli_get_mode() == CONFIG || cli_get_mode() == MOD)
        {
                float ref = str_to_float(command.argv[1]);

                if (ref > REF_MAX)
                {
                        printf("The reference cannot be higher than %05.2f and is now %05.2f.",
                               REF_MAX,
                               REF_MAX);
                        terminal_insert_new_line();
                }
                else if (ref < -1.0f * REF_MAX)
                {
                        printf("The reference cannot be lower than %05.2f and is now %05.2f.",
                               -1.0f * REF_MAX,
                               -1.0f * REF_MAX);
                        terminal_insert_new_line();
                }

                // The reference value is then limited between -50 and 50.
                ref = CLAMP(ref, -1.0f * REF_MAX, REF_MAX);
                pid_set_ref(&pid, ref);
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You cannot modify ref in idle mode!");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_exit_command_handler(command_t command)
{
        /*
         * The exit command only works for config mode.
         * In the config mode, the command make cli exit config mode,
         * and returns the cli back to the main menu in idle mode.
         */

        if (cli_get_mode() == CONFIG)
        {
                command_t dummy_command = {.argc = 2, .argv = {"mode", "idle"}};
                cli_set_mode_handler(dummy_command);
        }
        else
        {
                printf("The \"exit\" command only works in config mode! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
        }

        return 0;
}

static void cli_set_mode(mode_t mode)
{
        current_mode = mode;

        // Confgiure mode LEDs
        cli_configure_mode_LEDs(mode);
}

static mode_t cli_get_mode(void)
{
        return current_mode;
}

static void cli_configure_mode_LEDs(mode_t mode)
{
        switch (mode)
        {
        case IDLE:
                gpio_set_pin(GPIO_PORT_B, GPIO_PIN_3);
                gpio_clear_pin(GPIO_PORT_A, GPIO_PIN_8);
                gpio_clear_pin(GPIO_PORT_B, GPIO_PIN_4);
                break;
        case CONFIG:
                gpio_set_pin(GPIO_PORT_B, GPIO_PIN_4);
                gpio_clear_pin(GPIO_PORT_A, GPIO_PIN_8);
                gpio_clear_pin(GPIO_PORT_B, GPIO_PIN_3);
                break;

        case MOD:
                gpio_set_pin(GPIO_PORT_A, GPIO_PIN_8);
                gpio_clear_pin(GPIO_PORT_B, GPIO_PIN_4);
                gpio_clear_pin(GPIO_PORT_B, GPIO_PIN_3);
                break;
        }
}

/* ==================== CLI Helper Functions ==================== */
/*
 * Tokenizes a user-entered CLI command string.
 *
 * - The input string is parsed into whitespace-delimited tokens.
 * - The first token identifies the command to be executed.
 * - The possible second token represents the command argument.
 *
 * Note: Leading spaces in the input string are ignored.
 */
static command_t cli_tokenize_command(uint8_t *cmd_str)
{
        command_t command = {0};

        // Skip leading spaces of the command.
        while (*cmd_str == ' ')
        {
                cmd_str++;
        }

        while (*cmd_str != '\0' && command.argc < MAX_ARG_NUM)
        {
                command.argv[command.argc++] = (char *)cmd_str;
                while (*cmd_str != '\0' && *cmd_str != ' ')
                {
                        cmd_str++;
                }
                if (*cmd_str == ' ')
                {
                        *cmd_str = '\0';
                        cmd_str++;
                        while (*cmd_str == ' ')
                        {
                                cmd_str++;
                        }
                }
        }
        return command;
}

/* ==================== Line Processing Functions ==================== */
void cli_process_rx_byte(uint8_t ch)
{
        static int cmd_line_current_index = 0;

        if (uart_rx_new_char_available)
        {
                if (cli_stream_is_on)
                {
                        systick_disable_interrupt();
                        cli_stream_is_on = false;
                        terminal_insert_new_line();
                        terminal_print_arrow();
                }
                else if (ch == '\r' || ch == '\n')
                {
                        terminal_insert_new_line();
                        cli_buffer[cmd_line_current_index] = '\0';
                        command_t command                  = cli_tokenize_command(cli_buffer);
                        cli_execute_command(command);
                        cmd_line_current_index = 0;
                }
                else if (ch == '\b')
                {
                        if (cmd_line_current_index != 0)
                        {
                                cmd_line_current_index--;
                                printf("\b \b");
                        }
                }
                else
                {
                        if (cmd_line_current_index < CLI_BUFFER_LEN - 1)
                        {
                                cli_buffer[cmd_line_current_index++] = ch;
                                printf("%c", ch);
                        }
                }
                uart_rx_new_char_available = false;
        }
}

/* ==================== CLI Printing Functions ==================== */
void cli_show_startup_menu(void)
{
        terminal_clear();

        terminal_set_text_color(TERM_COLOR_CYAN);

        printf(SEPERATOR_1);
        terminal_insert_new_line();
        printf("  Nucleo-F411RE - Converter Control Interface  ");
        terminal_insert_new_line();
        printf(SEPERATOR_1);
        terminal_insert_new_line();
        terminal_insert_new_line();
        terminal_reset_text_color();

        terminal_set_text_color(TERM_COLOR_MAGENTA);

        printf("  Group Name    : Lazy Geniuses");
        terminal_insert_new_line();
        printf("  Students      : 1. Arman Golbidi,");
        terminal_insert_new_line();
        printf("                  2. Hossein Ghollabdouz,");
        terminal_insert_new_line();
        printf("                  3. Ian Chirchir,");
        terminal_insert_new_line();
        printf("                  4. Mike Komidiera");
        terminal_insert_new_line();
        printf("  Board         : NUCLEO-F411RE");

        terminal_reset_text_color();

        terminal_insert_new_line();
        terminal_insert_new_line();

        cli_show_system_status(current_mode,
                               pid_get_kp(&pid),
                               pid_get_ki(&pid),
                               pid_get_kd(&pid),
                               pid_get_ref(&pid));

        terminal_insert_new_line();

        cli_show_help_and_notes();

        terminal_insert_new_line();
        terminal_insert_new_line();

        terminal_print_arrow();
}

void cli_show_system_status(mode_t mode, float kp, float ki, float kd, float reference)
{
        terminal_set_text_color(TERM_COLOR_GREEN);

        printf("  System Status");
        terminal_insert_new_line();
        printf(SEPERATOR_2);
        terminal_insert_new_line();
        printf("  mode          : %s", modes[mode]);
        terminal_insert_new_line();
        printf("  kp            : %-11.6f", kp);
        terminal_insert_new_line();
        printf("  ki            : %-11.6f", ki);
        terminal_insert_new_line();
        printf("  kd            : %-11.6f", kd);
        terminal_insert_new_line();
        printf("  reference     : %-11.6f", reference);

        terminal_reset_text_color();

        terminal_insert_new_line();
}

void cli_show_config_menu(void)
{
        terminal_set_text_color(TERM_COLOR_YELLOW);

        printf("  Available commands in this mode");
        terminal_insert_new_line();
        printf(SEPERATOR_2);
        terminal_insert_new_line();
        printf("  kp <value>            - Set proportional gain");
        terminal_insert_new_line();
        printf("  ki <value>            - Set integral gain");
        terminal_insert_new_line();
        printf("  kd <value>            - Set derivative gain");
        terminal_insert_new_line();
        printf("  ref <value>           - Set reference value");
        terminal_insert_new_line();

        terminal_reset_text_color();
}

static void cli_show_help_and_notes(void)
{
        printf("  Available commands");
        terminal_insert_new_line();
        printf(SEPERATOR_2);
        terminal_insert_new_line();
        printf("  help                  - Show this help menu");
        terminal_insert_new_line();
        printf("  status                - Show current mode, kp, ki, kd, and ref");
        terminal_insert_new_line();
        printf("  mode idle             - Switch to idle mode");
        terminal_insert_new_line();
        printf("  mode config           - Enter config mode (tune Kp, Ki, kd, and ref)");
        terminal_insert_new_line();
        printf("  mode mod              - Enter mod mode (converter in operation) printing output "
               "voltage periodically");
        terminal_insert_new_line();
        printf("  kp <value>            - Set proportional gain (config mode only)");
        terminal_insert_new_line();
        printf("  ki <value>            - Set integral gain (config mode only)");
        terminal_insert_new_line();
        printf("  kd <value>            - Set derivative gain (config mode only)");
        terminal_insert_new_line();
        printf("  ref <voltage>         - Set reference voltage (config and mod mode only)");
        terminal_insert_new_line();
        printf("  stream                - Periodically print output voltage");
        terminal_insert_new_line();
        printf("  exit                  - Leave config mode and release uart semaphore");
        terminal_insert_new_line();
        terminal_insert_new_line();
        printf("  Notes");
        terminal_insert_new_line();
        printf(SEPERATOR_2);
        terminal_insert_new_line();
        printf("  - While CLI is printing the output voltage, press any key to stop the stream and "
               "enter a new command.");
        terminal_insert_new_line();
        printf("  - When UART enters CONFIG mode, button is disabled (semaphore taken).");
        terminal_insert_new_line();
        printf("  - After entering config mode by button, uart cannot change the mode for 5 "
               "seconds.");
        terminal_insert_new_line();
        printf("  - Type \"help\" at any time to reprint this summary.");
        terminal_insert_new_line();
}