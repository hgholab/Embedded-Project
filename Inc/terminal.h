#ifndef TERMINAL_H
#define TERMINAL_H

typedef enum {
    TERM_COLOR_DEFAULT = 0,
    TERM_COLOR_BLACK,
    TERM_COLOR_RED,
    TERM_COLOR_GREEN,
    TERM_COLOR_YELLOW,
    TERM_COLOR_BLUE,
    TERM_COLOR_MAGENTA,
    TERM_COLOR_CYAN,
    TERM_COLOR_WHITE
} Terminal_color_t;

void clear_terminal(void);
void insert_new_line(void);
void print_arrow(void);
void set_terminal_text_color(Terminal_color_t color);
void reset_terminal_text_color(void);
int read_terminal_line(char *dest, int max_length);

#endif
