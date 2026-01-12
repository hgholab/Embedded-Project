#ifndef TERMINAL_H
#define TERMINAL_H

typedef enum
{
        TERM_COLOR_YELLOW,
        TERM_COLOR_BLUE,
        TERM_COLOR_MAGENTA,
        TERM_COLOR_CYAN,
        TERM_COLOR_WHITE,
        TERM_COLOR_DEFAULT,
} terminal_color_t;

void terminal_clear(void);
void terminal_insert_new_line(void);
void terminal_print_arrow(void);
void terminal_set_text_color(terminal_color_t color);
void terminal_reset_text_color(void);

#endif
