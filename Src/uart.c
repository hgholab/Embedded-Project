#include "stm32f4xx.h"

#include "uart.h"

#include "cli.h"
#include "clock.h"

#define UART2_BAUDRATE 115200UL

volatile uint8_t uart_read_char;
volatile bool uart_rx_new_char_available;

static uint32_t uart2_calc_brr(const uint32_t clock_freq, const uint32_t baud_rate);

void USART2_IRQHandler(void)
{
        uart_read_char             = (uint8_t)(USART2->DR & 0xFF);
        uart_rx_new_char_available = true;
}

/*
 * Initialize USART2 for 115200 baud, 8 data bits, no parity, 1 stop bit.
 * The peripheral clock for USART2 (APB1) is 50 MHz based on clock tree.
 */
void uart2_init(void)
{
        // Enable USART2 clock on APB1 bus.
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

        // Disable USART before configuration.
        USART2->CR1 = 0;

        // Set baud rate.
        USART2->BRR = uart2_calc_brr(PCLK1, UART2_BAUDRATE);

        // Enable TX and RX.
        USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);

        // Enable RXNE interrupt.
        USART2->CR1 |= USART_CR1_RXNEIE;

        // Enable NVIC line for USART2.
        NVIC_SetPriority(USART2_IRQn, 1);
        NVIC_EnableIRQ(USART2_IRQn);

        // Enable USART2.
        USART2->CR1 |= USART_CR1_UE;
}

void uart2_write_char_blocking(char ch)
{
        // Wait until TXE flag is set.
        while (!(USART2->SR & USART_SR_TXE))
                ;

        // Write the character to the data register.
        USART2->DR = (uint8_t)ch;
}

/*
 * Calculates the USART2->BRR register value based on the
 * clock frequency of APB1 and the desired baud rate.
 */
static uint32_t uart2_calc_brr(const uint32_t clock_freq, const uint32_t baud_rate)
{
        float usartdiv    = ((float)clock_freq) / (16.0f * baud_rate);

        uint32_t mantissa = (uint32_t)usartdiv;
        uint32_t fraction = (uint32_t)((usartdiv - mantissa) * 16.0f + 0.5f);

        if (fraction == 16)
        {
                fraction = 0;
                mantissa++;
        }

        return (mantissa << 4) | (fraction & 0xFUL);
}