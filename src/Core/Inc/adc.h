#ifndef ADC_H
#define ADC_H
#include <stdint.h>

void ADC_Init(void);
uint16_t ADC_Read(void);
float ADC_Calibrate(void);
float ADC_GetMagnitude(void);

#endif /* ADC_H */