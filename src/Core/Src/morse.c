#include "morse.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/* Table de codage Morse */
typedef struct { char c; const char *code; } MorseEntry_t;

static const MorseEntry_t MORSE_TABLE[] = {
    {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."}, {'F', "..-."}, {'G', "--."}, {'H', "...."},
    {'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."},
    {'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
    {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
    {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
    {'8', "---.."}, {'9', "----."},
    {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."},
    {'/', "-..-."}, {'-', "-....-"}, {'(', "-.--.-"}, {'*', ".-.-.-"},
    {'\0', NULL}
};

/* --- Encode / Decode / Sanitize --- */
const char *Morse_Encode(char c) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; MORSE_TABLE[i].c != '\0'; i++)
        if (MORSE_TABLE[i].c == c) return MORSE_TABLE[i].code;
    return NULL;
}

char Morse_Decode(const char *sym) {
    if (!sym || sym[0] == '\0') return '\0';
    for (int i = 0; MORSE_TABLE[i].c != '\0'; i++)
        if (strcmp(MORSE_TABLE[i].code, sym) == 0) return MORSE_TABLE[i].c;
    return '?';
}

void Morse_Sanitize(char *text) {
    if (!text) return;
    int j = 0;
    for (int i = 0; text[i]; i++) {
        unsigned char ch = (unsigned char)text[i];
        if (isalpha(ch) || isdigit(ch)) text[j++] = toupper(ch);
        else if (ch == ' ' && j > 0 && text[j-1] != ' ') text[j++] = ' ';
    }
    if (j > 0 && text[j-1] == ' ') j--;
    text[j] = '\0';
}

/* --- TX --- */
void Morse_TX_Init(Morse_TX_t *ctx, Morse_SignalOn_t on, Morse_SignalOff_t off, Morse_Delay_t delay_fn) {
    ctx->signal_on  = on;
    ctx->signal_off = off;
    ctx->delay      = delay_fn;
}

static void tx_play_char(Morse_TX_t *ctx, char c) {
    const char *code = Morse_Encode(c);
    if (!code) return;
    for (int i=0; code[i]; i++) {
        ctx->signal_on();
        ctx->delay(code[i] == '.' ? MORSE_UNIT_MS : MORSE_DASH_MS);
        ctx->signal_off();
        if (code[i+1]) ctx->delay(MORSE_SYM_GAP_MS);
    }
    ctx->delay(MORSE_LETTER_GAP_MS);
}

void Morse_TX_Send(Morse_TX_t *ctx, const char *text) {
    if (!ctx || !text) return;
    char buf[128]; strncpy(buf, text, 127); buf[127]='\0';
    Morse_Sanitize(buf);
    for (int i=0; buf[i]; i++)
        if (buf[i] == ' ') ctx->delay(MORSE_WORD_GAP_MS - MORSE_LETTER_GAP_MS);
        else tx_play_char(ctx, buf[i]);
}

/* --- RX --- */
static void rx_commit_letter(Morse_RX_t *ctx) {
    if (ctx->sym_cnt==0) return;
    ctx->sym_buf[ctx->sym_cnt]='\0';
    char letter = Morse_Decode(ctx->sym_buf);
    if (ctx->word_cnt < MORSE_MAX_LETTERS_PER_WORD) ctx->word_buf[ctx->word_cnt++] = letter;
    ctx->word_buf[ctx->word_cnt]='\0';
    ctx->sym_cnt=0; ctx->sym_buf[0]='\0';
}

static void rx_commit_word(Morse_RX_t *ctx) {
    rx_commit_letter(ctx);
    if (ctx->word_cnt>0) ctx->word_ready=1;
}

void Morse_RX_Init(Morse_RX_t *ctx, uint32_t tick_now) {
    memset(ctx,0,sizeof(*ctx));
    ctx->state=MORSE_RX_SILENCE;
    ctx->state_ts=tick_now;
}

void Morse_RX_Update(Morse_RX_t *ctx, uint8_t sound, uint32_t tick) {
    uint32_t dt = tick - ctx->state_ts;
    switch(ctx->state) {
        case MORSE_RX_SILENCE:
            if (sound) {
                if (dt >= MORSE_WORD_GAP_MS-MORSE_RX_MARGIN_MS) rx_commit_word(ctx);
                else if (dt >= MORSE_LETTER_GAP_MS-MORSE_RX_MARGIN_MS) rx_commit_letter(ctx);
                ctx->state=MORSE_RX_TONE; ctx->state_ts=tick;
            } break;
        case MORSE_RX_TONE:
            if (!sound) {
                char sym = (dt >= MORSE_DASH_MS-MORSE_RX_MARGIN_MS) ? '-' : '.';
                if (ctx->sym_cnt<MORSE_MAX_SYM_PER_LETTER) ctx->sym_buf[ctx->sym_cnt++]=sym;
                ctx->sym_buf[ctx->sym_cnt]='\0';
                ctx->state=MORSE_RX_SILENCE; ctx->state_ts=tick;
            } break;
    }
}

void Morse_RX_FlushWord(Morse_RX_t *ctx) { rx_commit_word(ctx); }
void Morse_RX_ClearWord(Morse_RX_t *ctx) { ctx->word_ready=0; ctx->word_cnt=0; ctx->word_buf[0]='\0'; }