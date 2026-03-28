#ifndef MORSE_H
#define MORSE_H

#include <stdint.h>

/* =========================================================================
   morse.h — Bibliothèque Morse généraliste
   =========================================================================
   Aucune dépendance HAL / plateforme.
   Pour émettre : fournir deux callbacks on/off + délai (ms).
   Pour recevoir : appeler Morse_RX_Update() en boucle.
   ========================================================================= */

/* -------------------------------------------------------------------------
   Timing (ms) — modifier ici pour tout le projet
   ------------------------------------------------------------------------- */
#define MORSE_UNIT_MS       100     /* Durée d'un point                     */
#define MORSE_DASH_MS       (MORSE_UNIT_MS * 3)
#define MORSE_SYM_GAP_MS    MORSE_UNIT_MS
#define MORSE_LETTER_GAP_MS (MORSE_UNIT_MS * 3)
#define MORSE_WORD_GAP_MS   (MORSE_UNIT_MS * 7)

/* Marge de tolérance pour la réception (ms) */
#define MORSE_RX_MARGIN_MS  20

/* -------------------------------------------------------------------------
   Limites buffers réception
   ------------------------------------------------------------------------- */
#define MORSE_MAX_SYM_PER_LETTER  8
#define MORSE_MAX_LETTERS_PER_WORD 32

/* -------------------------------------------------------------------------
   Callbacks fournis par l'appelant
   ------------------------------------------------------------------------- */

/* Pointeur vers une fonction "allumer le signal" */
typedef void (*Morse_SignalOn_t)(void);

/* Pointeur vers une fonction "éteindre le signal" */
typedef void (*Morse_SignalOff_t)(void);

/* Pointeur vers HAL_Delay ou équivalent : délai en ms */
typedef void (*Morse_Delay_t)(uint32_t ms);

/* Pointeur vers HAL_GetTick ou équivalent : timestamp en ms */
typedef uint32_t (*Morse_GetTick_t)(void);

/* -------------------------------------------------------------------------
   Contexte émetteur TX
   -------------------------------------------------------------------------
   Initialiser avec Morse_TX_Init(), puis appeler Morse_TX_Send().
   ------------------------------------------------------------------------- */
typedef struct {
    Morse_SignalOn_t  signal_on;
    Morse_SignalOff_t signal_off;
    Morse_Delay_t     delay;
} Morse_TX_t;

void Morse_TX_Init(Morse_TX_t *ctx,
                   Morse_SignalOn_t  on,
                   Morse_SignalOff_t off,
                   Morse_Delay_t    delay_fn);

/* Envoie une chaîne de caractères en Morse (bloquant).
   Seuls A-Z, a-z, 0-9, espace sont traités ; le reste est ignoré. */
void Morse_TX_Send(Morse_TX_t *ctx, const char *text);

/* -------------------------------------------------------------------------
   Contexte récepteur RX
   -------------------------------------------------------------------------
   Initialiser avec Morse_RX_Init().
   Appeler Morse_RX_Update() régulièrement en lui passant :
     - sound : 1 si signal détecté, 0 sinon
     - tick  : timestamp courant en ms (ex. HAL_GetTick())
   Quand un mot est prêt, word_ready passe à 1 et word_buf est rempli.
   Appeler Morse_RX_ClearWord() après avoir lu word_buf.
   ------------------------------------------------------------------------- */
typedef enum { MORSE_RX_SILENCE, MORSE_RX_TONE } Morse_RX_State_t;

typedef struct {
    /* Machine à états */
    Morse_RX_State_t state;
    uint32_t         state_ts;       /* tick d'entrée dans l'état courant  */

    /* Symboles en cours pour la lettre courante */
    char     sym_buf[MORSE_MAX_SYM_PER_LETTER + 1];
    uint8_t  sym_cnt;

    /* Mot en cours de construction */
    char     word_buf[MORSE_MAX_LETTERS_PER_WORD + 1];
    uint8_t  word_cnt;

    /* Flag : un mot complet est disponible dans word_buf */
    uint8_t  word_ready;
} Morse_RX_t;

void Morse_RX_Init(Morse_RX_t *ctx, uint32_t tick_now);

/* Mettre à jour la réception. sound=1 si signal présent, tick=timestamp ms */
void Morse_RX_Update(Morse_RX_t *ctx, uint8_t sound, uint32_t tick);

/* Forcer la validation du mot courant (appeler si silence prolongé détecté) */
void Morse_RX_FlushWord(Morse_RX_t *ctx);

/* Remettre word_ready à 0 après lecture de word_buf */
void Morse_RX_ClearWord(Morse_RX_t *ctx);

/* -------------------------------------------------------------------------
   Utilitaires bas niveau (utilisables indépendamment)
   ------------------------------------------------------------------------- */

/* Retourne le code Morse d'un caractère (ex. 'A' → ".-"), ou NULL */
const char *Morse_Encode(char c);

/* Décode une séquence de points/tirets (ex. ".-") → caractère, ou '?' */
char Morse_Decode(const char *sym);

/* Nettoie une chaîne : garde uniquement lettres, chiffres et espaces,
   convertit en majuscules. Modifie la chaîne en place. */
void Morse_Sanitize(char *text);

#endif /* MORSE_H */
