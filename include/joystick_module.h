#ifndef JOYSTICK_MODULE_H
#define JOYSTICK_MODULE_H

#include <Arduino.h>

void setupJoystick();
void actualizarJoystick(int &menuIndex, bool &enSubMenu, int numItems);

#endif