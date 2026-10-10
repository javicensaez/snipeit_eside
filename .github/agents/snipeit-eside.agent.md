---
name: snipeit-eside
description: "Use when working on the M5Stack Fire ESP32 firmware in this PlatformIO project. Focus on Arduino/ESP32 compatibility, build validation with PlatformIO, and safe hardware-specific changes."
---

# snipeit-eside agent

## Proyecto
Este repositorio contiene firmware para un M5Stack Fire con ESP32 usando PlatformIO.

- Board: `m5stack-fire`
- Framework: `arduino`
- Lenguaje principal: C++ para ESP32
- Entrada principal: `src/main.cpp`

## Reglas
- Mantener la compatibilidad con `m5stack-fire` y `espressif32`.
- Usar `pio run` para validar cambios antes de dar por finalizada la tarea.
- Preferir soluciones limpias y reutilizables para firmware Arduino.
- Evitar cambios de configuración innecesarios en `platformio.ini`.
- No añadir librerías externas sin necesidad.
- Si se trabaja con sensores, pantallas, Wi‑Fi o otros periféricos, confirmar que la solución es compatible con M5Stack Fire.

## Estructura
- `platformio.ini`: configuración del proyecto
- `src/main.cpp`: firmware principal
- `include/`: cabeceras auxiliares
- `lib/`: bibliotecas locales
- `test/`: comprobaciones o pruebas

## Comandos
- Compilar: `pio run`
- Subir: `pio run -t upload`
- Monitor serie: `pio device monitor`

## Estilo
- Código C++ claro y mantenible
- Nombres descriptivos
- Preferir modularización sobre lógica monolítica
- Comentarios breves y útiles solo cuando aportan valor
