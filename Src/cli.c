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
 *     - Provides runtime configuration of PID parameters (kp, ki, kd, reference)
 *     - Prints system status, menus, and help information to the terminal
 *
 *     The CLI supports:
 *         - Mode switching and system inspection
 *         - Tuning of controller parameters
 *         - Command handling with input validation
 */

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "stm32f4xx.h"

#include "cli.h"

#include "controller.h"
#include "gpio.h"
#include "pwm.h"
#include "systick.h"
#include "terminal.h"
#include "uart.h"
#include "utils.h"

#define SEPERATOR_1    "==============================================="
#define SEPERATOR_2    "  -----------------------------------------------"
#define CLI_BUFFER_LEN 64
#define MAX_ARG_NUM    2

/*
 * argv[0] points to the command string, and argv[1] points to the possible
 * argument. If argv[1] points to NULL, it means the command has no argument.
 */
typedef struct
{
        int argc;
        char *argv[MAX_ARG_NUM];
        bool excessive_args;
} command_t;
typedef int (*cli_cmd_fn)(command_t command);
typedef struct
{
        const char *name;
        cli_cmd_fn handler;
        bool has_one_arg;
} cli_command_t;

static volatile bool cli_stream_is_on = false;
static uint8_t cli_buffer[CLI_BUFFER_LEN];
static ui_owner_t ui_owner = BOTH;

static int cli_show_help_and_notes_handler(command_t command);
static int cli_execute_command(command_t command);
static void cli_show_help_and_notes(void);
static int cli_show_status_handler(command_t command);
static int cli_set_mode_handler(command_t command);
static int cli_set_type_handler(command_t command);
static int cli_stream_handler(command_t);
static int cli_set_kp_handler(command_t command);
static int cli_set_ki_handler(command_t command);
static int cli_set_kd_handler(command_t command);
static int cli_set_ref_handler(command_t command);
static int cli_exit_command_handler(command_t command);
static command_t cli_tokenize_command(uint8_t *cmd_str);
static void cli_show_startup_menu(void);
static void cli_show_system_status(
        mode_t mode, converter_type_t type, float kp, float ki, float kd, float reference);
static void cli_show_config_menu(void);

static const cli_command_t cli_command_table[] = {{"help", cli_show_help_and_notes_handler, true},
                                                  {"status", cli_show_status_handler, true},
                                                  {"mode", cli_set_mode_handler, false},
                                                  {"type", cli_set_type_handler, false},
                                                  {"stream", cli_stream_handler, true},
                                                  {"kp", cli_set_kp_handler, false},
                                                  {"ki", cli_set_ki_handler, false},
                                                  {"kd", cli_set_kd_handler, false},
                                                  {"ref", cli_set_ref_handler, false},
                                                  {"exit", cli_exit_command_handler, true}};

void cli_init(void)
{
        cli_show_startup_menu();

        // Set the mode to idle at startup.
        converter_set_mode(IDLE);
}

void cli_process_rx_byte(void)
{
        uint8_t ch = uart_read_char;

        static int cmd_line_index = 0;

        // A button press while stream is on stops the stream.
        if (cli_stream_is_on)
        {
                systick_disable_interrupt();
                cli_stream_is_on = false;
                terminal_insert_new_line();
                terminal_print_arrow();
        }
        else
        {
                if (ch == '\r' || ch == '\n')
                {
                        terminal_insert_new_line();
                        cli_buffer[cmd_line_index] = '\0';
                        command_t command          = cli_tokenize_command(cli_buffer);
                        cli_execute_command(command);
                        cmd_line_index = 0;
                }
                else if (ch == '\b')
                {
                        if (cmd_line_index != 0)
                        {
                                cmd_line_index--;
                                printf("\b \b");
                        }
                }
                else
                {
                        if (cmd_line_index < CLI_BUFFER_LEN - 1)
                        {
                                cli_buffer[cmd_line_index++] = ch;
                                printf("%c", ch);
                        }
                }
        }
}

