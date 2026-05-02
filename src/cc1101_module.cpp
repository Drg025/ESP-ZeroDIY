#include "cc1101_module.h"
#include "sd_module.h"

// ── Joystick (mismos pines que el resto del proyecto) ─
#define JOY_Y  35
#define JOY_SW 32

// ── Estado global ─────────────────────────────────────
static bool     cc1101Listo    = false;
static uint8_t  freqActualID   = FREQ_433MHZ;

// Señales guardadas en RAM (máx 8)
struct SignalRecord {
    uint8_t  data[64];
    uint8_t  len;
    String   nombre;
    uint8_t  freqID;
};

static SignalRecord senalesGuardadas[8];
static int          numSenales = 0;

// ══════════════════════════════════════════════════════
//  CAPA BAJO NIVEL — SPI
// ══════════════════════════════════════════════════════

static void cc1101_spi_begin() {
    SPI.begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV16); // ~5 MHz
    pinMode(CC1101_CS, OUTPUT);
    digitalWrite(CC1101_CS, HIGH);
}

static void cc1101_select() {
    digitalWrite(CC1101_CS, LOW);
    unsigned long t = millis();
    while (digitalRead(CC1101_MISO) == HIGH) {
        if (millis() - t > 50) break;
    }
}

static void cc1101_deselect() {
    digitalWrite(CC1101_CS, HIGH);
}

static uint8_t cc1101_strobe(uint8_t cmd) {
    cc1101_select();
    uint8_t status = SPI.transfer(cmd);
    cc1101_deselect();
    return status;
}

static void cc1101_writeReg(uint8_t addr, uint8_t val) {
    cc1101_select();
    SPI.transfer(addr & 0x3F);   
    SPI.transfer(val);
    cc1101_deselect();
}

static uint8_t cc1101_readReg(uint8_t addr) {
    cc1101_select();
    SPI.transfer(addr | 0x80);   
    uint8_t val = SPI.transfer(0x00);
    cc1101_deselect();
    return val;
}

static uint8_t cc1101_readStatus(uint8_t addr) {
    cc1101_select();
    SPI.transfer(addr | 0xC0);   
    uint8_t val = SPI.transfer(0x00);
    cc1101_deselect();
    return val;
}

static void cc1101_writeBurst(uint8_t addr, uint8_t *buf, uint8_t len) {
    cc1101_select();
    SPI.transfer((addr & 0x3F) | 0x40);  
    for (uint8_t i = 0; i < len; i++) {
        SPI.transfer(buf[i]);
    }
    cc1101_deselect();
}

static void cc1101_readBurst(uint8_t addr, uint8_t *buf, uint8_t len) {
    cc1101_select();
    SPI.transfer((addr | 0x80) | 0x40);  
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = SPI.transfer(0x00);
    }
    cc1101_deselect();
}

// ══════════════════════════════════════════════════════
//  CONFIGURACIÓN DE REGISTROS
// ══════════════════════════════════════════════════════

static const uint8_t FREQ_TABLE[4][3] = {
    {0x0C, 0x1D, 0x89},   // 315 MHz
    {0x10, 0xA7, 0x62},   // 433.92 MHz
    {0x21, 0x62, 0x76},   // 868.35 MHz
    {0x23, 0x31, 0x3B},   // 915 MHz
};

