#include "stm32f4xx_hal.h"
#include "uart.h"
#include "morse.h"
#include <stdio.h>
#include <string.h>

/* UART */
UART_HandleTypeDef huart2;

/* Morse */
Morse_TX_t tx;
Morse_RX_t rx;

/* -------------------------------------------------------------------------
   Hardware simple (LED pour test)
   ------------------------------------------------------------------------- */
#define MORSE_PIN GPIO_PIN_5
#define MORSE_PORT GPIOA

void Morse_On(void)  { HAL_GPIO_WritePin(MORSE_PORT, MORSE_PIN, GPIO_PIN_SET); }
void Morse_Off(void) { HAL_GPIO_WritePin(MORSE_PORT, MORSE_PIN, GPIO_PIN_RESET); }
void Morse_Delay(uint32_t ms) { HAL_Delay(ms); }

/* ------------------------------------------------------------------------- */
void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = MORSE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(MORSE_PORT, &gpio);
}

/* ------------------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    GPIO_Init();
    UART_Init(&huart2);

    Morse_TX_Init(&tx, Morse_On, Morse_Off, Morse_Delay);
    Morse_RX_Init(&rx, HAL_GetTick());

    printf("\r\n=== MORSE TERMINAL ===\r\n");

    char buffer[64];
    uint8_t idx = 0;
    char c;

    while (1)
    {
        /* =============================
           UART INPUT (console)
           ============================= */
        if (HAL_UART_Receive(&huart2, (uint8_t*)&c, 1, 0) == HAL_OK)
        {
            if (c == '\r' || c == '\n')
            {
                if (idx > 0)
                {
                    buffer[idx] = '\0';

                    printf("\r\n[TX] %s\r\n", buffer);

                    Morse_TX_Send(&tx, buffer);

                    idx = 0;
                    printf("> ");
                }
            }
            else if (c == 8 || c == 127)
            {
                if (idx > 0)
                {
                    idx--;
                    printf("\b \b");
                }
            }
            else
            {
                if (idx < sizeof(buffer) - 1)
                {
                    buffer[idx++] = c;
                    HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 10);
                }
            }
        }

        /* =============================
           RX MORSE (simulation simple)
           ============================= */

        // ⚠️ Ici tu peux remettre TON Goertzel / ADC
        uint8_t sound = 0; // remplace par ton ADC

        Morse_RX_Update(&rx, sound, HAL_GetTick());

        if (rx.word_ready)
        {
            printf("[RX] %s\r\n", rx.word_buf);
            Morse_RX_ClearWord(&rx);
        }
    }
}