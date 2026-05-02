#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

#define LED_PIN 2

void setupLED();
void parpadearLED(int tiempo);
void encenderLED();
void apagarLED();

#endif