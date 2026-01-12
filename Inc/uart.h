#ifndef UART_H
#define UART_H

#include <stdint.h>

extern volatile uint8_t uart_read_char;

void uart2_init(void);
void uart2_write_char_blocking(char ch);

#endif
