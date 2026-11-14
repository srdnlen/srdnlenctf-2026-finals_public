#include "blink.h"

#define FRAME_WORDS ((FRAME_HDR_BYTES + DUMP_BLOCK_SIZE) / 4u)

static union {
    uint32_t w[FRAME_WORDS];
    uint8_t  b[FRAME_HDR_BYTES + DUMP_BLOCK_SIZE];
} frame;

static volatile uint32_t press_ts[TRIGGER_PRESSES];
static volatile uint32_t press_count;
static volatile uint32_t last_edge_ms;
static volatile uint32_t btn_down;
static volatile uint32_t btn_down_ms;

void dump_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_USART1EN;

    BTN_PORT->MODER = BTN_PORT->MODER & ~(0x3u << (BTN_PIN * 2u));
    BTN_PORT->PUPDR = BTN_PORT->PUPDR & ~(0x3u << (BTN_PIN * 2u));

    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL6)
                  | (7u << GPIO_AFRL_AFSEL6_Pos);
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~GPIO_OSPEEDR_OSPEED6)
                   | GPIO_OSPEEDR_OSPEED6_1;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER6)
                 | GPIO_MODER_MODER6_1;

    uint32_t pclk2 = SystemCoreClock
                   >> APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos];
    USART1->BRR = (uint16_t)(pclk2 / DUMP_BAUD);
    USART1->CR2 = 0u;                               
    USART1->CR3 = 0u;                               
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;      

    SYSCFG->EXTICR[3] = (SYSCFG->EXTICR[3] & ~SYSCFG_EXTICR4_EXTI13)
                      | SYSCFG_EXTICR4_EXTI13_PC;

    EXTI->FTSR |= EXTI_FTSR_TR13;
    EXTI->RTSR |= EXTI_RTSR_TR13;
    EXTI->PR    = EXTI_PR_PR13;
    EXTI->IMR  |= EXTI_IMR_MR13;

    NVIC_SetPriority(EXTI15_10_IRQn, 1);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void uart_tx(uint8_t byte)
{
    while ((USART1->SR & USART_SR_TXE) == 0u) {
    }
    USART1->DR = byte;
}

static void uart_flush(void)
{
    while ((USART1->SR & USART_SR_TC) == 0u) {
    }
}

static uint32_t frame_crc(void)
{
    CRC->CR = CRC_CR_RESET;
    for (uint32_t i = 0u; i < FRAME_WORDS; i++) {
        CRC->DR = frame.w[i];
    }
    return CRC->DR;
}

static void dump_block(uint16_t block)
{
    const uint32_t *src = (const uint32_t *)(FLASH_ORIGIN + block * DUMP_BLOCK_SIZE);

    frame.w[0] = FRAME_MAGIC;
    frame.w[1] = (uint32_t)block | ((uint32_t)DUMP_BLOCKS << 16);

    for (uint32_t i = 0u; i < DUMP_BLOCK_SIZE / 4u; i++) {
        frame.w[2u + i] = src[i];
    }

    uint32_t crc = frame_crc();

    for (uint32_t i = 0u; i < sizeof(frame.b); i++) {
        uart_tx(frame.b[i]);
    }
    for (uint32_t i = 0u; i < 4u; i++) {
        uart_tx((uint8_t)(crc >> (8u * i)));
    }
}

static void dump_firmware(void)
{
    for (uint32_t i = 0u; i < DUMP_BLOCKS; i++) {
        uint16_t block = (uint16_t)((i * DUMP_ORDER_MUL + DUMP_ORDER_ADD) % DUMP_BLOCKS);
        dump_block(block);
    }

    uart_flush();

    EXTI->PR = EXTI_PR_PR13;
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    btn_down = 0u;
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI->PR & EXTI_PR_PR13) == 0u) {
        return;
    }
    EXTI->PR = EXTI_PR_PR13;

    uint32_t now = g_ms;
    if ((uint32_t)(now - last_edge_ms) < TRIGGER_DEBOUNCE_MS) {
        return;
    }
    last_edge_ms = now;

    if ((BTN_PORT->IDR & (1u << BTN_PIN)) != 0u) {
        if (btn_down != 0u) {
            btn_down = 0u;
            if ((uint32_t)(now - btn_down_ms) > MORSE_HOLD_MS) {
                g_morse_on = 1u;
            }
        }
        return;
    }

    if (btn_down != 0u) {
        return;
    }
    btn_down = 1u;
    btn_down_ms = now;

    press_ts[press_count % TRIGGER_PRESSES] = now;
    press_count++;

    if (press_count >= TRIGGER_PRESSES) {
        uint32_t oldest = press_ts[(press_count - TRIGGER_PRESSES) % TRIGGER_PRESSES];
        if ((uint32_t)(now - oldest) <= TRIGGER_WINDOW_MS) {
            press_count = 0u;
            dump_firmware();
        }
    }
}
