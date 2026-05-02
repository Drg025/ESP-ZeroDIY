#include "nfc_module.h"
#include "sd_module.h"
#include "interface.h" 

#define JOY_Y  35
#define JOY_SW 32

#define PN532_IRQ   (21)  
#define PN532_RESET (5) 

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
uint8_t uidLeido[7];
uint8_t uidLongitud = 0;
String uidString = "";
uint8_t keyDefault[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // Clave por defecto Mifare

void setupNFC() {
    nfc.begin();               
    uint32_t version = nfc.getFirmwareVersion();
    if (!version) {
        Serial.println("Error: PN532 no encontrado.");
        return;               
    }
    nfc.SAMConfig();             
    Serial.println("NFC listo para el combate.");
}

bool capturarUID() {
    uidString = "";
    uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uidLeido, &uidLongitud, 100);
    
    if (success) {
        for (uint8_t i = 0; i < uidLongitud; i++) {
            if (uidLeido[i] < 0x10) uidString += "0"; 
            uidString += String(uidLeido[i], HEX);
        }
        uidString.toUpperCase();
        return true;
    }
    return false;
}

String obtenerUltimoUID() {
    return uidString;
}

// NUEVA: Volcado de memoria (Dump) de Mifare Classic 1K
void volcarMemoria() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(10, 20, "Extrayendo datos...");
    u8g2.drawStr(10, 40, "No muevas la tarjeta");
    u8g2.sendBuffer();

    String dumpCompleto = "UID: " + uidString + "\n";
    bool volcadoExitoso = false;

    // Intentamos leer los primeros 16 bloques (Sectores 0 a 3)
    for (uint8_t bloque = 0; bloque < 16; bloque++) {
        if (nfc.mifareclassic_AuthenticateBlock(uidLeido, uidLongitud, bloque, 0, keyDefault)) {
            uint8_t datos[16];
            if (nfc.mifareclassic_ReadDataBlock(bloque, datos)) {
                dumpCompleto += "Blk " + String(bloque) + ": ";
                for (uint8_t i = 0; i < 16; i++) {
                    if (datos[i] < 0x10) dumpCompleto += "0";
                    dumpCompleto += String(datos[i], HEX) + " ";
                }
                dumpCompleto += "\n";
                volcadoExitoso = true;
            }
        }
    }

    if (volcadoExitoso) {
        String nombreUser = tecladoNombre("DUMP");
        guardarDato("NFC", nombreUser + "_dump", dumpCompleto);
        u8g2.clearBuffer(); u8g2.drawStr(10, 30, "Dump Guardado en SD!"); u8g2.sendBuffer();
    } else {
        u8g2.clearBuffer(); u8g2.drawStr(10, 30, "Error: Sectores Cifrados"); u8g2.sendBuffer();
    }
    delay(2000);
}

bool clonarTarjeta() {
    uint8_t uidDestino[7];
    uint8_t uidLenDestino;

    // Esperar una tarjeta virgen (Magic Card Gen 1a)
    if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uidDestino, &uidLenDestino, 5000)) {
        return false; 
    }
    if (!nfc.mifareclassic_AuthenticateBlock(uidDestino, uidLenDestino, 0, 0, keyDefault)) {
        return false; 
    }

    // Preparar el Bloque 0 (UID + BCC + Datos del fabricante)
    uint8_t bloque0[16] = {0};
    for(int i = 0; i < 4; i++) {
        bloque0[i] = uidLeido[i];
    }
    bloque0[4] = uidLeido[0] ^ uidLeido[1] ^ uidLeido[2] ^ uidLeido[3]; // Cálculo del BCC
    bloque0[5] = 0x08; // SAK
    bloque0[6] = 0x04; // ATQA
    bloque0[7] = 0x00; 

    return nfc.mifareclassic_WriteDataBlock(0, bloque0);
}

void emularTarjeta() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(10, 20, "EMULANDO ID (BETA)");
    u8g2.setCursor(10, 40);
    u8g2.print("ID: " + uidString);
    u8g2.drawStr(10, 60, "(Click para salir)");
    u8g2.sendBuffer();

    while (digitalRead(JOY_SW) == HIGH) {
        // La emulación real requiere cambiar a la librería Elechouse PN532.
        // Aquí iría el comando: nfc.tgInitAsTarget();
        delay(100); 
    }
    nfc.SAMConfig();
}

