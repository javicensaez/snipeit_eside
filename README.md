# snipeit_eside

Firmware para un **M5Stack Fire** (ESP32) que muestra en pantalla el stock de varios consumibles de [Snipe-IT](https://snipeitapp.com/) y permite descontarlos con los botones, identificando al usuario con una tarjeta RFID.

## Funcionalidades

- Muestra las unidades restantes de los consumibles **74LS04**, **74LS08** y **74LS32**, actualizadas cada 60 s.
- Cada botón descuenta una unidad del consumible de su fila: **A** → 74LS04, **B** → 74LS08, **C** → 74LS32.
- Las pulsaciones consecutivas se acumulan y se envían de una vez cuando dejan de llegar (1,5 s). En pantalla se ve la cantidad pendiente junto al stock, por ejemplo `74LS04: 20 -3`.
- **Modo con usuario:** al pasar una tarjeta se busca al usuario cuyo número de empleado (`employee_num`) coincide con el UID de la tarjeta. Su nombre aparece en la primera fila y los descuentos se registran como un *checkout* a ese usuario.
  - El usuario debe pertenecer a la misma compañía que el consumible; si no, la pulsación se ignora y se marca con una `X` roja.
  - A los 30 s sin actividad vuelve al modo sin usuario. Pasar otra tarjeta cambia de usuario.
- **Modo sin usuario:** los descuentos bajan directamente la cantidad total del consumible.
- **Portal cautivo de configuración:** mantener pulsado el botón A durante 2 s.

## Hardware

- M5Stack Fire
- M5Stack Unit RFID 2 (WS1850S) en el puerto A (I2C: SDA 21, SCL 22, dirección `0x28`). La unidad no tiene pin de reset; el código reserva el GPIO 2 (`RFID_RST_PIN`).

## Requisitos

- [PlatformIO](https://platformio.org/)
- Un servidor Snipe-IT con un token de API que pueda leer usuarios y consumibles, y editarlos / hacer checkout.

## Configuración inicial

1. Compila y sube el firmware:
   ```sh
   pio run -t upload
   ```
2. Mantén pulsado **A** durante 2 s. La placa crea la red abierta `SnipeIT-Setup`.
3. Conéctate desde el móvil; el portal se abre solo (si no, entra en `http://192.168.4.1`).
4. Indica la WiFi (SSID y contraseña), la URL de Snipe-IT y el token de API, y pulsa **Guardar y conectar**.

Los datos se guardan en la memoria de la placa (NVS). Al volver a abrir el portal aparecen la WiFi y la URL guardadas; la contraseña y el token no se muestran y, si se dejan vacíos, se conservan. El portal se cierra solo a los 5 minutos.

## Preparar Snipe-IT

- Los consumibles deben llamarse exactamente `74LS04`, `74LS08` y `74LS32` (se pueden cambiar en `CONSUMABLES`, en `src/main.cpp`).
- Cada usuario debe tener como **número de empleado** el UID de su tarjeta en hexadecimal y mayúsculas, sin separadores (el mismo que muestra el firmware por serie, por ejemplo `04A1B2C3`).

## Uso

| Acción | Resultado |
| --- | --- |
| Pasar tarjeta | Muestra el usuario y activa el modo con usuario |
| Pulsar A / B / C | Acumula un descuento del 74LS04 / 74LS08 / 74LS32 |
| Dejar de pulsar 1,5 s | Envía el descuento (checkout o ajuste de cantidad) |
| Mantener A 2 s | Abre el portal de configuración |

## Monitor serie

Velocidad 115200 (`monitor_speed` ya está configurado):

```sh
pio device monitor
```

Muestra la conexión WiFi, las respuestas de Snipe-IT, los UID leídos y los errores de las peticiones.

## Estructura

- `platformio.ini`: placa, framework y librerías (M5Unified, ArduinoJson, MFRC522_I2C)
- `src/main.cpp`: todo el firmware
- `AGENTS.md`: guía para agentes de IA

## Limitaciones conocidas

- El portal es una red abierta mientras está activo.
- Las credenciales se guardan sin cifrar en la placa.
- Con HTTPS no se valida el certificado del servidor.
- Si falla el envío de un descuento (sin WiFi, error de la API), no se reintenta y se pierde.
- El checkout usa `checkout_qty`; en versiones antiguas de Snipe-IT que lo ignoran, se completa con checkouts individuales.
