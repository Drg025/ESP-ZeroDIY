#include "Led_control.h"

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
}

void encenderLED() {
  digitalWrite(LED_PIN, HIGH);
}

void apagarLED() {
  digitalWrite(LED_PIN, LOW);
}

void parpadearLED(int tiempo) {
  static unsigned long previousMillis = 0;
  static bool ledState = false;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= (unsigned long)tiempo) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}