void flujoCapturaRFID(bool &dentroDeOpcion) {
    bool enMenuRaiz = true;
    String menuRaiz[3] = {"1. Escanear Tag", "2. Cargar de SD", "3. Salir"};

    while (enMenuRaiz) {
        int indexRaiz = 0;
        bool raizSeleccionada = false;

        while (!raizSeleccionada) {
            int valorY = analogRead(JOY_Y);
            if (valorY < 1000) { if (indexRaiz > 0) indexRaiz--; delay(200); }
            if (valorY > 3000) { if (indexRaiz < 2) indexRaiz++; delay(200); }
            if (digitalRead(JOY_SW) == LOW) { raizSeleccionada = true; delay(300); }

            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x12_tr);
            u8g2.drawStr(2, 10, "RED TEAM - NFC");
            u8g2.drawLine(0, 12, 128, 12);

            for (int i = 0; i < 3; i++) {
                int yPos = 26 + (i * 12);
                if (i == indexRaiz) u8g2.drawStr(0, yPos, ">");
                u8g2.setCursor(10, yPos);
                u8g2.print(menuRaiz[i]);
            }
            u8g2.sendBuffer();
        }

        if (indexRaiz == 2) {
            enMenuRaiz = false;
            dentroDeOpcion = false;
            return;
        }

        bool idListoParaUsar = false;

        if (indexRaiz == 0) {
            u8g2.clearBuffer();
            u8g2.drawStr(10, 30, "Acerca el Tag...");
            u8g2.drawStr(10, 50, "(Click cancelar)");
            u8g2.sendBuffer();

            bool leida = false;
            while (!leida) {
                leida = capturarUID();
                if (digitalRead(JOY_SW) == LOW) { delay(300); break; }
            }
            if(leida) idListoParaUsar = true;

        } else if (indexRaiz == 1) {
            // LÓGICA DE CARGA DESDE SD (Requiere integrar lectura de archivos)
            u8g2.clearBuffer();
            u8g2.drawStr(10, 30, "Cargando de SD...");
            u8g2.sendBuffer();
            delay(1500);
            if (uidLongitud > 0) idListoParaUsar = true;
        }

        if (idListoParaUsar) {
            bool enMenuAccion = true;
            String opcionesAccion[5] = {"1. Volcar Sectores", "2. Clonar a Virgen", "3. Guardar UID a SD", "4. Emular UID", "5. Cerrar"};
            
            while (enMenuAccion) {
                int indexAccion = 0;
                bool accionSel = false;
                
                while (!accionSel) {
                    int valorY = analogRead(JOY_Y);
                    if (valorY < 1000) { if (indexAccion > 0) indexAccion--; delay(200); }
                    if (valorY > 3000) { if (indexAccion < 4) indexAccion++; delay(200); }
                    if (digitalRead(JOY_SW) == LOW) { accionSel = true; delay(300); }

                    u8g2.clearBuffer();
                    u8g2.setCursor(0, 10);
                    u8g2.print("TGT: " + uidString);
                    u8g2.drawLine(0, 14, 128, 14);

                    for (int i = 0; i < 5; i++) {
                        int yPos = 24 + (i * 10);
                        if (i == indexAccion) u8g2.drawStr(0, yPos, ">");
                        u8g2.setCursor(10, yPos);
                        u8g2.print(opcionesAccion[i]);
                    }
                    u8g2.sendBuffer();
                }

                if (indexAccion == 0) {
                    volcarMemoria();
                } else if (indexAccion == 1) {
                    u8g2.clearBuffer(); u8g2.drawStr(10, 30, "Acerca TAG virgen.."); u8g2.sendBuffer();
                    if (clonarTarjeta()) {
                        u8g2.clearBuffer(); u8g2.drawStr(20, 30, "Clonado OK!"); u8g2.sendBuffer();
                    } else {
                        u8g2.clearBuffer(); u8g2.drawStr(20, 30, "Fallo al clonar"); u8g2.sendBuffer();
                    }
                    delay(1500);
                } else if (indexAccion == 2) {
                    String nombreUser = tecladoNombre("NFC");
                    guardarDato("NFC", nombreUser, uidString);
                    u8g2.clearBuffer(); u8g2.drawStr(10, 30, "UID Guardado!"); u8g2.sendBuffer();
                    delay(1500);
                } else if (indexAccion == 3) {
                    emularTarjeta();
                } else if (indexAccion == 4) {
                    enMenuAccion = false; 
                }
            }
        }
    }
}