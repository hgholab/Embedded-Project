#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>

typedef enum
{
        GPIO_PORT_A = 0,
        GPIO_PORT_B,
        GPIO_PORT_C,
        GPIO_PORT_D,
        GPIO_PORT_E,
        GPIO_PORT_H
} GPIO_port_t;

typedef enum
{
        GPIO_PIN_0 = 0,
        GPIO_PIN_1,
        GPIO_PIN_2,
        GPIO_PIN_3,
        GPIO_PIN_4,
        GPIO_PIN_5,
        GPIO_PIN_6,
        GPIO_PIN_7,
        GPIO_PIN_8,
        GPIO_PIN_9,
        GPIO_PIN_10,
        GPIO_PIN_11,
        GPIO_PIN_12,
        GPIO_PIN_13,
        GPIO_PIN_14,
        GPIO_PIN_15
} GPIO_pin_t;

extern bool button_last_push_status;

void gpio_init(void);
void gpio_set_pin(GPIO_port_t port, GPIO_pin_t pin);
void gpio_clear_pin(GPIO_port_t port, GPIO_pin_t pin);
void gpio_toggle_pin(GPIO_port_t port, GPIO_pin_t pin);
bool gpio_read_pin_input(GPIO_port_t port, GPIO_pin_t pin);
bool gpio_button_is_pressed(void);

#endif
