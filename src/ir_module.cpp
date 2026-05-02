#include "ir_module.h"
#include "sd_module.h" 
#include "interface.h"
#include "joystick_module.h"

#include <ir_Gree.h>    
#include <ir_Midea.h>   
#include <ir_LG.h>      
#include <ir_Samsung.h> 

IRrecv irrecv(IR_RX_PIN);
IRsend irsend(IR_TX_PIN);
decode_results results;

#define JOY_Y  35
#define JOY_SW 32

void modoClonarIR(bool &dentroDeOpcion);
void ataqueTVUniversal();
void menuTVMarcas(bool &dentroDeOpcion); 
void menuAccionTV(int marcaID);          
void ejecutarAtaqueTV(int marcaID, int accionID); 
void menuACMarcas(bool &dentroDeOpcion); 
void menuAccionAC(int marcaID);          
void ejecutarAtaqueAC(int marcaID, bool encender); 
void mostrarOpcionesIR(String codigoHex, uint32_t codigoCrudo, bool &dentroDeOpcion);
bool esperarO_Salir(int tiempoMs); 
void enviarCodigoIR(uint32_t codigo); 

void setupIR() { irrecv.enableIRIn(); irsend.begin(); }

bool esperarO_Salir(int tiempoMs) {
    unsigned long inicio = millis();
    while (millis() - inicio < (unsigned long)tiempoMs) {
        if (digitalRead(JOY_SW) == LOW) { delay(200); return true; }
        delay(5); 
    }
    return false; 
}

void flujoInfrarrojo(bool &dentroDeOpcion) {
    bool enMenuIR = true; int subIndice = 0; const int SUB_ITEMS = 5; 
    String subMenu[SUB_ITEMS] = {"1. Clonar/Sniffer", "2. TV Rafaga", "3. TV x Marcas", "4. AC x Marcas", "5. Regresar"};

    while (enMenuIR) {
        dibujarPantalla(subIndice, false, subMenu, SUB_ITEMS);
        int valorY = analogRead(JOY_Y);
        if (valorY < 1000) { if (subIndice > 0) subIndice--; delay(150); }
        if (valorY > 3000) { if (subIndice < SUB_ITEMS - 1) subIndice++; delay(150); }

        if (digitalRead(JOY_SW) == LOW) {
            delay(200); 
            if (subIndice == 0) { bool enClonador = true; modoClonarIR(enClonador); } 
            else if (subIndice == 1) ataqueTVUniversal();
            else if (subIndice == 2) { bool enMarcasTV = true; menuTVMarcas(enMarcasTV); }
            else if (subIndice == 3) { bool enMarcasAC = true; menuACMarcas(enMarcasAC); }
            else if (subIndice == 4) enMenuIR = false; 
        }
    }
    dentroDeOpcion = false;
}

void menuTVMarcas(bool &dentroDeOpcion) {
    bool enMenuMarcas = true; int indiceMarca = 0; const int NUM_MARCAS = 5;
    String listaMarcas[NUM_MARCAS] = {"1. LG", "2. Samsung", "3. Sony", "4. Panasonic", "5. Regresar"};
    while(digitalRead(JOY_SW) == LOW) delay(10); 
    while (enMenuMarcas) {
        dibujarPantalla(indiceMarca, true, listaMarcas, NUM_MARCAS);
        int valorY = analogRead(JOY_Y);
        if (valorY < 1000) { if (indiceMarca > 0) indiceMarca--; delay(150); }
        if (valorY > 3000) { if (indiceMarca < NUM_MARCAS - 1) indiceMarca++; delay(150); }
        if (digitalRead(JOY_SW) == LOW) {
            delay(200);
            if (indiceMarca == 4) enMenuMarcas = false; 
            else menuAccionTV(indiceMarca); 
        }
    }
}

void menuAccionTV(int marcaID) {
    bool enAccion = true; int indiceAccion = 0; const int NUM_ACCIONES = 5;
    String listaAcciones[NUM_ACCIONES] = {"1. Power", "2. Vol +", "3. Vol -", "4. Mute", "5. Regresar"};
    while(digitalRead(JOY_SW) == LOW) delay(10); 
    while (enAccion) {
        dibujarPantalla(indiceAccion, true, listaAcciones, NUM_ACCIONES);
        int valorY = analogRead(JOY_Y);
        if (valorY < 1000) { if (indiceAccion > 0) indiceAccion--; delay(150); }
        if (valorY > 3000) { if (indiceAccion < NUM_ACCIONES - 1) indiceAccion++; delay(150); }
        if (digitalRead(JOY_SW) == LOW) {
            delay(200);
            if (indiceAccion == 4) enAccion = false;
            else ejecutarAtaqueTV(marcaID, indiceAccion); 
        }
    }
}

