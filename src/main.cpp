#include <Arduino.h>
#include <Wire.h>
#include "Led_control.h"
#include "interface.h"
#include "joystick_module.h"
#include "nfc_module.h"
#include "sd_module.h"
#include "ir_module.h"
#include "wifi_module.h"
#include "bt_module.h"  
#include "cc1101_module.h" 
#include "sleep_module.h" 
#include "nrf_module.h" 

const int NUM_ITEMS = 7; 
int indiceActual = 0;
bool dentroDeOpcion = false;


String menuItems[NUM_ITEMS] = {
    "FID/NFC", 
    "Infrarrojo IR", 
    "Wifi", 
    "Bluetooth", 
    "Sub-GHz CC1101", 
    "Gestor SD", 
    "Modulo NRF24"
};

void setup() {
  Serial.begin(115200); 
  Wire.begin(25, 26);
  
  setupSleep();
  
  setupLED();
  setupJoystick();
  inicializarPantalla();
  
  setupNFC();
  setupSD(); 
  setupIR();
  setupWiFi(); 
  setupBluetooth(); 
  setupCC1101(); 
  
  setupNRF();
}

void loop() {
  checkSleepMode(); 

  parpadearLED(500);
  actualizarJoystick(indiceActual, dentroDeOpcion, NUM_ITEMS);

  if (dentroDeOpcion && indiceActual == 0) flujoCapturaRFID(dentroDeOpcion);
  else if(dentroDeOpcion && indiceActual == 1) flujoInfrarrojo(dentroDeOpcion);
  else if(dentroDeOpcion && indiceActual == 2) flujoWiFi(dentroDeOpcion);
  else if(dentroDeOpcion && indiceActual == 3) flujoBluetooth(dentroDeOpcion);
  else if(dentroDeOpcion && indiceActual == 4) flujoCC1101(dentroDeOpcion); 
  else if(dentroDeOpcion && indiceActual == 5) flujoSDManager(dentroDeOpcion);
  
  // Ruteo al nuevo submenú modular
  else if(dentroDeOpcion && indiceActual == 6) {
      flujoNRF(dentroDeOpcion); 
  }
  
  else dibujarPantalla(indiceActual, dentroDeOpcion, menuItems, NUM_ITEMS);
}