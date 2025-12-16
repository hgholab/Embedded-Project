#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal.h"

#include "uart.h"

#define ESC        "\x1B"

void clear_terminal(void)
{
    printf(ESC "[3J" ESC "[H" ESC "[2J");
}

void insert_new_line(void)
{
    printf("\r\n");
}

void print_arrow(void)
{
    printf("> ");
}

void set_terminal_text_color(Terminal_color_t color)
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

void reset_terminal_text_color(void)
{
    printf(ESC "[0m");
}

int read_terminal_line(char *dest, int max_length)
{
    int length = 0;

    while (1)
    {
        char c = uart2_getchar_blocking();

        if (c == '\r' || c == '\n')
        {
            printf("\r\n");
            dest[length] = '\0';
            break;
        }
        else if (c == '\b' || c == 127)
        {
            if (length > 0)
            {
                length--;
                printf("\b \b");
            }
        }
        else if (length < max_length - 1)
        {
            dest[length++] = c;
            uart2_write_char_blocking(c);
        }
    }

    return length;
}

//static void handle_terminal_line(char *line)
//{
//    char *cmd = strtok(line, " ");
//    if (!cmd) return;
//
//    if (strcmp(cmd, "freq") == 0) {
//        char *a = strtok(NULL, " ");
//        if (!a) { usart2_puts("usage: freq <Hz>\r\n"); return; }
//
//        uint32_t f = (uint32_t)strtoul(a, NULL, 10);
//        if (f == 0) { usart2_puts("freq must be > 0\r\n"); return; }
//
//        pwm_freq_hz = f;
//        pwm_apply_settings();
//        usart2_puts("OK\r\n");
//
//    } else if (strcmp(cmd, "duty") == 0) {
//        char *a = strtok(NULL, " ");
//        if (!a) { usart2_puts("usage: duty <0.0-1.0>\r\n"); return; }
//
//        float d = strtof(a, NULL);
//        if (d < 0.0f || d > 1.0f) { usart2_puts("duty 0.0..1.0\r\n"); return; }
//
//        pwm_duty = d;
//        pwm_apply_settings();
//        usart2_puts("OK\r\n");
//
//    } else if (strcmp(cmd, "pwm") == 0) {
//        char *a1 = strtok(NULL, " ");
//        char *a2 = strtok(NULL, " ");
//        if (!a1 || !a2) { usart2_puts("usage: pwm <Hz> <0.0-1.0>\r\n"); return; }
//
//        uint32_t f = (uint32_t)strtoul(a1, NULL, 10);
//        float    d = strtof(a2, NULL);
//
//        if (f == 0 || d < 0.0f || d > 1.0f) {
//            usart2_puts("invalid args\r\n");
//            return;
//        }
//
//        pwm_freq_hz = f;
//        pwm_duty    = d;
//        pwm_apply_settings();
//        usart2_puts("OK\r\n");
//
//    } else if (strcmp(cmd, "show") == 0) {
//        char buf[64];
//        snprintf(buf, sizeof(buf), "freq=%lu Hz, duty=%.3f\r\n",
//                 (unsigned long)pwm_freq_hz, (double)pwm_duty);
//        usart2_puts(buf);
//
//    } else if (strcmp(cmd, "help") == 0) {
//        usart2_puts("Commands:\r\n");
//        usart2_puts("  freq <Hz>\r\n");
//        usart2_puts("  duty <0.0-1.0>\r\n");
//        usart2_puts("  pwm <Hz> <0.0-1.0>\r\n");
//        usart2_puts("  show\r\n");
//
//    } else {
//        usart2_puts("Unknown command. Type 'help'.\r\n");
//    }
//}