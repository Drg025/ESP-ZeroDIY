#include "sleep_module.h"
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_sleep.h>
#include <U8g2lib.h>

#define SLEEP_PIN 12 // GPIO para el switch de sleep (conecta a GND para dormir)

// Mandamos a llamar a la pantalla global de tu proyecto
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

void setupSleep() {
    pinMode(SLEEP_PIN, INPUT_PULLUP);
}

void enterLightSleep() {
    Serial.println("[SLEEP] Preparando modo bajo consumo...");
    
    // 1. Asesinato de antenas nativo (Ahorro masivo de batería)
    WiFi.mode(WIFI_OFF);
    btStop(); 
    
    // 2. Apagar pantalla físicamente
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    u8g2.setPowerSave(1); // Este comando apaga los píxeles OLED por completo
    
    // 3. Configurar wake-up: cuando el switch pase a HIGH
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable((gpio_num_t)SLEEP_PIN, GPIO_INTR_HIGH_LEVEL);
    
    Serial.println("[SLEEP] Entrando en light sleep... Zzz");
    delay(100);
    
    esp_light_sleep_start(); // EL PROCESADOR SE DETIENE AQUÍ
    
    // ======== AL DESPERTAR ========
    Serial.println("[SLEEP] Despertando...");
    
    // Restaurar pantalla
    u8g2.setPowerSave(0);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(15, 30, "== FLIPPER ==");
    u8g2.drawStr(25, 45, "DESPERTANDO...");
    u8g2.sendBuffer();
    delay(800);
}

void checkSleepMode() {
    // Si el switch está cerrado (LOW) -> entrar en sleep
    if (digitalRead(SLEEP_PIN) == LOW) {
        delay(200); // Anti-rebote
        
        if (digitalRead(SLEEP_PIN) == LOW) {
            enterLightSleep();
        }
    }
}