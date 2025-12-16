#include <stdio.h>

#include "cli.h"

#include "uart.h"
#include "terminal.h"

#define SEPERATOR_1          "==============================================="
#define SEPERATOR_2          "  -----------------------------------------------"

static void show_cli_general_info(void);

void show_cli_startup_menu(void)
{
    clear_terminal();

    show_cli_general_info();
    insert_new_line();

    show_cli_system_status("IDLE", 1, 1, 1);
    insert_new_line();

    show_cli_available_commands();
    insert_new_line();

    show_cli_notes();
    insert_new_line();

    print_arrow();
}

static void show_cli_general_info(void)
{
    set_terminal_text_color(TERM_COLOR_CYAN);

    printf(SEPERATOR_1);
    insert_new_line();
    printf("  Nucleo-F411RE – Converter Control Interface  ");
    insert_new_line();
    printf(SEPERATOR_1);
    insert_new_line();
    insert_new_line();

    reset_terminal_text_color();

    set_terminal_text_color(TERM_COLOR_MAGENTA);

    printf("  Group Name    : Lazy Geniuses");
    insert_new_line();
    printf("  Students      : 1. Hossein Ghollabdouz,");
    insert_new_line();
    printf("                  2. Arman Golbidi");
    insert_new_line();
    printf("  Board         : NUCLEO-F411RE");

    reset_terminal_text_color();

    insert_new_line();
}

void show_cli_system_status(char *mode, float kp, float ki, float kd)
{
    set_terminal_text_color(TERM_COLOR_GREEN);

    printf("  System Status");
    insert_new_line();
    printf(SEPERATOR_2);
    insert_new_line();
    printf("  Mode          : IDLE (placeholder)");
    insert_new_line();
    printf("  Kp            : 0.50 (placeholder)");
    insert_new_line();
    printf("  Ki            : 10.00 (placeholder)");
    insert_new_line();
    printf("  Reference U*  : 12.0 V (placeholder)");
    insert_new_line();
    printf("  UI Owner      : None (buttons + UART allowed)");

    reset_terminal_text_color();

    insert_new_line();
}

void show_cli_available_commands(void)
{
    printf("  Available commands");
    insert_new_line();

    printf(SEPERATOR_2);
    insert_new_line();

    printf("  help                  - Show this help menu");
    insert_new_line();

    printf("  status                - Show current mode, Kp, Ki, reference, UI owner");
    insert_new_line();

    printf("  mode idle             - Switch to IDLE mode");
    insert_new_line();

    printf("  mode config           - Enter CONFIG mode (tune Kp, Ki)");
    insert_new_line();

    printf("  mode mod              - Enter MODULATING mode (closed-loop control)");
    insert_new_line();

    printf("  kp <value>            - Set proportional gain (CONFIG mode only)");
    insert_new_line();

    printf("  ki <value>            - Set integral gain (CONFIG mode only)");
    insert_new_line();

    printf("  kd <value>            - Set derivative gain (CONFIG mode only)");
    insert_new_line();

    printf("  ref <voltage>         - Set reference output voltage (MOD mode only)");
    insert_new_line();

    printf("  stream on             - Periodically print output voltage");
    insert_new_line();

    printf("  stream off            - Stop periodic printing");
    insert_new_line();

    printf("  exit                  - Leave CONFIG mode and release UART semaphore");
    insert_new_line();
}

void show_cli_notes(void)
{
    printf("  Notes");
    insert_new_line();

    printf(SEPERATOR_2);
    insert_new_line();

    printf("  - When UART enters CONFIG mode, buttons are disabled (semaphore taken).");
    insert_new_line();

    printf("  - After button-based changes, UART may be blocked for ~5 s.");
    insert_new_line();

    printf("  - Type 'help' at any time to reprint this summary.");
    insert_new_line();
}



