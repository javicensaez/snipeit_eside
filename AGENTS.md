# AGENTS.md

## Proyecto
Este repositorio es un proyecto de PlatformIO para un dispositivo M5Stack Fire basado en ESP32.

- Plataforma: `espressif32`
- Board: `m5stack-fire`
- Framework: `arduino`
- Lenguaje principal: C++ para Arduino/ESP32
- Punto de entrada: `src/main.cpp`

## Objetivo del proyecto
El proyecto está pensado para desarrollar firmware para hardware M5Stack Fire, con lógica principal en `src/main.cpp` y extensión posible mediante librerías en `lib/` o código auxiliar en `include/`.

## Reglas de trabajo
- Mantener compatibilidad con PlatformIO y con la placa `m5stack-fire`.
- Preferir código limpio, modular y fácil de depurar para Arduino/ESP32.
- Si se añaden dependencias, declararlas en `platformio.ini` o usar librerías estándar del framework.
- Evitar cambios innecesarios en la configuración del proyecto.
- Si se hace lógica de negocio o hardware, documentarla con comentarios cortos y precisos.
- No introducir dependencias externas sin necesidad.

## Comandos útiles
Usar PlatformIO para compilar, subir y monitorizar:

- Compilar: `pio run`
- Subir firmware: `pio run -t upload`
- Monitor serie: `pio device monitor`
- Build limpio: `pio run -t clean`

## Estructura relevante
- `platformio.ini`: configuración de entorno y placa
- `src/main.cpp`: firmware principal
- `include/`: cabeceras auxiliares
- `lib/`: bibliotecas del proyecto
- `test/`: pruebas o utilidades de verificación

## Estilo de código
- C++ orientado a Arduino, con `setup()` y `loop()`.
- Usar nombres descriptivos y evitar variables globales innecesarias.
- Priorizar claridad y mantenibilidad sobre micro-optimizaciones.
- Probar compilación después de cambios importantes.

## Recomendaciones para futuras tareas
- Antes de añadir funcionalidad compleja, revisa si encaja mejor en una librería separada en `lib/`.
- Mantén el código del firmware central claro y mínimo.
- Si se necesita integración con sensores, Wi‑Fi, MQTT, almacenamiento o pantallas M5Stack, usa APIs del framework y librerías compatibles con la placa.