static void cc1101_applyConfig() {
    cc1101_writeReg(CC1101_IOCFG0,   0x0D); 
    cc1101_writeReg(CC1101_FIFOTHR,  0x47);
    cc1101_writeReg(CC1101_PKTCTRL0, 0x00); 
    cc1101_writeReg(CC1101_PKTLEN,   0x3D); 

    cc1101_writeReg(CC1101_FSCTRL1,  0x06);
    cc1101_writeReg(CC1101_FSCTRL0,  0x00);

    const uint8_t *f = FREQ_TABLE[freqActualID];
    cc1101_writeReg(CC1101_FREQ2, f[0]);
    cc1101_writeReg(CC1101_FREQ1, f[1]);
    cc1101_writeReg(CC1101_FREQ0, f[2]);

    cc1101_writeReg(CC1101_MDMCFG4, 0xF5); 
    cc1101_writeReg(CC1101_MDMCFG3, 0x83); 
    cc1101_writeReg(CC1101_MDMCFG2, 0x30); 
    cc1101_writeReg(CC1101_MDMCFG1, 0x22);
    cc1101_writeReg(CC1101_MDMCFG0, 0xF8);

    cc1101_writeReg(CC1101_DEVIATN,  0x15);
    cc1101_writeReg(CC1101_MCSM0,   0x18); 
    cc1101_writeReg(CC1101_FOCCFG,  0x14);
    cc1101_writeReg(CC1101_AGCCTRL2, 0x43);
    cc1101_writeReg(CC1101_AGCCTRL1, 0x49);
    cc1101_writeReg(CC1101_AGCCTRL0, 0x91);
    cc1101_writeReg(CC1101_FREND1,   0x56);
    cc1101_writeReg(CC1101_FREND0,   0x11);  
    cc1101_writeReg(CC1101_FSCAL3,   0xE9);
    cc1101_writeReg(CC1101_FSCAL2,   0x2A);
    cc1101_writeReg(CC1101_FSCAL1,   0x00);
    cc1101_writeReg(CC1101_FSCAL0,   0x1F);
    cc1101_writeReg(CC1101_TEST2,    0x81);
    cc1101_writeReg(CC1101_TEST1,    0x35);
    cc1101_writeReg(CC1101_TEST0,    0x09);

    uint8_t patable[8] = {0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cc1101_select();
    SPI.transfer(0x7E | 0x40);  
    for (uint8_t i = 0; i < 8; i++) {
        SPI.transfer(patable[i]);
    }
    cc1101_deselect();
}

// ══════════════════════════════════════════════════════
//  API PÚBLICA — SETUP
// ══════════════════════════════════════════════════════

bool setupCC1101() {
    cc1101_spi_begin();
    delay(10);
    cc1101_strobe(CC1101_SRES);
    delay(10);

    uint8_t partnum = cc1101_readStatus(CC1101_PARTNUM);
    uint8_t version = cc1101_readStatus(CC1101_VERSION);

    Serial.printf("[CC1101] PARTNUM=0x%02X  VERSION=0x%02X\n", partnum, version);

    if (version == 0x00 || version == 0xFF) {
        Serial.println("[CC1101] Módulo no detectado — revisa el cableado");
        cc1101Listo = false;
        return false;
    }

    cc1101_applyConfig();
    cc1101Listo = true;
    Serial.println("[CC1101] Listo");
    return true;
}

bool cc1101Detectado() { 
    return cc1101Listo; 
}

void cc1101SetFrequencia(uint8_t freqID) {
    if (freqID > FREQ_915MHZ) freqID = FREQ_433MHZ;
    freqActualID = freqID;
    
    if (!cc1101Listo) return;
    
    const uint8_t *f = FREQ_TABLE[freqID];
    cc1101_strobe(CC1101_SIDLE);
    cc1101_writeReg(CC1101_FREQ2, f[0]);
    cc1101_writeReg(CC1101_FREQ1, f[1]);
    cc1101_writeReg(CC1101_FREQ0, f[2]);
}

String cc1101GetFreqNombre(uint8_t freqID) {
    switch (freqID) {
        case FREQ_315MHZ: return "315 MHz";
        case FREQ_433MHZ: return "433 MHz";
        case FREQ_868MHZ: return "868 MHz";
        case FREQ_915MHZ: return "915 MHz";
        default:          return "???";
    }
}

// ══════════════════════════════════════════════════════
//  API PÚBLICA — RECIBIR SEÑAL RAW
// ══════════════════════════════════════════════════════
bool cc1101RecibirRaw(uint8_t *buffer, uint8_t &len, uint16_t timeoutMs) {
    if (!cc1101Listo) return false;

    cc1101_strobe(CC1101_SIDLE);
    delayMicroseconds(100);
    cc1101_strobe(CC1101_SFLUSH); 
    delayMicroseconds(100);
    cc1101_strobe(CC1101_SRX);     

    unsigned long inicio = millis();
    len = 0;

    while (millis() - inicio < timeoutMs) {
        uint8_t bytesDisponibles = cc1101_readStatus(CC1101_RXBYTES) & 0x7F;
        
        if (bytesDisponibles > 0) {
            uint8_t toRead = bytesDisponibles;
            if (len + toRead > 64) {
                toRead = 64 - len;
            }
            cc1101_readBurst(CC1101_RXFIFO & 0x3F, buffer + len, toRead);
            len += toRead;
            if (len >= 64) break;
            delay(20);
        }
        
        if (digitalRead(JOY_SW) == LOW) {
            break;  
        }
        delay(5);
    }

    cc1101_strobe(CC1101_SIDLE);
    return (len > 0);
}

// ══════════════════════════════════════════════════════
//  API PÚBLICA — ENVIAR SEÑAL RAW
// ══════════════════════════════════════════════════════
void cc1101EnviarRaw(uint8_t *buffer, uint8_t len) {
    if (!cc1101Listo || len == 0) return;

    cc1101_strobe(CC1101_SIDLE);
    delayMicroseconds(100);
    
    cc1101_writeReg(CC1101_PKTLEN, len);
    cc1101_writeBurst(CC1101_TXFIFO & 0x3F, buffer, len);
    cc1101_strobe(CC1101_STX);

    unsigned long t = millis();
    while (millis() - t < 2000) {
        uint8_t state = cc1101_readStatus(CC1101_MARCSTATE) & 0x1F;
        if (state == 0x01) {
            break;  
        }
        delay(5);
    }
    cc1101_strobe(CC1101_SIDLE);
}

// ══════════════════════════════════════════════════════
//  HELPERS DE PANTALLA
// ══════════════════════════════════════════════════════

static void mostrarMensaje(const char *linea1, const char *linea2 = "", uint16_t ms = 1500) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(10, 25, linea1);
    
    if (strlen(linea2) > 0) {
        u8g2.drawStr(10, 40, linea2);
    }
    
    u8g2.sendBuffer();
    delay(ms);
}

