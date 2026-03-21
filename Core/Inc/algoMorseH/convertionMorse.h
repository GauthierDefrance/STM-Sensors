#ifndef __MORSE_ENCODER_H
#define __MORSE_ENCODER_H


#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>



/*
 * Fonctions principales d'encodage
 * Prend en paramètre le texte et la broche de sortie
 *  */
void convMorse(char* texte, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/* Fonctions utilitaires */
void garderLettresEspace(char *texte);
int tailleChar(char* texte);
char* codeMorse(char texte);

/* Contrôle du signal de sortie */
void buzzer_on(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void buzzer_off(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

#endif
