#include "adc.h"
#include "main.h"
#include "stm32f446xx.h"
#include <math.h>

/* Implémentation des fonctions */
static float goertzel(float target_freq, uint32_t N);

uint16_t ADC_Read(void) { /* HAL_ADC_Start + GetValue */ }

float ADC_Calibrate(void)
{
    uint32_t t_start = HAL_GetTick();
    uint32_t samples = 0;
    while (HAL_GetTick() - t_start < 1000) { ADC_Read(); samples++; }
    return (float)samples;
}

float ADC_GetMagnitude(void) { return goertzel(TARGET_FREQ, N_GOERTZEL); }

/* Goertzel interne */
static float goertzel(float target_freq, uint32_t N)
{
    float k = 0.5f + (N * target_freq) / real_sampling_rate;
    float omega = (2.0f * M_PI * k) / N;
    float coeff = 2.0f * cosf(omega);
    float q0 = 0, q1 = 0, q2 = 0;
    for (uint32_t i=0;i<N;i++) { /* lire ADC et calcul */ }
    return (q1*q1 + q2*q2 - q1*q2*coeff);
}