static int menuGenerico(const char *titulo, String opciones[], int numOpc) {
    int idx = 0;
    while(digitalRead(JOY_SW) == LOW) {
        delay(10); // Anti-rebote
    }
    
    while (true) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(5, 10, titulo);
        u8g2.drawLine(0, 13, 128, 13);
        
        for (int i = 0; i < numOpc && i < 4; i++) {
            int y = 24 + i * 12;
            if (i == idx) {
                u8g2.drawStr(0, y, ">");
            }
            u8g2.setCursor(10, y);
            u8g2.print(opciones[i]);
        }
        u8g2.sendBuffer();

        int vy = analogRead(JOY_Y);
        if (vy < 1000) { 
            if (idx > 0) idx--; 
            delay(200); 
        }
        if (vy > 3000) { 
            if (idx < numOpc - 1) idx++; 
            delay(200); 
        }

        if (digitalRead(JOY_SW) == LOW) { 
            delay(300); 
            return idx; 
        }
        delay(30);
    }
}

// ══════════════════════════════════════════════════════
//  SUB-FLUJOS
// ══════════════════════════════════════════════════════

static void flujoCapturar() {
    if (numSenales >= 8) { 
        mostrarMensaje("Memoria llena", "Borra una senal"); 
        return; 
    }
    
    String freqs[4] = {"315 MHz", "433 MHz", "868 MHz", "915 MHz"};
    int fSel = menuGenerico("FRECUENCIA:", freqs, 4);
    cc1101SetFrequencia((uint8_t)fSel);

    u8g2.clearBuffer(); 
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(5, 15, "CAPTURANDO..."); 
    u8g2.setCursor(5, 30); 
    u8g2.print(cc1101GetFreqNombre(fSel));
    u8g2.drawStr(5, 48, "Acerca el mando"); 
    u8g2.drawStr(5, 62, "Click = cancelar"); 
    u8g2.sendBuffer();

    uint8_t buf[64]; 
    uint8_t len = 0;
    bool ok = cc1101RecibirRaw(buf, len, 5000);

    if (!ok || len == 0) { 
        mostrarMensaje("Sin senal", "Intenta de nuevo"); 
        return; 
    }

    String nombreUser = tecladoNombre("CC1101");
    SignalRecord &rec = senalesGuardadas[numSenales];
    
    memcpy(rec.data, buf, len);
    rec.len = len; 
    rec.freqID = (uint8_t)fSel; 
    rec.nombre = nombreUser;
    
    numSenales++;

    // Guardado automático en la Tarjeta SD con las dependencias actualizadas
    guardarDato("CC1101", nombreUser, "HEX_CAPTURADO"); 
    
    u8g2.clearBuffer(); 
    u8g2.drawStr(5, 12, "SENAL CAPTURADA!"); 
    u8g2.drawLine(0, 15, 128, 15);
    u8g2.setCursor(5, 27); 
    u8g2.printf("Frec: %s", cc1101GetFreqNombre(fSel).c_str());
    u8g2.setCursor(5, 39); 
    u8g2.printf("Bytes: %d", len);
    u8g2.drawStr(5, 63, "Click para cont."); 
    u8g2.sendBuffer();
    
    while (digitalRead(JOY_SW) == HIGH) {
        delay(100);
    }
    delay(300);
}

