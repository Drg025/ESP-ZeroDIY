# ESP32 Red Team Multi-Tool (DIY Auditor)

Una herramienta portátil de auditoría de seguridad y *Red Teaming* construida desde cero utilizando un microcontrolador ESP32. Este dispositivo cuenta con una arquitectura modular diseñada para la recolección de datos en campo, clonación de tarjetas RFID/NFC y exfiltración inalámbrica de *payloads* y volcados de memoria.



## Descripción del Proyecto

Este proyecto fue desarrollado como parte de una práctica académica en el área de Ciberseguridad. El objetivo principal es la aplicación técnica de conocimientos en hardware hacking, programación en C++ y protocolos de comunicación inalámbrica en un escenario controlado de Red Teaming.
La arquitectura y funcionalidades de esta herramienta están fuertemente inspiradas en el ecosistema de **Flipper Zero**, buscando replicar su versatilidad y potencia en un entorno DIY (Do It Yourself) utilizando hardware accesible. A diferencia de las soluciones comerciales cerradas, este sistema permite un control total sobre el firmware, ofreciendo navegación autónoma mediante una interfaz física y gestión inalámbrica de los datos capturados.

## Créditos y Referencias

* **Inspiración Original:** Este proyecto rinde tributo a las capacidades del [Flipper Zero](https://flipperzero.one/), el cual sirvió como referencia principal para el diseño de la interfaz de usuario y la integración de módulos de ataque inalámbrico.
* **Desarrollo:** Firmware y ensamblaje desarrollado de forma independiente para fines educativos.

## Características Principales

*   **Interfaz Autónoma:** Sistema de menús dinámicos controlados por un joystick analógico y visualizados en una pantalla OLED.
*   **Almacenamiento Persistente:** Integración de un módulo MicroSD para guardar diccionarios, *payloads* y volcados de memoria completos.
*   **Exfiltración Inalámbrica (FTP):** El ESP32 levanta un punto de acceso WiFi (`FLIPPER_DIY`) con un servidor FTP embebido, permitiendo extraer los datos capturados directamente a un smartphone o PC mediante consola (`cmd`) o FileZilla.
*   **Vector de Ataque NFC/RFID:**
    *   Lectura de UIDs (ISO14443A / Mifare).
    *   Volcado (*Dump*) automatizado de sectores de memoria usando claves por defecto.
    *   Clonación directa del Bloque 0 hacia tarjetas "Mágicas" (Gen 1a).

## Hardware Necesario

*   **Microcontrolador:** Placa de desarrollo ESP32 (Módulo WROOM).
*   **Pantalla:** Módulo OLED 1.3" (Chip SH1106) vía I2C.
*   **Control:** Joystick analógico estándar.
*   **Almacenamiento:** Módulo lector de tarjetas MicroSD (vía SPI).
*   **RFID/NFC:** Módulo Adafruit PN532 (vía I2C).

## Estructura del Software y Librerías

El firmware está desarrollado en **C++** utilizando **PlatformIO**. Se diseñó de forma modular separando la lógica de la interfaz (`interface.cpp`), almacenamiento (`sd_module.cpp`) y módulos de ataque (`nfc_module.cpp`).

Dependencias principales en `platformio.ini`:
*   `olikraus/U8g2` (Gestión de la pantalla OLED).
*   `adafruit/Adafruit PN532` (Lógica de lectura y escritura NFC).
*   `xreef/SimpleFTPServer` (Servidor FTP modificado para apuntar a la tarjeta SD).

## Instalación y Uso

1.  Clona este repositorio:
    ```bash
    git clone [https://github.com/Drg025/ESP32-Flipper-hub](https://github.com/Drg025/ESP32-Flipper-hub)
    ```
2.  Abre el proyecto en **Visual Studio Code** con la extensión de **PlatformIO**.
3.  Conecta tu ESP32 y compila/sube el código usando PlatformIO.
4.  **Para extraer datos vía FTP:**
    *   Conéctate a la red WiFi del dispositivo (`FLIPPER_DIY`).
    *   Abre una terminal en tu PC y ejecuta: `ftp 192.168.4.1`
    *   Credenciales por defecto: User: `admin` / Pass: `admin`.

## Aviso Legal / Disclaimer

Este proyecto fue creado **únicamente con fines educativos y de investigación en ciberseguridad**. El autor no se hace responsable del mal uso que se le pueda dar a esta herramienta o al código fuente proporcionado. Utiliza este dispositivo únicamente en redes, sistemas y tarjetas físicas sobre las cuales tengas autorización explícita para auditar.