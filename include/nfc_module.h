#ifndef NFC_MODULE_H
#define NFC_MODULE_H

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include <U8g2lib.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

void setupNFC();
bool capturarUID();
String obtenerUltimoUID();
bool clonarTarjeta();
void emularTarjeta(); 
void volcarMemoria(); // <-- NUEVA: Función de volcado de sectores
void flujoCapturaRFID(bool &dentroDeOpcion);

#endif