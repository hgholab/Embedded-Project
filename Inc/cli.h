#ifndef CLI_H
#define CLI_H

#include <stdint.h>

void cli_init(void);
void cli_process_rx_byte(uint8_t ch);

#endif
