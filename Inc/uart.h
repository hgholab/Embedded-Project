#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

extern volatile uint8_t uart_read_char;
extern volatile bool uart_rx_new_char_available;

void uart2_init(void);
void uart2_write_char_blocking(char ch);

#endif
