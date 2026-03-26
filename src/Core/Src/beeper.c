#include "beeper.h"
#include "stm32f446xx.h"
#include "main.h"

void Beeper_On(void)
{
    uint32_t arr = (1000000UL / MORSE_BEEP_FREQ_HZ) - 1;
    if (arr > 0xFFFF) arr = 0xFFFF;
    TIM3->ARR  = arr;
    TIM3->CCR2 = arr / 2;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->CR1  |= TIM_CR1_CEN;
}

void Beeper_Off(void)
{
    TIM3->CCER &= ~TIM_CCER_CC2E;
    TIM3->CR1  &= ~TIM_CR1_CEN;
    GPIOA->ODR &= ~GPIO_PIN_7;
}