static void flujoEnviar() {
    if (numSenales == 0) { 
        mostrarMensaje("No hay senales", "Captura una primero"); 
        return; 
    }
    
    String nombres[8]; 
    for (int i = 0; i < numSenales; i++) {
        nombres[i] = senalesGuardadas[i].nombre;
    }
    
    int sel = menuGenerico("ENVIAR SENAL:", nombres, numSenales);

    SignalRecord &rec = senalesGuardadas[sel];
    cc1101SetFrequencia(rec.freqID);

    u8g2.clearBuffer(); 
    u8g2.drawStr(10, 25, "Enviando..."); 
    u8g2.setCursor(10, 40); 
    u8g2.print(rec.nombre); 
    u8g2.sendBuffer();
    
    cc1101EnviarRaw(rec.data, rec.len);
    mostrarMensaje("Senal enviada!", "");
}

static void flujoVerSenales() {
    if (numSenales == 0) { 
        mostrarMensaje("Sin senales", ""); 
        return; 
    }
    
    int idx = 0; 
    bool viendo = true;
    
    while (viendo) {
        SignalRecord &r = senalesGuardadas[idx];
        u8g2.clearBuffer(); 
        u8g2.setFont(u8g2_font_6x10_tf); 
        u8g2.drawStr(5, 10, "MIS SENALES:"); 
        u8g2.drawLine(0, 13, 128, 13);
        u8g2.setCursor(5, 24); 
        u8g2.printf("%d/%d  %s", idx + 1, numSenales, r.nombre.c_str());
        u8g2.setCursor(5, 36); 
        u8g2.printf("Frec: %s", cc1101GetFreqNombre(r.freqID).c_str());
        u8g2.setCursor(5, 48); 
        u8g2.printf("Bytes: %d", r.len);
        u8g2.sendBuffer();

        int vy = analogRead(JOY_Y);
        if (vy < 1000) { 
            if (idx > 0) idx--; 
            delay(200); 
        }
        if (vy > 3000) { 
            if (idx < numSenales - 1) idx++; 
            delay(200); 
        }
        if (digitalRead(JOY_SW) == LOW) { 
            delay(300); 
            viendo = false; 
        }
        delay(30);
    }
}

