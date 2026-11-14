#ifndef BLINK_H
#define BLINK_H

#include "stm32f4xx.h"

#define MORSE_PORT        GPIOA
#define MORSE_PIN         8u

#define BTN_PORT          GPIOC
#define BTN_PIN           13u

#define MORSE_UNIT_MS     80u
#define MORSE_DOT_MS      (1u * MORSE_UNIT_MS)
#define MORSE_DASH_MS     (3u * MORSE_UNIT_MS)
#define MORSE_GAP_MS      (1u * MORSE_UNIT_MS)
#define MORSE_LETTER_MS   (3u * MORSE_UNIT_MS)
#define MORSE_WORD_MS     (7u * MORSE_UNIT_MS)
#define MORSE_REPEAT_MS   (20u * MORSE_UNIT_MS)

#define TRIGGER_PRESSES   8u
#define TRIGGER_WINDOW_MS 3000u
#define TRIGGER_DEBOUNCE_MS 40u
#define MORSE_HOLD_MS     5000u

#define FLASH_ORIGIN      0x08000000UL
#define DUMP_BLOCK_SIZE   1024u
#define DUMP_BLOCKS       63u                       
#define FLAG_ADDR         (FLASH_ORIGIN + DUMP_BLOCKS * DUMP_BLOCK_SIZE)
#define FLAG_MAX_LEN      256u

#define FRAME_MAGIC       0x4B4E4C42UL              
#define FRAME_HDR_BYTES   8u
#define DUMP_BAUD         115200u
#define DUMP_ORDER_MUL    29u
#define DUMP_ORDER_ADD    11u

extern volatile uint32_t g_ms;
extern volatile uint32_t g_morse_on;

void delay_ms(uint32_t ms);

void morse_out(int level);
void morse_send(const volatile char *s, uint32_t max_len);

void dump_init(void);

#endif 
