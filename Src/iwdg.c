#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx.h"

#include "iwdg.h"

#include "clock.h"

#define IWDG_KEY_ENABLE           0x0000CCCCUL
#define IWDG_KEY_WR_ACCESS_ENABLE 0x00005555UL
#define IWDG_KEY_RELOAD           0x0000AAAAUL
#define IWDG_PRESCALER_64         0x00000004UL
#define IWDG_PRESCALER            64UL
#define IWDG_TIMEOUT              5.0f

static uint32_t iwdg_compute_rlr(float timeout_s, uint32_t lsi_hz, uint32_t prescaler);
static bool iwdg_is_ready(void);

void iwdg_init(void)
{
        // Enable the IWDG by writing 0x0000CCCCU in the IWDG_KR register.
        IWDG->KR = IWDG_KEY_ENABLE;

        // Enable register access by writing 0x00005555U in the IWDG_KR register.
        IWDG->KR = IWDG_KEY_WR_ACCESS_ENABLE;

        // Set the IWDG Prescaler.
        IWDG->PR = IWDG_PRESCALER_64;

        // Set the reload register (IWDG_RLR) to the largest value 0xFFFU.
        IWDG->RLR = iwdg_compute_rlr(IWDG_TIMEOUT, LSI_CLK, IWDG_PRESCALER);

        // Wait for the registers to be updated.
        while (!iwdg_is_ready())
                ;

        // Refresh the counter value by writing 0x0000AAAAUL to the key register.
        IWDG->KR = IWDG_KEY_RELOAD;
}

void iwdg_pet_the_dog(void)
{
        IWDG->KR = IWDG_KEY_RELOAD;
}

static uint32_t iwdg_compute_rlr(float timeout_s, uint32_t lsi_hz, uint32_t prescaler)
{
        float ticks  = (timeout_s * (float)lsi_hz) / (float)prescaler;
        uint32_t rlr = (uint32_t)(ticks);

        if (rlr > 0x0FFFUL)
        {
                rlr = 0x0FFFUL;
        }

        return rlr;
}

static bool iwdg_is_ready(void)
{
        return (IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) == 0;
}