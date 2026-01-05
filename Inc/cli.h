#ifndef CLI_H
#define CLI_H

#include <stdint.h>

#include "converter.h"

typedef enum
{
        UART,
        BUTTON,
        BOTH
} ui_owner_t;

void cli_init(void);
void cli_process_rx_byte(void);
void cli_configure_mode_LEDs(mode_t mode);
void cli_configure_text_color(mode_t mode);
void cli_set_ui_owner(ui_owner_t owner);
ui_owner_t cli_get_ui_owner(void);

#endif
