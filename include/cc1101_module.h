#ifndef CC1101_MODULE_H
#define CC1101_MODULE_H

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>

// =====================================================
//  PINES CC1101  (SPI compartido con SD)
// =====================================================
#define CC1101_SCK   18
#define CC1101_MISO  19
#define CC1101_MOSI  23
#define CC1101_CS     5   // <-- Pin CS de tu módulo CC1101
#define CC1101_GDO0  34   // Pin de interrupción / dato

// Pantalla compartida
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// ── Frecuencias soportadas ──
#define FREQ_315MHZ 0
#define FREQ_433MHZ 1
#define FREQ_868MHZ 2
#define FREQ_915MHZ 3

// ── Registros Básicos CC1101 ──
#define CC1101_IOCFG0   0x0B
#define CC1101_FIFOTHR  0x03
#define CC1101_PKTLEN   0x06
#define CC1101_PKTCTRL1 0x07
#define CC1101_PKTCTRL0 0x08
#define CC1101_ADDR     0x09
#define CC1101_CHANNR   0x0A
#define CC1101_FSCTRL1  0x0B
#define CC1101_FSCTRL0  0x0C
#define CC1101_FREQ2    0x0D
#define CC1101_FREQ1    0x0E
#define CC1101_FREQ0    0x0F
#define CC1101_MDMCFG4  0x10
#define CC1101_MDMCFG3  0x11
#define CC1101_MDMCFG2  0x12
#define CC1101_MDMCFG1  0x13
#define CC1101_MDMCFG0  0x14
#define CC1101_DEVIATN  0x15
#define CC1101_MCSM2    0x16
#define CC1101_MCSM1    0x17
#define CC1101_MCSM0    0x18
#define CC1101_FOCCFG   0x19
#define CC1101_BSCFG    0x1A
#define CC1101_AGCCTRL2 0x1B
#define CC1101_AGCCTRL1 0x1C
#define CC1101_AGCCTRL0 0x1D
#define CC1101_WOREVT1  0x1E
#define CC1101_WOREVT0  0x1F
#define CC1101_WORCTRL  0x20
#define CC1101_FREND1   0x21
#define CC1101_FREND0   0x22
#define CC1101_FSCAL3   0x23
#define CC1101_FSCAL2   0x24
#define CC1101_FSCAL1   0x25
#define CC1101_FSCAL0   0x26
#define CC1101_TEST2    0x2C
#define CC1101_TEST1    0x2D
#define CC1101_TEST0    0x2E

// ── Comandos strobe ──
#define CC1101_SRES   0x30   // Reset
#define CC1101_STX    0x35   // Transmitir
#define CC1101_SRX    0x34   // Recibir
#define CC1101_SIDLE  0x36   // Idle
#define CC1101_SFLUSH 0x3B   // Flush RX FIFO (SFRX)
#define CC1101_SNOP   0x3D   // No-op

// ── Registros de estado ──
#define CC1101_RXBYTES   0xFB  // Bytes en FIFO RX
#define CC1101_TXBYTES   0xFA  // Bytes en FIFO TX
#define CC1101_RXFIFO    0xFF  // Leer FIFO RX
#define CC1101_TXFIFO    0x7F  // Escribir FIFO TX
#define CC1101_PARTNUM   0xF0
#define CC1101_VERSION   0xF1
#define CC1101_RSSI      0xF4
#define CC1101_MARCSTATE 0xF5

// --- FUNCIONES PÚBLICAS ---
bool setupCC1101();
bool cc1101Detectado();
void cc1101SetFrequencia(uint8_t freqID);
String cc1101GetFreqNombre(uint8_t freqID);
bool cc1101RecibirRaw(uint8_t *buffer, uint8_t &len, uint16_t timeoutMs);
void cc1101EnviarRaw(uint8_t *buffer, uint8_t len);
void flujoCC1101(bool &dentroDeOpcion);

#endif