/**
 * The following LED colors represent each mode:
 * - blue: idle
 * - yellow: config
 * - white: mod
 *
 * Also two green LEDs, one on the Nucleo board and the other on the breadboard,
 * show the PID controller output. In the basic task, this signal is the input
 * to the converter but in the bonus task 2, this signal makes the PWM for
 * H-brdige switches and LEDs. Nevertheless, even in the basic task, we have to
 * use the PID controller output to make the PWM signal for the green LEDs.
 */
void cli_configure_mode_LEDs(mode_t mode)
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

void cli_configure_text_color(mode_t mode)
{
        switch (mode)
        {
        case IDLE:
                terminal_reset_text_color();
                terminal_set_text_color(TERM_COLOR_BLUE);
                break;
        case CONFIG:
                terminal_reset_text_color();
                terminal_set_text_color(TERM_COLOR_YELLOW);
                break;

        case MOD:
                terminal_reset_text_color();
                terminal_set_text_color(TERM_COLOR_WHITE);
                break;
        }
}

void cli_set_ui_owner(ui_owner_t owner)
{
        ui_owner = owner;
}

ui_owner_t cli_get_ui_owner(void)
{
        return ui_owner;
}

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

        while (*cmd_str == ' ')
        {
                cmd_str++;
        }

        while (*cmd_str != '\0')
        {
                if (command.argc >= MAX_ARG_NUM)
                {
                        command.excessive_args = true;
                        break;
                }

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

static int cli_execute_command(command_t command)
{
        /*
         * All commands have at most two command line arguments, namely the command
         * function like mode and its possible argument like idle (mode idle).
         */
        if (command.argc == 0)
        {
                printf("  You did not enter a command! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
        else if (command.excessive_args)
        {
                printf("  Command has too many arguments! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
        else
        {
                size_t i;

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
                                if ((cli_command_table[i]).has_one_arg && command.argc != 1)
                                {
                                        printf("  The command %s does not accept any arguments! "
                                               "Try "
                                               "again.",
                                               command.argv[0]);
                                        terminal_insert_new_line();
                                        terminal_print_arrow();
                                        return -1;
                                }
                                else if (!(cli_command_table[i]).has_one_arg && command.argc == 1)
                                {
                                        printf("  This command needs an additional argument! Try "
                                               "again.");
                                        terminal_insert_new_line();
                                        terminal_print_arrow();
                                        return -1;
                                }
                                else
                                {
                                        return ((cli_command_table[i]).handler)(command);
                                }
                        }
                }

                if (i == cli_number_of_commands)
                {
                        printf("  Command not found! Try again.");
                        terminal_insert_new_line();
                        terminal_print_arrow();
                        return -1;
                }
                return -1;
        }
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
        float kp              = pid_get_kp(&pid);
        float ki              = pid_get_ki(&pid);
        float kd              = pid_get_kd(&pid);
        float ref             = pid_get_ref(&pid);
        mode_t mode           = converter_get_mode();
        converter_type_t type = converter_get_type();

        cli_show_system_status(mode, type, kp, ki, kd, ref);
        terminal_print_arrow();
        return 0;
}

static int cli_set_mode_handler(command_t command)
{
        if (strcmp("idle", command.argv[1]) == 0 || strcmp("config", command.argv[1]) == 0 ||
            strcmp("mod", command.argv[1]) == 0)
        {
                mode_t mode;

                if (strcmp("idle", command.argv[1]) == 0)
                        mode = IDLE;
                else if (strcmp("config", command.argv[1]) == 0)
                        mode = CONFIG;
                else
                        mode = MOD;

                if (converter_get_mode() != mode) // Converter mode has changed
                {
                        converter_set_mode(mode);

                        if (mode == IDLE || mode == CONFIG)
                        {
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

                                if (mode == CONFIG)
                                {
                                        cli_show_config_menu();
                                }
                        }
                        else
                        {
                                NVIC_EnableIRQ(TIM2_IRQn);
                        }

                        printf("  In %s mode. ", modes[mode]);

                        switch (mode)
                        {
                        case IDLE:
                                printf("Converter is off.");
                                break;
                        case CONFIG:
                                printf("You can configure the controller.");
                                break;
                        case MOD:
                                printf("The converter is operating.");
                                break;
                        }

                        terminal_insert_new_line();
                        terminal_print_arrow();
                        return 0;
                }

                else
                {
                        printf("  Already in %s mode! Try again.", modes[mode]);
                        terminal_insert_new_line();
                        terminal_print_arrow();
                        return -1;
                }
        }
        else
        {
                printf("The mode was not found! Still in %s mode. Try again.",
                       modes[converter_get_mode()]);
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_type_handler(command_t command)
{
        // Converter type can only be changed in config mode.
        if (converter_get_mode() != CONFIG)
        {
                printf("  Converter type can only be changed in config mode! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
        else
        {
                if (strcmp("0", command.argv[1]) == 0 || strcmp("1", command.argv[1]) == 0 ||
                    strcmp("2", command.argv[1]) == 0 || strcmp("3", command.argv[1]) == 0)
                {
                        int type_id = (int)str_to_float(command.argv[1]);
                        if (converter_get_type() != type_id)
                        {
                                converter_set_type(type_id);
                                printf("  Converter type changed to %s.", types[type_id]);
                                terminal_insert_new_line();
                                terminal_print_arrow();
                                return 0;
                        }
                        else
                        {
                                printf("  Converter type is already %s! Try again.",
                                       types[type_id]);
                                terminal_insert_new_line();
                                terminal_print_arrow();
                                return -1;
                        }
                }
                else
                {
                        printf("  The type id is invalid! Try again.");
                        terminal_insert_new_line();
                        terminal_print_arrow();
                        return -1;
                }
        }
}

static int cli_stream_handler(command_t)
{
        if (converter_get_mode() == MOD)
        {
                cli_stream_is_on = true;
                systick_enable_interrupt();
                return 0;
        }
        else
        {
                printf("Stream cannot be turned on in %s mode! Try again.",
                       modes[converter_get_mode()]);
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_kp_handler(command_t command)
{
        if (converter_get_mode() == CONFIG)
        {
                pid_set_kp(&pid, str_to_float(command.argv[1]));
                terminal_insert_new_line();
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You can modify kp only in config mode! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_ki_handler(command_t command)
{
        if (converter_get_mode() == CONFIG)
        {
                pid_set_ki(&pid, str_to_float(command.argv[1]));
                terminal_insert_new_line();
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You can modify ki only in config mode! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_kd_handler(command_t command)
{
        if (converter_get_mode() == CONFIG)
        {
                pid_set_kd(&pid, str_to_float(command.argv[1]));
                terminal_insert_new_line();
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You can modify kd only in config mode! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

static int cli_set_ref_handler(command_t command)
{
        mode_t current_mode = converter_get_mode();

        // Reference can only be changed in config and mod modes.
        if (current_mode == CONFIG || converter_get_mode() == MOD)
        {
                float ref = str_to_float(command.argv[1]);

                if (ref > REF_MAX)
                {
                        printf("The reference cannot be higher than %05.2f and is now "
                               "%05.2f.",
                               REF_MAX,
                               REF_MAX);
                }
                else if (ref < -1.0f * REF_MAX)
                {
                        printf("The reference cannot be lower than %05.2f and is now "
                               "%05.2f.",
                               -1.0f * REF_MAX,
                               -1.0f * REF_MAX);
                }

                // The reference value is then limited between -REF_MAX and REF_MAX.
                ref = CLAMP(ref, -1.0f * REF_MAX, REF_MAX);
                pid_set_ref(&pid, ref);
                terminal_insert_new_line();
                terminal_print_arrow();
                return 0;
        }
        else
        {
                printf("You cannot modify ref in idle mode! Try again.");
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

        if (converter_get_mode() == CONFIG)
        {
                command_t dummy_command = {.argc = 2, .argv = {"mode", "idle"}};
                cli_set_mode_handler(dummy_command);
                return 0;
        }
        else
        {
                printf("The \"exit\" command only works in config mode! Try again.");
                terminal_insert_new_line();
                terminal_print_arrow();
                return -1;
        }
}

/* ==================== CLI Printing Functions ==================== */
static void cli_show_startup_menu(void)
{
        terminal_clear();

        terminal_reset_text_color();
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
        printf("                  3. Ian Chirchir, and ");
        terminal_insert_new_line();
        printf("                  4. Mike Komidiera");
        terminal_insert_new_line();
        printf("  Board         : NUCLEO-F411RE");

        terminal_reset_text_color();
        terminal_set_text_color(TERM_COLOR_BLUE);

        terminal_insert_new_line();
        terminal_insert_new_line();

        cli_show_system_status(converter_get_mode(),
                               converter_get_type(),
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

static void cli_show_system_status(
        mode_t mode, converter_type_t type, float kp, float ki, float kd, float reference)
{
        printf("  System Status");
        terminal_insert_new_line();
        printf(SEPERATOR_2);
        terminal_insert_new_line();
        printf("  type          : %s", types[type]);
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

        terminal_insert_new_line();
}

static void cli_show_config_menu(void)
{
        printf("  Available commands in this mode");
        terminal_insert_new_line();
        printf(SEPERATOR_2);
        terminal_insert_new_line();
        printf("  type <type_id>        - Set converter model type");
        terminal_insert_new_line();
        printf("  kp <value>            - Set proportional gain");
        terminal_insert_new_line();
        printf("  ki <value>            - Set integral gain");
        terminal_insert_new_line();
        printf("  kd <value>            - Set derivative gain");
        terminal_insert_new_line();
        printf("  ref <value>           - Set reference value");
        terminal_insert_new_line();
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
        printf("  type <type_id>        - Switch to the converter type with selected id:");
        terminal_insert_new_line();
        printf("                        - 0: DC-DC Ideal Bridge");
        terminal_insert_new_line();
        printf("                        - 1: Inverter Ideal Bridge");
        terminal_insert_new_line();
        printf("                        - 2: DC-DC H-Bridge, and");
        terminal_insert_new_line();
        printf("                        - 3: Inverter H-Bridge");
        terminal_insert_new_line();
        printf("  mode idle             - Switch to idle mode");
        terminal_insert_new_line();
        printf("  mode config           - Enter config mode (tune Kp, Ki, kd, and ref)");
        terminal_insert_new_line();
        printf("  mode mod              - Enter mod mode (converter in operation) printing "
               "output "
               "voltage periodically");
        terminal_insert_new_line();
        printf("  kp <value>            - Set proportional gain (config mode only)");
        terminal_insert_new_line();
        printf("  ki <value>            - Set integral gain (config mode only)");
        terminal_insert_new_line();
        printf("  kd <value>            - Set derivative gain (config mode only)");
        terminal_insert_new_line();
        printf("  ref <voltage>         - Set reference voltage (config and mod mode "
               "only)");
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
        printf("  - While CLI is printing the output voltage, press any key to stop the "
               "stream and "
               "enter a new command.");
        terminal_insert_new_line();
        printf("  - When UART enters CONFIG mode, button is disabled (semaphore taken).");
        terminal_insert_new_line();
        printf("  - After entering config mode by button, uart cannot change the mode for "
               "5 "
               "seconds.");
        terminal_insert_new_line();
        printf("  - Type \"help\" at any time to reprint this summary.");
        terminal_insert_new_line();
}