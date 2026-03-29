# STM Sensor - Communication Sonore entre Microcontrôleurs STM32

Projet L3 Informatique — Introduction aux microcontrôleurs
Cergy Paris Université 2025/2026

---

## Table des Matières

- [Description](#description)
- [Membre de l'équipe](#membre-de-lequipe)
- [Matériel requis](#materiel-requis)
- [Schéma](#schema)
- [Fonctionnalités](#fonctionnalites)
- [Fonctionnement](#fonctionnement)
- [Implémentation logicielle](#implémentation-logicielle)
- [Résultat et limites](#resultats-et-limites)
- [Licence](#licence)

---

## Description

Système de communication sans fil entre deux microcontrôleurs **STM32F4** utilisant le son (code Morse) comme moyen de transmission de données.

Un message saisi sur un PC est encodé en Morse,
transmis via buzzer passif,
capté par un microphone KY-038,
décodé grâce à l'algorithme de Goertzel,
puis affiché sur un second PC.

Le système est bidirectionnel : les deux microcontrôleurs peuvent à la fois émettre et recevoir des messages.

---

## Membre de l'équipe

Groupe STM Sensors - Groupe C

- TAKKA Kamelia
- DEFRANCE Gauthier
- AMMAD Kenan
- FERON Stéphen
- FANG Zexu
- HAOHUN Yu

---

## Matériel requis

| Composant | Quantité | Rôle |
|-----------|----------|------|
| STM32 Nucleo F446RE | 2x | Microcontrôleurs principaux |
| Buzzer passif (ex: 12085) | 2x | Émission des signaux Morse |
| Microphone KY-038 | 2x | Capture des signaux sonores |
| Câble USB | 2x | Liaison série PC - STM32 |
| Breadboard + câbles | - | Prototypage |

> Le microphone KY-038 nécessite une résistance entre la sortie analogique et son VCC pour rehausser la tension de sortie.

---

## Schéma
![schéma](https://github.com/GauthierDefrance/STM-Sensors.git/image.png)

## Fonctionnalités

- Encodage de messages texte en code Morse
- Transmission sonore via un buzzer piloté en PWM
- Réception et décodage des signaux sonores via ADC
- Filtrage fréquentiel par **algorithme de Goertzel** (anti-bruit)
- Communication **bidirectionnelle** entre les deux microcontrôleurs
- LED indicatrice d'émission en cours

---

## Fonctionnement

1. L'utilisateur saisit un message sur **PC 1** (terminal PuTTY, baudrate 115200)
2. **MC1** reçoit le message via UART et l'encode en signaux Morse (PWM vers le buzzer)
3. **MC2** capte les signaux via le microphone (entrée analogique PA0)
4. L'algorithme de Goertzel filtre les bruits parasites sur la fréquence cible
5. Le message décodé est transmis au **PC 2** et affiché

---

## Implémentation logiciel

### Configuration STM32 (commune aux deux cartes)

- USART2 - communication série avec le PC
- PA0 - entrée analogique (ADC) pour le microphone
- TI%3 - génération du signal PWM pour le buzzer

### Boucle principale
1. **Réception UART** - lecture caractère par caractère, envoi du message par la touche Entrée
2. **Encodage Morse** - convertion via la table de correspondance, activation buzzer
3. **Déction sonore** - lecture ADC + filtrage Goertzel + classification court/long
4. **Décodage Morse** - conversion des signaux `.` et  `-` en caractères et affichage une fois 500ms de silence détecté

---

## Résultat et limites

### Ce qui fonctionne

- Communication Morse sonore validée en conditions réelles (salle de classe)
- L'algorithme de Goertzel réduit significativement les faux positifs
- Test réussi avec le message `SOS` entre les deux microcontrôleurs

### Limites identifiées

- Un bruit bref et intense (ex : claquement de mains) est interprété comme la lettre **"E"** (signale court en Morse)
- Portée limitée par la quantité du microphone KY-038 (bas de gamme)
- Implémentation basique de Goertzel - une version optimisée améliorait la robustesse

---

## Licence

Projet académique — L3 Informatique
Cergy Paris Université