void ejecutarAtaqueTV(int marcaID, int accionID) {
    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0, 15, "DISPARANDO..."); u8g2.sendBuffer();
    for(int i = 0; i < 3; i++) {
        if (marcaID == 0) { 
            if (accionID == 0) irsend.sendNEC(0x20DF10EF, 32); else if (accionID == 1) irsend.sendNEC(0x20DF40BF, 32); 
            else if (accionID == 2) irsend.sendNEC(0x20DFC03F, 32); else if (accionID == 3) irsend.sendNEC(0x20DF906F, 32); 
        }
        else if (marcaID == 1) {
            if (accionID == 0) irsend.sendSAMSUNG(0xE0E040BF, 32); else if (accionID == 1) irsend.sendSAMSUNG(0xE0E0E01F, 32); 
            else if (accionID == 2) irsend.sendSAMSUNG(0xE0E0D02F, 32); else if (accionID == 3) irsend.sendSAMSUNG(0xE0E0F00F, 32); 
        }
        else if (marcaID == 2) {
            if (accionID == 0) irsend.sendSony(0xA90, 12, 2); else if (accionID == 1) irsend.sendSony(0x490, 12, 2); 
            else if (accionID == 2) irsend.sendSony(0xC90, 12, 2); else if (accionID == 3) irsend.sendSony(0x290, 12, 2); 
        }
        else if (marcaID == 3) {
            if (accionID == 0) irsend.sendPanasonic(0x4004, 0x100BCBD); else if (accionID == 1) irsend.sendPanasonic(0x4004, 0x1000405); 
            else if (accionID == 2) irsend.sendPanasonic(0x4004, 0x1008485); else if (accionID == 3) irsend.sendPanasonic(0x4004, 0x1004C4D); 
        }
        delay(40);
    }
}

void menuACMarcas(bool &dentroDeOpcion) {
    bool enMenuMarcas = true; int indiceMarca = 0; const int NUM_MARCAS = 5;
    String listaMarcas[NUM_MARCAS] = {"1. Trane/Gree", "2. Mirage/Midea", "3. LG", "4. Samsung", "5. Regresar"};
    while(digitalRead(JOY_SW) == LOW) delay(10); 
    while (enMenuMarcas) {
        dibujarPantalla(indiceMarca, true, listaMarcas, NUM_MARCAS);
        int valorY = analogRead(JOY_Y);
        if (valorY < 1000) { if (indiceMarca > 0) indiceMarca--; delay(150); }
        if (valorY > 3000) { if (indiceMarca < NUM_MARCAS - 1) indiceMarca++; delay(150); }
        if (digitalRead(JOY_SW) == LOW) { delay(200); if (indiceMarca == 4) enMenuMarcas = false; else menuAccionAC(indiceMarca); }
    }
}

void menuAccionAC(int marcaID) {
    bool enAccion = true; int indiceAccion = 0; const int NUM_ACCIONES = 3;
    String listaAcciones[NUM_ACCIONES] = {"1. Encender", "2. Apagar", "3. Regresar"};
    while(digitalRead(JOY_SW) == LOW) delay(10); 
    while (enAccion) {
        dibujarPantalla(indiceAccion, true, listaAcciones, NUM_ACCIONES);
        int valorY = analogRead(JOY_Y);
        if (valorY < 1000) { if (indiceAccion > 0) indiceAccion--; delay(150); }
        if (valorY > 3000) { if (indiceAccion < NUM_ACCIONES - 1) indiceAccion++; delay(150); }
        if (digitalRead(JOY_SW) == LOW) { delay(200); if (indiceAccion == 2) enAccion = false; else ejecutarAtaqueAC(marcaID, indiceAccion == 0); }
    }
}

