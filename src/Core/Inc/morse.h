#ifndef MORSE_H
#define MORSE_H

#include <stdint.h>

/* Timing */
#define MORSE_UNIT_MS        100
#define MORSE_DASH_MS        (MORSE_UNIT_MS * 3)
#define MORSE_SYM_GAP_MS     MORSE_UNIT_MS
#define MORSE_LETTER_GAP_MS  (MORSE_UNIT_MS * 3)
#define MORSE_WORD_GAP_MS    (MORSE_UNIT_MS * 7)
#define MORSE_RX_MARGIN_MS   20

/* Buffers RX */
#define MORSE_MAX_SYM_PER_LETTER   8
#define MORSE_MAX_LETTERS_PER_WORD 32

/* Callbacks */
typedef void (*Morse_SignalOn_t)(void);
typedef void (*Morse_SignalOff_t)(void);
typedef void (*Morse_Delay_t)(uint32_t ms);

/* TX */
typedef struct {
    Morse_SignalOn_t  signal_on;
    Morse_SignalOff_t signal_off;
    Morse_Delay_t     delay;
} Morse_TX_t;

void Morse_TX_Init(Morse_TX_t *ctx, Morse_SignalOn_t on, Morse_SignalOff_t off, Morse_Delay_t delay_fn);
void Morse_TX_Send(Morse_TX_t *ctx, const char *text);

/* RX */
typedef enum { MORSE_RX_SILENCE, MORSE_RX_TONE } Morse_RX_State_t;

typedef struct {
    Morse_RX_State_t state;
    uint32_t state_ts;
    char sym_buf[MORSE_MAX_SYM_PER_LETTER + 1];
    uint8_t sym_cnt;
    char word_buf[MORSE_MAX_LETTERS_PER_WORD + 1];
    uint8_t word_cnt;
    uint8_t word_ready;
} Morse_RX_t;

void Morse_RX_Init(Morse_RX_t *ctx, uint32_t tick_now);
void Morse_RX_Update(Morse_RX_t *ctx, uint8_t sound, uint32_t tick);
void Morse_RX_FlushWord(Morse_RX_t *ctx);
void Morse_RX_ClearWord(Morse_RX_t *ctx);

/* Utilitaires */
const char *Morse_Encode(char c);
char Morse_Decode(const char *sym);
void Morse_Sanitize(char *text);

#endif /* MORSE_H */