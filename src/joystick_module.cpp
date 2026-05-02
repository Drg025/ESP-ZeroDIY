#include "joystick_module.h"

#define JOY_Y 35
#define JOY_SW 32

void setupJoystick() {
    pinMode(JOY_SW, INPUT_PULLUP);
}

void actualizarJoystick(int &menuIndex, bool &enSubMenu, int numItems){
    static bool lastSW = HIGH;  // recordar estado anterior del botón

    if (!enSubMenu) {
        int valorY = analogRead(JOY_Y);

        if(valorY < 1000){
            if(menuIndex > 0) menuIndex--;
            delay(100);
        }
        if(valorY > 3000){
            if(menuIndex < numItems - 1) menuIndex++;
            delay(100);
        }

        bool currentSW = digitalRead(JOY_SW);
        if (currentSW == LOW && lastSW == HIGH) {  // detectar flanco de bajada
            enSubMenu = true;
            delay(100);  // debounce
        }
        lastSW = currentSW;
    } else {
        bool currentSW = digitalRead(JOY_SW);
        if (currentSW == LOW && lastSW == HIGH) {
            enSubMenu = false;
            delay(200);
        }
        lastSW = currentSW;
    }
}
