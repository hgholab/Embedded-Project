/*
 * terminal.c
 *
 * Description:
 *     Provides basic terminal control utilities using ANSI escape sequences.
 *     This file enables formatted interaction with a serial terminal by
 *     offering functions to:
 *     - Clear the terminal screen
 *     - Insert new lines
 *     - Print a command prompt arrow
 *     - Change and reset terminal text colors
 *
 *     These utilities are intended for use over a serial interface
 *     (e.g., UART) where ANSI-compatible terminals are available.
 */

#include <stdio.h>

#include "terminal.h"

#define ESC "\x1B"

void terminal_clear(void)
{
        // A series of escape sequences to clear the serial terminal.
        printf(ESC "[3J" ESC "[H" ESC "[2J");
}

void terminal_insert_new_line(void)
{
        printf("\r\n");
}

void terminal_print_arrow(void)
{
        printf("> ");
}

// This function prints the appropriate escape sequence to change the color of text in the terminal.
void terminal_set_text_color(terminal_color_t color)
{
        switch (color)
        {
        case TERM_COLOR_YELLOW:
                printf(ESC "[33m");
                break;
        case TERM_COLOR_BLUE:
                printf(ESC "[34m");
                break;
        case TERM_COLOR_MAGENTA:
                printf(ESC "[35m");
                break;
        case TERM_COLOR_CYAN:
                printf(ESC "[36m");
                break;
        case TERM_COLOR_DEFAULT:
        case TERM_COLOR_WHITE:
        default:
                printf(ESC "[37m");
                break;
        }
}

void terminal_reset_text_color(void)
{
        terminal_set_text_color(TERM_COLOR_WHITE);
}