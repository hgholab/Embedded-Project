/*
 * gpio.c
 *
 * Description:
 *     Enables clocks for GPIO ports and configures GPIO pins.
 *     On Port A, this file configures:
 *     - PA2 as USART2_TX (AF07)
 *     - PA3 as USART2_RX (AF07)
 *     - PA5 as TIM2_CH1  (AF01)
 *     And on Port C, this file configures:
 *     - PC13 as the board push button
 */

#include <stdint.h>

#include "stm32f4xx.h"

#include "gpio.h"

// Helpers for locating and masking the 2-bit MODE field of a GPIO pin.
#define GPIO_MODER_MODE_Pos(pin) ((pin) * 2U)
#define GPIO_MODER_MODE_Msk (0x3UL)
// The following macro is used when setting mode of a GPIO pin.
#define GPIO_MODER_MODE(pin, mode) ((mode) << (GPIO_MODER_MODE_Pos(pin)))

// Helpers to locate and mask the 4-bit field of a GPIO pin alternate function.
#define GPIO_AFR_AFSEL_Pos(pin) ((pin) * 4U)
#define GPIO_AFR_AFSEL_Msk (0xFUL)
// Macro used to set alternate function mode of a GPIO pin.
#define GPIO_AFR_AFSEL(pin, func) ((func) << (GPIO_AFR_AFSEL_Pos(pin)))

// Helper for locating the 2-bit speed field of a GPIO pin.
#define GPIO_OSPEEDR_SPEED_Pos(pin) ((pin) * 2U)
// Mask for the 2-bit speed field used when configuring pin speed.
#define GPIO_OSPEEDR_SPEED_Msk (0x3UL)
// Macro used to place the speed value into the correct position in OSPEEDR.
#define GPIO_OSPEEDR_SPEED(pin, val) ((val) << GPIO_OSPEEDR_SPEED_Pos(pin))

// Helper for locating the 2-bit pull-up/pull-down field of a GPIO pin.
#define GPIO_PUPDR_PUPD_Pos(pin) ((pin) * 2U)
// Mask for the 2-bit PUPD field used when configuring pull resistors.
#define GPIO_PUPDR_PUPD_Msk (0x3UL)
// Macro to place pull-up/pull-down value into the correct position in PUPDR.
#define GPIO_PUPDR_PUPD(pin, val) ((val) << GPIO_PUPDR_PUPD_Pos(pin))

typedef enum
{
        GPIO_PIN_MODE_INPUT = 0x0UL,
        GPIO_PIN_MODE_OUTPUT = 0x1UL,
        GPIO_PIN_MODE_ALTERNATE = 0x2UL,
        GPIO_PIN_MODE_ANALOG = 0x3UL
} GPIO_pin_mode_t;

typedef enum
{
        GPIO_SPEED_LOW = 0x0UL,
        GPIO_SPEED_MEDIUM = 0x1UL,
        GPIO_SPEED_HIGH = 0x2UL,
        GPIO_SPEED_VERY_HIGH = 0x3UL
} GPIO_speed_t;

typedef enum
{
        GPIO_OTYPE_PUSH_PULL = 0x0UL,
        GPIO_OTYPE_OPEN_DRAIN = 0x1UL
} GPIO_otype_t;

typedef enum
{
        GPIO_PUPD_NONE = 0x0UL,
        GPIO_PUPD_PULL_UP = 0x1UL,
        GPIO_PUPD_PULL_DOWN = 0x2UL
} GPIO_pupd_t;

static uint32_t const gpio_port_rcc_ahb1_en[] = {RCC_AHB1ENR_GPIOAEN, RCC_AHB1ENR_GPIOBEN,
                                                 RCC_AHB1ENR_GPIOCEN, RCC_AHB1ENR_GPIODEN,
                                                 RCC_AHB1ENR_GPIOEEN, RCC_AHB1ENR_GPIOHEN};

static GPIO_TypeDef *const gpio_port_base_address[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOH};

