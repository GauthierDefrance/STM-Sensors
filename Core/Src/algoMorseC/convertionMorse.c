#include "algoMorseH/convertionMorse.h"

#define UNIT 100  // 1 unité = 100ms

void convMorse(char* texte, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    if (texte == NULL || texte[0] == '\0') return;

    // Étape 1 : On nettoie et on met TOUT en minuscule d'un coup
    garderLettresEspace(texte);

    int i = 0;
    while (texte[i] != '\0') {
        if (texte[i] == ' ') {
            // Espace entre mots : Standard Morse = 7 unités.
            // Comme on a déjà fini une lettre (3u), on rajoute 4u.
            HAL_Delay(4 * UNIT);
        } else {
            char* code = codeMorse(texte[i]);

            if (code != NULL) {
                int taille = tailleChar(code);

                for (int j = 0; j < taille; j++) {
                    buzzer_on(GPIOx, GPIO_Pin);

                    if (code[j] == '.') HAL_Delay(1 * UNIT);
                    else if (code[j] == '-') HAL_Delay(3 * UNIT);

                    buzzer_off(GPIOx, GPIO_Pin);

                    // Silence entre deux symboles d'une même lettre (1u)
                    if (j < taille - 1) HAL_Delay(1 * UNIT);
                }
                // Silence entre deux lettres (3u)
                HAL_Delay(3 * UNIT);
            }
        }
        i++;
    }
}

void garderLettresEspace(char *texte) {
    int j = 0;
    for (int i = 0; texte[i] != '\0'; i++) {
        // On vérifie si le caractère est une lettre
        if (isalpha((unsigned char)texte[i]) ) {
        	texte[j++] = tolower((unsigned char)texte[i]);
        } else if ( texte[i] == ' ' ) {
        	texte[j++] = ' ';
        }
    }
    texte[j] = '\0'; // On n'oublie pas de fermer la chaîne
}

int tailleChar(char* texte) {
    int cpt = 0;
    while (texte[cpt] != '\0') cpt++;
    return cpt;
}

void buzzer_on(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    // allumer le buzzer
	// mais comme stefx n'a pas de buzzer, j'active la LED PA5
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

void buzzer_off(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    // éteindre le buzzer
	// mais comme stefx n'a pas de buzzer, j'éteint la LED PA5
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}

char* codeMorse(char texte) {
    switch (texte) {
        case 'a': return ".-";
        case 'b': return "-...";
        case 'c': return "-.-.";
        case 'd': return "-..";
        case 'e': return ".";
        case 'f': return "..-.";
        case 'g': return "--.";
        case 'h': return "....";
        case 'i': return "..";
        case 'j': return ".---";
        case 'k': return "-.-";
        case 'l': return ".-..";
        case 'm': return "--";
        case 'n': return "-.";
        case 'o': return "---";
        case 'p': return ".--.";
        case 'q': return "--.-";
        case 'r': return ".-.";
        case 's': return "...";
        case 't': return "-";
        case 'u': return "..-";
        case 'v': return "...-";
        case 'w': return ".--";
        case 'x': return "-..-";
        case 'y': return "-.--";
        case 'z': return "--..";
        default : return NULL;
    }
}
