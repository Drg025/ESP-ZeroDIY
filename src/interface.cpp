#include <Arduino.h>
#include <interface.h>

// Definición de tu pantalla OLED
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/26 , /* data=*/ 25);

void inicializarPantalla() {
  u8g2.begin();
}

void dibujarPantalla(int menuIndex, bool enSubMenu, String items[], int numItems){
  u8g2.clearBuffer();

  // --- 1. DIBUJAR LA CABECERA ---
  u8g2.setFont(u8g2_font_ncenB08_tr);
  if(!enSubMenu){
    u8g2.drawStr(15, 10, "-- FLIPPER HUB --");
  } else {
    u8g2.drawStr(25, 10, "-- SUBMENU --");
  }
  // Línea separadora horizontal
  u8g2.drawLine(0, 13, 128, 13);

  // --- 2. LÓGICA DE SCROLLING DINÁMICO ---
  int maxOpcionesVisibles = 4; // Máximo de renglones que caben perfectamente en 64px
  int indiceInicio = 0;

  // Si el cursor baja más allá de lo visible, deslizamos la "ventana" hacia abajo
  if (menuIndex >= maxOpcionesVisibles) {
    indiceInicio = menuIndex - maxOpcionesVisibles + 1;
  }

  // --- 3. DIBUJAR LAS OPCIONES VISIBLES ---
  u8g2.setFont(u8g2_font_6x12_tr); // Fuente compacta para los textos

  for (int i = 0; i < maxOpcionesVisibles; i++) {
    int itemReal = indiceInicio + i;
    
    // Si ya dibujamos todos los items existentes, salimos del bucle de dibujo
    if (itemReal >= numItems) break; 

    // Calculamos la posición Y (i=0 -> 25, i=1 -> 37, i=2 -> 49, i=3 -> 61)
    int yPos = 25 + (i * 12); 

    // Dibujar el cursor ">" si es la opción seleccionada
    if (itemReal == menuIndex) {
      u8g2.drawStr(0, yPos, ">"); 
    }
    
    // Dibujar el texto de la opción
    u8g2.setCursor(12, yPos);
    u8g2.print(items[itemReal]);
  }

  // --- 4. INDICADORES VISUALES DE DESPLAZAMIENTO ---
  // Si estamos más abajo del inicio, mostramos una flecha hacia arriba
  if (indiceInicio > 0) {
    u8g2.drawStr(120, 22, "^"); 
  }
  // Si todavía hay elementos por debajo de nuestra ventana, mostramos flecha abajo
  if (indiceInicio + maxOpcionesVisibles < numItems) {
    u8g2.drawStr(120, 62, "v"); 
  }

  u8g2.sendBuffer();
}