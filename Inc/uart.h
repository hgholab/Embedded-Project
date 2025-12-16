#ifndef UART_H
#define UART_H

#include <stdint.h>


void uart2_init(void);
void uart2_write_char_blocking(char ch);
uint8_t uart2_getchar_blocking(void);

#endif
