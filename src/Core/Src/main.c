/* =========================================================================
   main.c — Émetteur / Récepteur Morse STM32 (NUCLEO-F411RE ou compatible)
   =========================================================================
   Matériel :
     Beeper passif  → PA7 (TIM3_CH2) via 100Ω
     KY-038 AO      → diviseur 10k/10k → PA0 (ADC1_IN0)
     KY-038 VCC     → 5V,  GND → GND
     LD2 (LED)      → PA5 (clignote à chaque mot reçu)

   Utilisation (terminal 115200 baud) :
     Tapez un message + Entrée → émis en Morse sur le beeper
     Ce que le micro entend s'affiche automatiquement : [RX] MOT
   ========================================================================= */

#include "main.h"
#include "morse.h"      /* Bibliothèque Morse généraliste                  */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "stm32f446xx.h"
#include <stdint.h>


/* =========================================================================
   Handles HAL
   ========================================================================= */
ADC_HandleTypeDef  hadc1;
TIM_HandleTypeDef  htim3;
UART_HandleTypeDef huart2;

/* =========================================================================
   Prototypes init
   ========================================================================= */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);

/* =========================================================================
   Paramètres matériels
   ========================================================================= */
#define MORSE_BEEP_FREQ_HZ  2000     /* Fréquence du bip Morse               */
#define ADC_SAMPLES         32      /* Échantillons pour le calcul RMS      */
#define ADC_MAX             4095
#define DB_THRESHOLD       -40.0f   /* dBFS : seuil de détection du son     */
float real_sampling_rate = 100000.0f; // Valeur par défaut, sera écrasée au démarrage

/* =========================================================================
   printf → UART2
   ========================================================================= */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* =========================================================================
   ADC — lecture micro KY-038
   ========================================================================= */
static uint16_t ADC_Read(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t v = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return v;
}

/* Paramètres Goertzel */
#define TARGET_FREQ    2000.0f
#define SAMPLING_RATE  8000.0f  // Ajustez selon votre vitesse d'échantillonnage réelle
#define N_GOERTZEL     400      // Augmenté car le STM32 lit l'ADC très vite

float get_mag_at_freq(void) {
    // On calcule les coefficients avec la vraie vitesse mesurée
    float k = (int)(0.5f + ((float)N_GOERTZEL * TARGET_FREQ) / real_sampling_rate);
    float omega = (2.0f * (float)M_PI * k) / N_GOERTZEL;
    float cosine = cosf(omega);
    float coeff = 2.0f * cosine;

    float q0, q1, q2;
    q1 = 0; q2 = 0;

    for (int i = 0; i < N_GOERTZEL; i++) {
        float sample = (float)ADC_Read() - 2048.0f; // Centrer le signal
        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }

    return (q1 * q1 + q2 * q2 - q1 * q2 * coeff);
}

/* =========================================================================
   Beeper — callbacks pour Morse_TX_t
   ========================================================================= */
static void Beeper_On(void)
{
    uint32_t arr = (1000000UL / MORSE_BEEP_FREQ_HZ) - 1;
    if (arr > 0xFFFF) arr = 0xFFFF;
    TIM3->ARR  = arr;
    TIM3->CCR2 = arr / 2;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->CR1  |= TIM_CR1_CEN;
}

static void Beeper_Off(void)
{
    TIM3->CCER &= ~TIM_CCER_CC2E;
    TIM3->CR1  &= ~TIM_CR1_CEN;
    GPIOA->ODR &= ~GPIO_PIN_7;   /* Forcer PA7 à 0 */
}

