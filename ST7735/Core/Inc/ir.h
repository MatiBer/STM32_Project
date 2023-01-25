#pragma once

#include "stm32l476xx.h"

// Stałe opisujące klawisze
#define IR_ONOFF      0x45
#define IR_MENU       0x47
#define IR_PLUS       0x40
#define IR_BACK       0x43
#define IR_REWIND     0x07
#define IR_PLAY       0x15
#define IR_FORWARD    0x09
#define IR_MINUS      0x19
#define IR_CANCEL     0x0D


// Procedura obsługi przerwania
void ir_tim_interrupt(void);

// Inicjalizacja komunikacji przez podczerwień
void ir_init(void);

// Funkcja odczytująca dane
// return - kod klawisza lub -1
int ir_read(void);
