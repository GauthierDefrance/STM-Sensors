#ifndef UART_H
#define UART_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Initialise l'UART2 à 115200 baud */
void UART_Init(UART_HandleTypeDef *huart);

/* Redirection printf vers UART2 */
int __io_putchar(int ch);

/* Lecture non bloquante : retourne 1 si un caractère lu, 0 sinon */
uint8_t UART_ReadChar(UART_HandleTypeDef *huart, char *c);

/* Écriture d'un buffer */
void UART_Write(UART_HandleTypeDef *huart, const char *buf);

#endif /* UART_H */