/* =========================================================================
   MAIN
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
    Beeper_Off();

    /* --- Initialisation contextes Morse --- */
    Morse_TX_t tx;
    Morse_TX_Init(&tx, Beeper_On, Beeper_Off, HAL_Delay);

    Morse_RX_t rx;
    Morse_RX_Init(&rx, HAL_GetTick());

    /* --- Accueil --- */
    printf("\r\n");
    printf("============================================\r\n");
    printf("  EMETTEUR / RECEPTEUR MORSE STM32\r\n");
    printf("============================================\r\n");
    printf("  Tapez un message + Entree pour emettre.\r\n");
    printf("  Reception affichee automatiquement.\r\n");
    printf("  Seuil micro : %.0f dBFS | Dot : %d ms\r\n",
           (double)DB_THRESHOLD, MORSE_UNIT_MS);
    printf("============================================\r\n\r\n");

    /* --- Buffer saisie UART --- */
    char    uart_buf[64];
    uint8_t uart_idx = 0;
    uint8_t tx_busy  = 0;   /* 1 pendant l'émission → réception suspendue */

    char c;

    /* --- Calibration de la vitesse du micro --- */
	printf("Calibrage du micro (silence s'il vous plait)... ");
	uint32_t t_start = HAL_GetTick();
	uint32_t samples_count = 0;

	// Compter combien de lectures l'ADC peut faire en 1 seconde (1000 ms)
	while((HAL_GetTick() - t_start) < 1000) {
		ADC_Read();
		samples_count++;
	}
	real_sampling_rate = (float)samples_count;
	printf("Fait ! Vitesse ADC : %.0f Hz\r\n", real_sampling_rate);
	printf("============================================\r\n\r\n");

    while (1)
    {
        /* ----------------------------------------------------------------
           1. SAISIE UART
           ---------------------------------------------------------------- */
        if (HAL_UART_Receive(&huart2, (uint8_t*)&c, 1, 0) == HAL_OK) {

            if (c == '\r' || c == '\n') {
                /* Entrée → émettre */
                if (uart_idx > 0) {
                    uart_buf[uart_idx] = '\0';
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
                    printf("[TX] %s\r\n", uart_buf);

                    tx_busy = 1;
                    Morse_TX_Send(&tx, uart_buf);
                    tx_busy = 0;

                    uart_idx = 0;
                    printf("[TX] Termine. Entrez un message :\r\n");
                }

            } else if (c == 8 || c == 127) {
                /* Backspace */
                if (uart_idx > 0) {
                    uart_idx--;
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\b \b", 3, 10);
                }

            } else {
                /* Caractère normal */
                if (uart_idx < sizeof(uart_buf) - 1) {
                    uart_buf[uart_idx++] = c;
                    HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 10); /* écho */
                }
            }
        }

        /* ----------------------------------------------------------------
           2. RÉCEPTION MORSE (suspendue pendant TX)
           ---------------------------------------------------------------- */
        if (!tx_busy) {
        	float magnitude = get_mag_at_freq();
        	uint8_t sound = (magnitude > 500000.0f) ? 1 : 0;
            uint32_t tick  = HAL_GetTick();

            Morse_RX_Update(&rx, sound, tick);

            /* Flush automatique si silence très long (fin de message) */
            if (rx.state == MORSE_RX_SILENCE) {
                uint32_t silent = tick - rx.state_ts;
                if (silent > MORSE_WORD_GAP_MS * 3 && rx.word_cnt > 0) {
                    Morse_RX_FlushWord(&rx);
                }
            }

            /* Mot prêt → afficher */
            if (rx.word_ready) {
                printf("[RX] %s\r\n", rx.word_buf);

                // Détection de la fin de phrase
                if (strchr(rx.word_buf, '*') != NULL) {
                    printf("--- FIN DE TRANSMISSION ---\r\n");
                }

                HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
                Morse_RX_ClearWord(&rx);
            }
        }
    }
}

/* =========================================================================
   INIT PÉRIPHÉRIQUES
   ========================================================================= */

static void MX_TIM3_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 83;       /* 84 MHz / 84 = 1 MHz        */
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 999;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 499;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);

    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_7;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOA, &gpio);
}

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 16;
    RCC_OscInitStruct.PLL.PLLN            = 336;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ            = 2;
    RCC_OscInitStruct.PLL.PLLR            = 2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* PA0 → analogique (ADC) */
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA5 → LED LD2 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PC13 → B1 (bouton, non utilisé mais configuré) */
    GPIO_InitStruct.Pin  = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
