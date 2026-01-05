/*
 * clocks.c
 *
 * Description:
 *     Provides full system clock initialization for the board.
 *
 *     This module:
 *     - Enables the external high-speed clock (HSE) in bypass mode
 *     - Configures Flash wait states and enables caches
 *     - Sets AHB, APB1, and APB2 prescalers
 *     - Configures the main PLL using HSE as the clock source
 *     - Enables the PLL and switches SYSCLK to the PLL output
 *
 *     The final system clock configuration is:
 *         SYSCLK = 100 MHz (from PLL)
 *         HCLK   = 100 MHz
 *         PCLK1  = 50  MHz
 *         PCLK2  = 100 MHz
 */

// stddef.h is used to access size_t type definition
#include <stddef.h>
#include <stdint.h>

#include "stm32f4xx.h"

#include "clock.h"

#define WAIT_STATE 3
#define PLLM_VALUE 4UL
#define PLLN_VALUE 100UL
#define PLLP_VALUE 2UL

static const uint32_t flash_acr_latency_ws[] = {
        FLASH_ACR_LATENCY_0WS,
        FLASH_ACR_LATENCY_1WS,
        FLASH_ACR_LATENCY_2WS,
        FLASH_ACR_LATENCY_3WS,
        FLASH_ACR_LATENCY_4WS,
        FLASH_ACR_LATENCY_5WS,
        FLASH_ACR_LATENCY_6WS,
        FLASH_ACR_LATENCY_7WS,
};

static void clock_enable_HSE(void);
static void clock_configure_flash_wait_states(size_t wait_state);
static void clock_configure_prescalers(void);
static void clock_configure_PLL(void);
static void clock_switch_SYSCLK_to_PLL(void);

/*
 * Complete system clock initialization.
 *
 * - Enable HSE in bypass mode (8 MHz from ST-LINK MCO)
 * - Configure Flash wait states and caches
 * - Set AHB and APB prescalers
 * - Configure and enable PLL (100 MHz SYSCLK)
 * - Switch SYSCLK to PLL
 */
void clock_init(void)
{
        clock_enable_HSE();
        clock_configure_flash_wait_states(WAIT_STATE);
        clock_configure_prescalers();
        clock_configure_PLL();
        clock_switch_SYSCLK_to_PLL();
}

/*
 * Enable the High-Speed External (HSE) clock in bypass mode.
 *
 * On the Nucleo board, the ST-LINK debugger provides a fixed 8 MHz clock
 * to the OSC_IN pin (PH0). There is no crystal connected, and OSC_OUT
 * (PH1) is left floating. Therefore, HSE must be enabled in bypass mode
 * to accept the externally driven clock signal.
 */
static void clock_enable_HSE(void)
{
        // Select bypass mode since OSC_IN is driven by an external clock.
        RCC->CR |= RCC_CR_HSEBYP;

        // Enable the external high-speed clock.
        RCC->CR |= RCC_CR_HSEON;

        // Wait until the HSE clock becomes ready.
        while (!(RCC->CR & RCC_CR_HSERDY))
                ;
}

/*
 * Configure Flash wait states for 100 MHz operation.
 *
 * At 3.3 V and SYSCLK = 100 MHz, the Flash requires 3 WS (wait states) for
 * reliable read access.
 */
static void clock_configure_flash_wait_states(size_t wait_state)
{
        FLASH->ACR &= ~(FLASH_ACR_LATENCY_Msk);
        FLASH->ACR |= flash_acr_latency_ws[wait_state];

        // Enable Flash pre-fetch to reduce wait-state penalties.
        FLASH->ACR |= FLASH_ACR_PRFTEN;

        // Enable instruction cache for faster code fetches.
        FLASH->ACR |= FLASH_ACR_ICEN;

        // Enable data cache for faster Flash data accesses.
        FLASH->ACR |= FLASH_ACR_DCEN;
}

/*
 * Configure AHB, APB1, and APB2 prescalers for 100 MHz operation.
 *
 * - AHB  (HCLK)  = SYSCLK / 1 = 100 MHz
 * - APB1 (PCLK1) = HCLK   / 2 = 50  MHz (max allowed on STM32F411RE)
 * - APB2 (PCLK2) = HCLK   / 1 = 100 MHz
 */
static void clock_configure_prescalers(void)
{
        /*
         * AHB prescaler: HCLK = SYSCLK / 1
         * Clear HPRE bits [7:4] : no division.
         */
        RCC->CFGR &= ~RCC_CFGR_HPRE;
        RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

        /*
         * APB1 prescaler: PCLK1 = HCLK / 2
         * Clear PPRE1 bits [12:10], then set divide-by-2.
         */
        RCC->CFGR &= ~RCC_CFGR_PPRE1;
        RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

        /*
         * APB2 prescaler: PCLK2 = HCLK / 1
         * Clear PPRE2 bits [15:13] : no division.
         */
        RCC->CFGR &= ~RCC_CFGR_PPRE2;
        RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
}

/*
 * Configure PLL to generate a 100 MHz system clock from HSE (8 MHz).
 *
 * PLL settings:
 * - PLLM = 4   : VCO_in  = 2 MHz   (must be 1–2 MHz)
 * - PLLN = 100 : VCO_out = 200 MHz (must be 100–432 MHz)
 * - PLLP = 2   : SYSCLK  = 100 MHz
 */
static void clock_configure_PLL(void)
{
        // Disable PLL before configuration.
        RCC->CR &= ~RCC_CR_PLLON;
        while (RCC->CR & RCC_CR_PLLRDY)
                ;

        // Start from current value to preserve reserved bits.
        uint32_t pll_cfgr = RCC->PLLCFGR;

        pll_cfgr &= ~(RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk |
                      RCC_PLLCFGR_PLLSRC_Msk);

        // Select HSE as PLL source.
        pll_cfgr |= RCC_PLLCFGR_PLLSRC_HSE;

        pll_cfgr |= (PLLM_VALUE << 0U);

        pll_cfgr |= (PLLN_VALUE << 6U);

        // Set PLLP (bits 17:16), encoded as (PLLP/2 - 1)
        pll_cfgr |= (((PLLP_VALUE / 2U) - 1U) << 16U);

        // Write back the configuration (reserved bits preserved).
        RCC->PLLCFGR = pll_cfgr;

        // Enable PLL.
        RCC->CR |= RCC_CR_PLLON;

        // Wait until PLL is locked and ready.
        while (!(RCC->CR & RCC_CR_PLLRDY))
                ;
}

/*
 * Switch the system clock source to PLL.
 *
 * SW  = 0b10: PLL selected as system clock
 * SWS = 0b10: PLL is the active system clock
 */
static void clock_switch_SYSCLK_to_PLL(void)
{
        RCC->CFGR &= ~RCC_CFGR_SW_Msk;

        // Select PLL as system clock.
        RCC->CFGR |= RCC_CFGR_SW_PLL;

        // Wait until PLL is actually used as system clock.
        while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL)
                ;
}