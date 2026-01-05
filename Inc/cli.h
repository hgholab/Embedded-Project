#ifndef CLI_H
#define CLI_H

#include <stdbool.h>

#include "converter.h"

extern bool cli_uart_is_in_config;
extern bool cli_button_is_disabled;
extern volatile bool cli_stream_is_on;

void cli_init(void);
void cli_process_rx_byte(void);
void cli_button_handler(void);
void cli_configure_mode_LEDs(converter_mode_t mode);
void cli_configure_text_color(converter_mode_t mode);

#endif