static void gpio_enable_port_clock(GPIO_port_t port);
static void gpio_set_pin_mode(GPIO_port_t port, GPIO_pin_t pin, GPIO_pin_mode_t mode);
static void gpio_set_pin_alternate_function(GPIO_port_t port, GPIO_pin_t pin,
                                            uint32_t alternate_function_code);
static void gpio_set_pin_ospeed(GPIO_port_t port, GPIO_pin_t pin, GPIO_speed_t speed);
static void gpio_set_pin_otype(GPIO_port_t port, GPIO_pin_t pin, GPIO_otype_t type);
static void gpio_set_pin_pupd(GPIO_port_t port, GPIO_pin_t pin, GPIO_pupd_t pupd);

void gpio_init(void)
{
        // Enable clock for Port A and configure PA5 as TIM2_CH1 output.
        gpio_enable_port_clock(GPIO_PORT_A);
        gpio_set_pin_ospeed(GPIO_PORT_A, GPIO_PIN_5, GPIO_SPEED_VERY_HIGH);
        gpio_set_pin_otype(GPIO_PORT_A, GPIO_PIN_5, GPIO_OTYPE_PUSH_PULL);
        gpio_set_pin_pupd(GPIO_PORT_A, GPIO_PIN_5, GPIO_PUPD_NONE);
        gpio_set_pin_alternate_function(GPIO_PORT_A, GPIO_PIN_5, 1UL);
        gpio_set_pin_mode(GPIO_PORT_A, GPIO_PIN_5, GPIO_PIN_MODE_ALTERNATE);

        // Enable clock for Port C and configure PC13 as the board push button.
        gpio_enable_port_clock(GPIO_PORT_C);
        gpio_set_pin_mode(GPIO_PORT_C, GPIO_PIN_13, GPIO_PIN_MODE_INPUT);
        gpio_set_pin_pupd(GPIO_PORT_C, GPIO_PIN_13, GPIO_PUPD_PULL_UP);

        // Set PA2 as USART2_TX.
        gpio_set_pin_ospeed(GPIO_PORT_A, GPIO_PIN_2, GPIO_SPEED_VERY_HIGH);
        gpio_set_pin_otype(GPIO_PORT_A, GPIO_PIN_2, GPIO_OTYPE_PUSH_PULL);
        gpio_set_pin_pupd(GPIO_PORT_A, GPIO_PIN_2, GPIO_PUPD_NONE);
        gpio_set_pin_alternate_function(GPIO_PORT_A, GPIO_PIN_2, 7UL);
        gpio_set_pin_mode(GPIO_PORT_A, GPIO_PIN_2, GPIO_PIN_MODE_ALTERNATE);

        // Set PA3 as USART2_RX.
        gpio_set_pin_alternate_function(GPIO_PORT_A, GPIO_PIN_3, 7UL);
        gpio_set_pin_mode(GPIO_PORT_A, GPIO_PIN_3, GPIO_PIN_MODE_ALTERNATE);

        // Set PA8 as blue (idle mode) LED driver
        gpio_set_pin_mode(GPIO_PORT_A, GPIO_PIN_8, GPIO_PIN_MODE_OUTPUT);
        gpio_set_pin_otype(GPIO_PORT_A, GPIO_PIN_8, GPIO_OTYPE_PUSH_PULL);
        gpio_set_pin_pupd(GPIO_PORT_A, GPIO_PIN_8, GPIO_PUPD_NONE);

        // Set PB4 as yellow (config mode) LED driver
        gpio_enable_port_clock(GPIO_PORT_B);
        gpio_set_pin_mode(GPIO_PORT_B, GPIO_PIN_4, GPIO_PIN_MODE_OUTPUT);
        gpio_set_pin_otype(GPIO_PORT_B, GPIO_PIN_4, GPIO_OTYPE_PUSH_PULL);
        gpio_set_pin_pupd(GPIO_PORT_B, GPIO_PIN_4, GPIO_PUPD_NONE);

        // Set PB3 as white (mod mode) LED driver
        gpio_set_pin_mode(GPIO_PORT_B, GPIO_PIN_3, GPIO_PIN_MODE_OUTPUT);
        gpio_set_pin_otype(GPIO_PORT_B, GPIO_PIN_3, GPIO_OTYPE_PUSH_PULL);
        gpio_set_pin_pupd(GPIO_PORT_B, GPIO_PIN_3, GPIO_PUPD_NONE);
}