// CORRECCIÓN DE SINTAXIS PARA LIBRERÍAS DE AC (IRLgAc, kMideaACCool, etc)
void ejecutarAtaqueAC(int marcaID, bool encender) {
    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0, 15, encender ? "ENCENDIENDO..." : "APAGANDO..."); u8g2.sendBuffer();

    if (marcaID == 0) { 
        IRGreeAC ac(IR_TX_PIN); ac.begin();
        if(encender) { ac.on(); ac.setTemp(20); ac.setMode(kGreeCool); ac.setFan(kGreeFanMax); } else ac.off();
        ac.send();
    } 
    else if (marcaID == 1) { 
        IRMideaAC ac(IR_TX_PIN); ac.begin();
        if(encender) { ac.on(); ac.setTemp(20); ac.setMode(kMideaACCool); ac.setFan(kMideaACFanHigh); } else ac.off();
        ac.send();
    } 
    else if (marcaID == 2) { 
        IRLgAc ac(IR_TX_PIN); ac.begin();
        if(encender) { ac.on(); ac.setTemp(20); ac.setMode(kLgAcCool); ac.setFan(kLgAcFanHigh); } else ac.off();
        ac.send();
    } 
    else if (marcaID == 3) { 
        IRSamsungAc ac(IR_TX_PIN); ac.begin();
        if(encender) { ac.on(); ac.setTemp(20); ac.setMode(kSamsungAcCool); ac.setFan(kSamsungAcFanHigh); } else ac.off();
        ac.send();
    }
    delay(1000);
}

bool recibirCodigoIR(String &resultadoHex, uint32_t &codigoCrudo) {
    if (irrecv.decode(&results)) {
        resultadoHex = resultToHexidecimal(&results);
        codigoCrudo = (uint32_t)results.value;
        irrecv.resume(); return true;
    }
    return false;
}
void enviarCodigoIR(uint32_t codigo) { irsend.sendNEC(codigo, 32); }

// CONECTADO AL TECLADO Y CORRECCIÓN DE FLECHAS ">"
void mostrarOpcionesIR(String codigoHex, uint32_t codigoCrudo, bool &dentroDeOpcion) {
    int indexOpcion = 0; String opcionesIR[3] = {"1. Replicar", "2. Guardar", "3. Regresar"}; bool enOpcionesIR = true;

    while (enOpcionesIR) {
        u8g2.clearBuffer(); u8g2.setFont(u8g2_font_6x12_tr); u8g2.setCursor(0, 10); u8g2.print("Hex: " + codigoHex);
        for (int i = 0; i < 3; i++) {
            int yPos = 30 + (i * 12);
            if (i == indexOpcion) u8g2.drawStr(0, yPos, ">"); 
            u8g2.setCursor(10, yPos); u8g2.print(opcionesIR[i]);
        }
        u8g2.sendBuffer();

        int valorY = analogRead(JOY_Y);
        if (valorY < 1000) { if (indexOpcion > 0) indexOpcion--; delay(200); }
        if (valorY > 3000) { if (indexOpcion < 2) indexOpcion++; delay(200); }

        if (digitalRead(JOY_SW) == LOW) {
            delay(300);
            if (indexOpcion == 0) enviarCodigoIR(codigoCrudo); 
            else if (indexOpcion == 1) {
                // AQUÍ INTEGRAMOS EL TECLADO SD
                String nombreUser = tecladoNombre("IR");
                guardarDato("IR", nombreUser, "Hex: " + codigoHex);
                u8g2.clearBuffer(); u8g2.drawStr(10, 30, "Guardado!"); u8g2.sendBuffer(); delay(1000);
            }
            else if (indexOpcion == 2) enOpcionesIR = false;
        }
    }
}

void modoClonarIR(bool &dentroDeOpcion) {
    String codigoHex = ""; uint32_t codigoCrudo = 0;
    while (true) {
        u8g2.clearBuffer(); u8g2.setFont(u8g2_font_6x12_tr); u8g2.drawStr(10, 20, "Esperando IR..."); u8g2.sendBuffer();
        if (recibirCodigoIR(codigoHex, codigoCrudo)) { mostrarOpcionesIR(codigoHex, codigoCrudo, dentroDeOpcion); break; }
        if (digitalRead(JOY_SW) == LOW) { delay(200); break; }
    }
}

void ataqueTVUniversal() {
    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr); u8g2.drawStr(0, 15, "TV RÁFAGA"); u8g2.sendBuffer();
    while(digitalRead(JOY_SW) == LOW) delay(10);
    bool atacando = true;
    while (atacando) {
        irsend.sendNEC(0x20DF10EF, 32); if(esperarO_Salir(50)) break; 
        irsend.sendSAMSUNG(0xE0E040BF, 32); if(esperarO_Salir(50)) break;
        irsend.sendSony(0xA90, 12, 2); if(esperarO_Salir(50)) break;
        irsend.sendPanasonic(0x4004, 0x100BCBD); if(esperarO_Salir(50)) break;
        irsend.sendNEC(0x4FB4AB5, 32); if(esperarO_Salir(50)) break;
        irsend.sendRC5(0x0C, 12); if(esperarO_Salir(50)) break;
        if(esperarO_Salir(500)) break; 
    }
}