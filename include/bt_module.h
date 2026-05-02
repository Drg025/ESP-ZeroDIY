#ifndef BT_MODULE_H
#define BT_MODULE_H

#include <Arduino.h>

void setupBluetooth();
void flujoBluetooth(bool &dentroDeOpcion);
void scanBluetoothNetworks();
void testBluetoothConnection();
void scanBLENetworks();
void bleSpamReal();
void macSpoofingReal();

#endif // BT_MODULE_H