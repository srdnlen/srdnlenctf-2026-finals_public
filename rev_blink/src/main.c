#include "blink.h"

volatile uint32_t g_ms;
volatile uint32_t g_morse_on;

void delay_ms(uint32_t ms)
{
    uint32_t t0 = g_ms;
    while ((uint32_t)(g_ms - t0) < ms) {
        __WFI();
    }
}

void SysTick_Handler(void)
{
    g_ms++;
}

static void clock_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;                          

    RCC->CR |= RCC_CR_HSEBYP | RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0u) {
    }

    RCC->PLLCFGR = (4u   << RCC_PLLCFGR_PLLM_Pos)
                 | (180u << RCC_PLLCFGR_PLLN_Pos)
                 | (0u   << RCC_PLLCFGR_PLLP_Pos)  
                 | (8u   << RCC_PLLCFGR_PLLQ_Pos)  
                 | (2u   << RCC_PLLCFGR_PLLR_Pos)  
                 | RCC_PLLCFGR_PLLSRC_HSE;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0u) {
    }

    PWR->CR |= PWR_CR_ODEN;
    while ((PWR->CSR & PWR_CSR_ODRDY) == 0u) {
    }
    PWR->CR |= PWR_CR_ODSWEN;
    while ((PWR->CSR & PWR_CSR_ODSWRDY) == 0u) {
    }

    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN
               | FLASH_ACR_LATENCY_5WS;

    RCC->CFGR = RCC_CFGR_HPRE_DIV1     
              | RCC_CFGR_PPRE1_DIV4    
              | RCC_CFGR_PPRE2_DIV2;   

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }

    SystemCoreClockUpdate();
}

static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    MORSE_PORT->MODER = (MORSE_PORT->MODER & ~(0x3u << (MORSE_PIN * 2u)))
                      | (0x1u << (MORSE_PIN * 2u));
    morse_out(0);
}

int main(void)
{
    clock_init();
    gpio_init();
    dump_init();

    SysTick_Config(SystemCoreClock / 1000u);
    NVIC_SetPriority(SysTick_IRQn, 0);

    while (g_morse_on == 0u) {
        __WFI();
    }

    for (;;) {
        morse_send((const volatile char *)FLAG_ADDR, FLAG_MAX_LEN);
        delay_ms(MORSE_REPEAT_MS);
    }
}
