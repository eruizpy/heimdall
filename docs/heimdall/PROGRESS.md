# Heimdall — Progreso y memoria del proyecto

Memoria compartida entre equipos (Ubuntu / MacBook). Actualizar al final de cada sesión. Lo más reciente va arriba.

## Estado actual

- **Etapa:** base importada; personalización todavía sin empezar.
- **Próximo paso:** definir la personalización: placa/hardware objetivo, nombre del asistente, servidor/backend, idioma.

## Decisiones

| Fecha | Decisión |
|---|---|
| 2026-11-05 | Heimdall parte de xiaozhi-esp32 (`main` @ `a73e9a7`), con el historial completo. |
| 2026-11-05 | Se trabaja directo en `main`, con commit y push tras cada cambio, para poder seguir desde cualquier equipo. |
| 2026-11-05 | LICENSE MIT: se conservan los avisos originales y se agrega "Esteban RD". |

## Configuración por equipo

| Equipo | ESP-IDF | Puerto serie | Notas |
|---|---|---|---|
| Ubuntu | sin instalar/verificar | — | Identidad git local: Esteban RD <eruizpy@gmail.com> |
| MacBook | sin verificar | — | — |

## Pendientes

- [ ] Elegir placa/hardware objetivo.
- [ ] Instalar/verificar ESP-IDF v6.x en cada equipo.
- [ ] Definir el nombre/identidad del asistente y el servidor.

## Bitácora

### 2026-11-05 — Ubuntu
- Se clonó heimdall y se importó xiaozhi-esp32 con su historial completo.
- Se creó `CLAUDE.md` con la regla de continuidad entre máquinas y este archivo de progreso.
