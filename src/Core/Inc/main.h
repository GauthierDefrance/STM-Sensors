#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

/* Paramètres matériels */
#define MORSE_BEEP_FREQ_HZ  2000
#define ADC_SAMPLES         32
#define ADC_MAX             4095
#define DB_THRESHOLD       -40.0f

extern float real_sampling_rate;

#endif /* MAIN_H */