static void flujoGuardarSD() {
    // Reemplazado por el guardado automático del teclado
    mostrarMensaje("La senal ya se", "guardo al capturar."); 
}

static void flujoBorrar() {
    if (numSenales == 0) { 
        mostrarMensaje("Sin senales", ""); 
        return; 
    }
    
    String opts[2] = {"Si, borrar todo", "Cancelar"};
    int r = menuGenerico("BORRAR TODO?", opts, 2);
    
    if (r == 0) { 
        numSenales = 0; 
        mostrarMensaje("Senales borradas", ""); 
    }
}

static void flujoEscanear() {
    String freqs[4] = {"315 MHz", "433 MHz", "868 MHz", "915 MHz"};
    int fSel = menuGenerico("FRECUENCIA:", freqs, 4);
    cc1101SetFrequencia((uint8_t)fSel);
    
    bool escaneando = true; 
    int8_t maxRSSI = -120;

    while (escaneando) {
        cc1101_strobe(CC1101_SIDLE); 
        delayMicroseconds(200); 
        cc1101_strobe(CC1101_SRX); 
        delay(5);
        
        int8_t raw = (int8_t)cc1101_readStatus(CC1101_RSSI);
        int dBm = (raw >= 128) ? ((raw - 256) / 2 - 74) : (raw / 2 - 74);
        
        if (dBm > maxRSSI) {
            maxRSSI = dBm;
        }
        
        int barWidth = constrain(map(dBm, -100, -20, 0, 118), 0, 118);

        u8g2.clearBuffer(); 
        u8g2.drawStr(5, 12, "ESCANER RSSI"); 
        u8g2.drawLine(0, 15, 128, 15);
        u8g2.setCursor(5, 28); 
        u8g2.printf("Frec: %s", cc1101GetFreqNombre(fSel).c_str());
        u8g2.setCursor(5, 40); 
        u8g2.printf("RSSI: %d dBm", dBm);
        u8g2.drawFrame(5, 55, 118, 8); 
        
        if (barWidth > 0) {
            u8g2.drawBox(5, 55, barWidth, 8);
        }
        
        u8g2.drawStr(5, 64, "Click = salir"); 
        u8g2.sendBuffer();

        if (digitalRead(JOY_SW) == LOW) { 
            delay(300); 
            escaneando = false; 
        }
        delay(100);
    }
    cc1101_strobe(CC1101_SIDLE);
}

// ══════════════════════════════════════════════════════
//  NUEVO: FUERZA BRUTA (BRUTE-FORCER SUB-GHz) ANTI-CONGELAMIENTO
// ══════════════════════════════════════════════════════
static void flujoBruteForce() {
    String freqs[4] = {"315 MHz", "433 MHz", "868 MHz", "915 MHz"};
    int fSel = menuGenerico("FREC FUERZA BRUTA:", freqs, 4);
    cc1101SetFrequencia((uint8_t)fSel);

    String bitsOpc[2] = {"12-Bits (Clasico)", "24-Bits"};
    int bitsSel = menuGenerico("TIPO DE ATAQUE:", bitsOpc, 2);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 15, "ATACANDO...");
    u8g2.drawStr(0, 60, "Click = DETENER");
    u8g2.sendBuffer();

    while(digitalRead(JOY_SW) == LOW) {
        delay(10); // Anti-rebote
    }

    int limite = (bitsSel == 0) ? 4096 : 16777215; // 12-bit max or 24-bit max
    int step = (bitsSel == 0) ? 1 : 256; // Para 24 bits saltamos rápido
    uint8_t buf[3];
    bool atacando = true;

    for (int i = 0; i < limite && atacando; i += step) {
        // Preparar payload binario
        buf[0] = (i >> 16) & 0xFF;
        buf[1] = (i >> 8) & 0xFF;
        buf[2] = i & 0xFF;

        cc1101EnviarRaw(buf, 3); // Disparo RF

        // === EL SECRETO ANTI-CONGELAMIENTO ===
        // Actualizamos la pantalla solo cada 16 disparos
        if (i % 16 == 0) {
            u8g2.setDrawColor(0);
            u8g2.drawBox(0, 20, 128, 30); // Limpia zona del texto
            u8g2.setDrawColor(1);
            u8g2.setCursor(5, 35);
            u8g2.printf("PIN: %04d / %d", i, limite);

            int barW = map(i, 0, limite, 0, 128);
            u8g2.drawBox(0, 45, barW, 5); // Barra de progreso
            u8g2.sendBuffer();
        }

        // Leer botón de escape urgente continuamente
        if (digitalRead(JOY_SW) == LOW) {
            atacando = false;
            delay(300);
        }
    }
    mostrarMensaje(atacando ? "Ataque Completado" : "Ataque Cancelado");
}

