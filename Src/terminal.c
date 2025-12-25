#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal.h"

#include "uart.h"

#define ESC "\x1B"

void terminal_clear(void)
{
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

void terminal_set_text_color(Terminal_color_t color)
{
    switch (color)
    {
    case TERM_COLOR_BLACK:
        printf(ESC "[30m");
        break;
    case TERM_COLOR_RED:
        printf(ESC "[31m");
        break;
    case TERM_COLOR_GREEN:
        printf(ESC "[32m");
        break;
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
    case TERM_COLOR_WHITE:
        printf(ESC "[37m");
        break;
    case TERM_COLOR_DEFAULT:
    default:
        printf(ESC "[0m");
        break;
    }
}

void terminal_reset_text_color(void)
{
    printf(ESC "[0m");
}