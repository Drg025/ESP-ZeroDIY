#ifndef PANTALLA_FLIPPER_H
#define PANTALLA_FLIPPER_H

#include <U8g2lib.h>
#include <Wire.h>

void inicializarPantalla();

void dibujarPantalla(int menuIndex, bool enSubMenu, String items[], int numItems);
#endif 
