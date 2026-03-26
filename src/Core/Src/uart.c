#include "uart.h"
#include <string.h>
#include <stdio.h>

UART_HandleTypeDef huart2;

void UART_Init(UART_HandleTypeDef *huart)
{
    huart->Instance          = USART2;
    huart->Init.BaudRate     = 115200;
    huart->Init.WordLength   = UART_WORDLENGTH_8B;
    huart->Init.StopBits     = UART_STOPBITS_1;
    huart->Init.Parity       = UART_PARITY_NONE;
    huart->Init.Mode         = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(huart);
}

int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

uint8_t UART_ReadChar(UART_HandleTypeDef *huart, char *c)
{
    return (HAL_UART_Receive(huart, (uint8_t*)c, 1, 0) == HAL_OK);
}

void UART_Write(UART_HandleTypeDef *huart, const char *buf)
{
    HAL_UART_Transmit(huart, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}