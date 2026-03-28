/* =========================================================================
   main.c — Terminal Morse Audio STM32 (TX + RX)
   Matériel :
     - PA7 : TIM3_CH2 → Beeper passif
     - PA0 : ADC1_IN0 → Micro KY-038 (via diviseur 10k/10k)
     - PA5 : LED LD2
   ========================================================================= */

#include "main.h"
#include "morse.h"
#include "peripherical_Init.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* =========================================================================
   PARAMÈTRES GLOBAUX
   ========================================================================= */

#define MORSE_BEEP_FREQ_HZ   2000
#define TARGET_FREQ          2000.0f
#define N_GOERTZEL           400
#define DB_THRESHOLD         -40.0f

float real_sampling_rate = 100000.0f;

/* =========================================================================
   HANDLES HAL (déclarés dans peripherical_Init.c)
   ========================================================================= */

extern ADC_HandleTypeDef  hadc1;
extern TIM_HandleTypeDef  htim3;
extern UART_HandleTypeDef huart2;

/* =========================================================================
   REDIRECTION printf → UART2
   -------------------------------------------------------------------------
   Permet d'utiliser printf() directement sur l'UART2.
   Chaque caractère est transmis via HAL_UART_Transmit().
   ========================================================================= */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* =========================================================================
   ADC — Lecture brute
   -------------------------------------------------------------------------
   Effectue une conversion ADC unique sur PA0 (micro).
   Retourne une valeur 12 bits (0–4095).
   Utilisé par l’algorithme de Goertzel.
   ========================================================================= */
static uint16_t adc_read(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t v = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return v;
}

/* =========================================================================
   Goertzel — Détection amplitude à 2 kHz
   -------------------------------------------------------------------------
   Implémente l’algorithme de Goertzel pour mesurer l’énergie du signal
   autour de 2000 Hz. Utilisé pour détecter si un bip est présent.

   - Lit N_GOERTZEL échantillons ADC
   - Applique la récurrence Goertzel
   - Retourne la magnitude (énergie)
   ========================================================================= */
static float goertzel_2k(void)
{
    float k = (int)(0.5f + ((float)N_GOERTZEL * TARGET_FREQ) / real_sampling_rate);
    float omega = (2.0f * M_PI * k) / N_GOERTZEL;
    float coeff = 2.0f * cosf(omega);

    float q0, q1 = 0, q2 = 0;

    for (int i = 0; i < N_GOERTZEL; i++) {
        float sample = (float)adc_read() - 2048.0f;  // centrage
        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }

    return (q1*q1 + q2*q2 - q1*q2*coeff);
}

/* =========================================================================
   BEEPER — pour Morse_TX
   -------------------------------------------------------------------------
   beep_on()  : configure TIM3_CH2 pour générer un PWM à 2 kHz.
   beep_off() : coupe le PWM et force PA7 à 0.

   Ces fonctions sont passées comme callbacks à la librairie Morse.
   ========================================================================= */
static void beep_on(void)
{
    uint32_t arr = (1000000UL / MORSE_BEEP_FREQ_HZ) - 1;
    TIM3->ARR  = arr;
    TIM3->CCR2 = arr / 2;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->CR1  |= TIM_CR1_CEN;
}

static void beep_off(void)
{
    TIM3->CCER &= ~TIM_CCER_CC2E;
    TIM3->CR1  &= ~TIM_CR1_CEN;
    GPIOA->ODR &= ~GPIO_PIN_7;
}

/* =========================================================================
   MAIN — Boucle principale
   -------------------------------------------------------------------------
   1. Initialise tous les périphériques
   2. Calibre la vitesse d’échantillonnage ADC
   3. Lit les caractères UART → émet en Morse
   4. Analyse le son via Goertzel → réception Morse
   ========================================================================= */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    MX_TIM3_Init();

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    beep_off();

    /* Contexte Morse */
    Morse_TX_t tx;
    Morse_TX_Init(&tx, beep_on, beep_off, HAL_Delay);

    Morse_RX_t rx;
    Morse_RX_Init(&rx, HAL_GetTick());

    printf("\r\n=== STM SENSORS ===\r\n");

    /* ---------------------------------------------------------------------
       Calibration ADC : mesure combien d’échantillons ADC/s on peut lire.
       Permet d’ajuster Goertzel à la vraie vitesse d’échantillonnage.
       --------------------------------------------------------------------- */
    printf("Calibration en cours... ");
    printf("Restez silencieux... ");
    uint32_t t0 = HAL_GetTick();
    uint32_t count = 0;

    while ((HAL_GetTick() - t0) < 1000) {
        adc_read();
        count++;
    }

    real_sampling_rate = (float)count;
    printf("OK : %.0f Hz\r\n\r\n", real_sampling_rate);

    /* Buffer UART */
    char    buf[64];
    uint8_t idx = 0;
    uint8_t tx_busy = 0;
    char    c;

    printf("\r\n=== ============= ===\r\n");

    /* ---------------------------------------------------------------------
       Boucle principale
       --------------------------------------------------------------------- */
    printf("Entrez les messages que vous souhaitez envoyer.\r\n");
    printf("Puis cliquez sur entrée.\r\n");
    while (1)
    {
        /* ---------------- UART : saisie utilisateur ---------------- */
        if (HAL_UART_Receive(&huart2, (uint8_t*)&c, 1, 0) == HAL_OK)
        {
            if (c == '\r' || c == '\n') {
                if (idx > 0) {
                    buf[idx] = '\0';
                    printf("\r\n[ENVOIE] %s\r\n", buf);

                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
                    tx_busy = 1;
                    Morse_TX_Send(&tx, buf);
                    tx_busy = 0;
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

                    idx = 0;
                    printf("> ");
                }
            }
            else if (c == 8 || c == 127) {  // Backspace
                if (idx > 0) {
                    idx--;
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 10);
                }
            }
            else {
                if (idx < sizeof(buf)-1) {
                    buf[idx++] = c;
                    HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 10);
                }
            }
        }

        /* ---------------- Réception Morse via Goertzel ---------------- */
        if (!tx_busy)
        {
            float mag = goertzel_2k();
            uint8_t sound = (mag > 500000.0f);
            uint32_t tick = HAL_GetTick();

            Morse_RX_Update(&rx, sound, tick);

            /* Fin de mot détectée par silence prolongé */
            if (rx.state == MORSE_RX_SILENCE) {
                uint32_t silent = tick - rx.state_ts;
                if (silent > MORSE_WORD_GAP_MS * 3 && rx.word_cnt > 0)
                    Morse_RX_FlushWord(&rx);
            }

            /* Mot complet reçu */
            if (rx.word_ready) {
                printf("[RECEPTION] %s\r\n", rx.word_buf);
                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                Morse_RX_ClearWord(&rx);
            }
        }
    }
}
