#ifndef IR_MODULE_H
#define IR_MODULE_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <U8g2lib.h>

// Definición de pines según tu esquemático
#define IR_TX_PIN 4
#define IR_RX_PIN 13

// Pantalla compartida
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// --- FUNCIONES PRINCIPALES (Expuestas al main) ---
void setupIR();
void flujoInfrarrojo(bool &dentroDeOpcion);

// --- FUNCIONES INTERNAS DEL MÓDULO ---
void modoClonarIR(bool &dentroDeOpcion);
bool recibirCodigoIR(String &resultadoHex, uint32_t &codigoCrudo);
void enviarCodigoIR(uint32_t codigo);
void mostrarOpcionesIR(String codigoHex, uint32_t codigoCrudo, bool &dentroDeOpcion);
void ataqueTVUniversal();

// --- SISTEMA MODULAR DE TELEVISIONES ---
void menuTVMarcas(bool &dentroDeOpcion); 
void menuAccionTV(int marcaID);          
void ejecutarAtaqueTV(int marcaID, int accionID);

// --- SISTEMA MODULAR DE AIRES ACONDICIONADOS ---
void menuACMarcas(bool &dentroDeOpcion); 
void menuAccionAC(int marcaID);          
// Se modificó para recibir el ID de la acción completa
void ejecutarAtaqueAC(int marcaID, int accionAC); 

#endif // IR_MODULE_H