// ══════════════════════════════════════════════════════
//  FLUJO PRINCIPAL DE UI
// ══════════════════════════════════════════════════════
void flujoCC1101(bool &dentroDeOpcion) {
    if (!cc1101Listo) {
        mostrarMensaje("CC1101 no detectado", "Revisa cableado");
        dentroDeOpcion = false;
        return;
    }

    const int  NUM_OPC = 8; 
    String     opts[NUM_OPC] = {
        "1. Capturar",
        "2. Enviar",
        "3. Ver senales",
        "4. Guardar SD",
        "5. Borrar RAM",
        "6. Escanear",
        "7. Fuerza Bruta", 
        "8. Regresar"
    };
    
    int  idx        = 0;
    bool enFlujo    = true;

    while (enFlujo) {
        u8g2.clearBuffer(); 
        u8g2.setFont(u8g2_font_6x12_tr);
        
        // Título más corto y separado de la frecuencia
        u8g2.drawStr(2, 10, "CC1101"); 
        u8g2.drawLine(0, 12, 128, 12); 
        u8g2.setCursor(80, 10); 
        u8g2.print(cc1101GetFreqNombre(freqActualID));

        // Mostrar SOLO 3 opciones a la vez para que respiren en la pantalla OLED
        int start = (idx >= 3) ? idx - 2 : 0;
        for (int i = 0; i < 3; i++) {
            int op = start + i; 
            if (op >= NUM_OPC) break;
            
            int yPos = 25 + (i * 12); // Coordenadas perfectas: 25, 37 y 49
            if (op == idx) {
                u8g2.drawStr(0, yPos, ">");
            }
            u8g2.setCursor(10, yPos); 
            u8g2.print(opts[op]);
        }

        // Barra de estado hasta abajo del todo (sin estorbar a las opciones)
        u8g2.setCursor(2, 63); 
        u8g2.printf("Senales RAM: %d/8", numSenales); 
        
        u8g2.sendBuffer();

        int vy = analogRead(JOY_Y);
        if (vy < 1000) { 
            if (idx > 0) idx--;           
            delay(200); 
        }
        if (vy > 3000) { 
            if (idx < NUM_OPC - 1) idx++; 
            delay(200); 
        }

        if (digitalRead(JOY_SW) == LOW) {
            delay(300);
            switch (idx) {
                case 0: flujoCapturar();  break;
                case 1: flujoEnviar();    break;
                case 2: flujoVerSenales();break;
                case 3: flujoGuardarSD(); break;
                case 4: flujoBorrar();    break;
                case 5: flujoEscanear();  break;
                case 6: flujoBruteForce();break; 
                case 7: enFlujo = false; dentroDeOpcion = false; break;
            }
        }
        delay(30);
    }
}