#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include "esp_wifi.h"

// Pantalla compartida
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// ══════════════════════════════════════════════════════
//  ESTRUCTURAS PARA ARCHIVOS .PCAP (WARDRIVING)
// ══════════════════════════════════════════════════════

// Cabecera global del archivo PCAP (24 bytes)
typedef struct {
    uint32_t magic_number;   // Número mágico (0xa1b2c3d4)
    uint16_t version_major;  // Versión mayor (2)
    uint16_t version_minor;  // Versión menor (4)
    int32_t  thiszone;       // Corrección de zona horaria (0)
    uint32_t sigfigs;        // Precisión (0)
    uint32_t snaplen;        // Longitud máxima de captura (65535)
    uint32_t network;        // Tipo de enlace de datos (105 para IEEE 802.11)
} pcap_hdr_t;

// Cabecera de cada paquete individual capturado (16 bytes)
typedef struct {
    uint32_t ts_sec;         // Timestamp (segundos)
    uint32_t ts_usec;        // Timestamp (microsegundos)
    uint32_t incl_len;       // Longitud del paquete guardado
    uint32_t orig_len;       // Longitud real del paquete en el aire
} pcaprec_hdr_t;

// ══════════════════════════════════════════════════════
//  FUNCIONES PÚBLICAS
// ══════════════════════════════════════════════════════
void setupWiFi();
void flujoWiFi(bool &dentroDeOpcion);
void menuDeauth(bool &dentroDeOpcion);
void menuWardriving(bool &dentroDeOpcion);
void scanNetworks();
void startHoneypot(int plantilla);
void flujoServidorDatos(bool &dentroDeOpcion);
void ssidSpamReal();
void probeSnifferReal();

#endif // WIFI_MODULE_H