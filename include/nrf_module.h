#ifndef NRF_MODULE_H
#define NRF_MODULE_H

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <U8g2lib.h>
#include <SD.h>

// Los pines exactos que acabamos de conectar
#define NRF_CE_PIN 4
#define NRF_CSN_PIN 27

// Aseguramos el acceso a tu pantalla global (igual que en sd_module.cpp)
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2; 

// Declaración de funciones
bool setupNRF();
void iniciarEscanerNRF();
void escanerMouseJack();
void flujoNRF(bool &dentroDeOpcion);
void inyectarPayloadLogitech(uint8_t* mac_addr);
void ejecutarDuckyScript(const char* filepath, uint64_t target_mac);

#endif