// atomic set: write 1 to lower 16 bits.
void gpio_set_pin(GPIO_port_t port, GPIO_pin_t pin)
{
        gpio_port_base_address[port]->BSRR = (1UL << pin);
}

// atomic clear: write 1 to upper 16 bits.
void gpio_clear_pin(GPIO_port_t port, GPIO_pin_t pin)
{
        gpio_port_base_address[port]->BSRR = (1UL << (pin + 16U));
}

void gpio_toggle_pin(GPIO_port_t port, GPIO_pin_t pin)
{
        gpio_port_base_address[port]->ODR ^= (1UL << pin);
}

bool gpio_read_pin_input(GPIO_port_t port, GPIO_pin_t pin)
{
        return (gpio_port_base_address[port]->IDR & (1UL << pin)) != 0;
}

static void gpio_enable_port_clock(GPIO_port_t port)
{
        RCC->AHB1ENR |= gpio_port_rcc_ahb1_en[port];
}

static void gpio_set_pin_mode(GPIO_port_t port, GPIO_pin_t pin, GPIO_pin_mode_t mode)
{
        gpio_port_base_address[port]->MODER &= ~(GPIO_MODER_MODE_Msk << GPIO_MODER_MODE_Pos(pin));

        gpio_port_base_address[port]->MODER |= GPIO_MODER_MODE(pin, mode);
}

static void gpio_set_pin_alternate_function(GPIO_port_t port, GPIO_pin_t pin,
                                            uint32_t alternate_function_code)
{
        /*
         * Pins 0-7 and 8-15 have different AFR registers. This if-else takes care
         * of that.
         */
        if (pin <= GPIO_PIN_7)
        {
                gpio_port_base_address[port]->AFR[0] &=
                        ~(GPIO_AFR_AFSEL_Msk << GPIO_AFR_AFSEL_Pos(pin));

                gpio_port_base_address[port]->AFR[0] |=
                        GPIO_AFR_AFSEL(pin, alternate_function_code);
        }
        else
        {
                gpio_port_base_address[port]->AFR[1] &=
                        ~(GPIO_AFR_AFSEL_Msk << GPIO_AFR_AFSEL_Pos(pin - 8));

                gpio_port_base_address[port]->AFR[1] |=
                        GPIO_AFR_AFSEL(pin - 8, alternate_function_code);
        }
}

static void gpio_set_pin_ospeed(GPIO_port_t port, GPIO_pin_t pin, GPIO_speed_t speed)
{
        gpio_port_base_address[port]->OSPEEDR &=
                ~(GPIO_OSPEEDR_SPEED_Msk << GPIO_OSPEEDR_SPEED_Pos(pin));

        gpio_port_base_address[port]->OSPEEDR |= GPIO_OSPEEDR_SPEED(pin, speed);
}

static void gpio_set_pin_otype(GPIO_port_t port, GPIO_pin_t pin, GPIO_otype_t type)
{
        if (type == GPIO_OTYPE_OPEN_DRAIN)
        {
                gpio_port_base_address[port]->OTYPER |= (1UL << pin);
        }
        else
        {
                gpio_port_base_address[port]->OTYPER &= ~(1UL << pin);
        }
}

static void gpio_set_pin_pupd(GPIO_port_t port, GPIO_pin_t pin, GPIO_pupd_t pupd)
{
        gpio_port_base_address[port]->PUPDR &= ~(GPIO_PUPDR_PUPD_Msk << GPIO_PUPDR_PUPD_Pos(pin));

        gpio_port_base_address[port]->PUPDR |= GPIO_PUPDR_PUPD(pin, pupd);
}