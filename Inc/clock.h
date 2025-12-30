#ifndef CLOCK_H
#define CLOCK_H

// Clock values used for this project.
#define HSI_CLK 16000000UL
#define HSE_CLK 8000000UL
#define SYSCLK 100000000UL
#define HCLK 100000000UL
#define PCLK1 (HCLK / 2)
#define PCLK2 (HCLK / 1)
#define APB1_TIM_CLK (PCLK1 * 2)
#define APB2_TIM_CLK (PCLK2)

void clock_init(void);

#endif
