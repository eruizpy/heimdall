# Heimdall robot — arquitectura y compras

## Arquitectura

```
                 WiFi (wss://<dominio>)
                          │
┌─────────────────────────┴──────────────────────────┐
│  CORE: ESP32-S3 N16R8                               │
│  • xiaozhi/Heimdall: voz, LLM, herramientas MCP     │
│  • INMP441 (mic) + MAX98357A (parlante)  ── I2S     │
│  • Cabeza: 2 servos por LEDC de la S3               │
│  • Patas: 18 servos por 2× PCA9685       ── I2C     │
│  • Marcha: cinemática inversa, paso trípode         │
└──────────────┬──────────────────────────────────────┘
               │ UART (CN1 de la CYD: 3.3V, GND, GPIO22, GPIO27) + 5V por P1 (VIN)
┌──────────────┴──────────────────────────────────────┐
│  CARA: ESP32-2432S028 (CYD), montada en el pan-tilt │
│  • Pantalla remota (caras, texto, estado)           │
│  • Touch con coordenadas → eventos al core          │
└─────────────────────────────────────────────────────┘
```

Etapas de software: (1) CYD como terminal UART + touch con coordenadas (se puede hacer sin comprar nada),
(2) core S3 con voz y pantalla remota, (3) cabeza pan-tilt, (4) hexápodo.

Precios aproximados de noviembre de 2026, sin envío ni impuestos. Los enlaces son búsquedas en amazon.com.

## Compra A (noviembre 2026): voz y cabeza móvil — USD 105–141

| Producto | Cant. | Aprox. | Fijate que diga |
|---|---|---|---|
| [ESP32-S3 DevKitC-1 N16R8](https://www.amazon.com/s?k=ESP32-S3-DevKitC-1+N16R8) | 1 | USD 15–20 | N16R8, USB-C |
| [Micrófono INMP441 I2S](https://www.amazon.com/s?k=INMP441+I2S+microphone+module) | 1 pack | USD 8–12 | INMP441 |
| [Amplificador MAX98357A I2S](https://www.amazon.com/s?k=MAX98357A+I2S+amplifier) | 1 pack | USD 8–12 | MAX98357A, 3 W |
| [Parlante 4 Ω 3 W](https://www.amazon.com/s?k=4+ohm+3W+speaker+JST+PH2.0) | 1–2 | USD 8–10 | 4 Ω, 3 W |
| [Servos MG90S](https://www.amazon.com/s?k=MG90S+metal+gear+servo+10+pack) | 1 pack de 10 | USD 25–30 | MG90S engranaje metálico |
| [Soporte pan-tilt](https://www.amazon.com/s?k=pan+tilt+bracket+SG90+MG90S) | 1 | USD 8–12 | Para micro servos |
| [Fuente 5 V 3 A 5,5×2,1](https://www.amazon.com/s?k=5V+3A+power+supply+5.5x2.1) + [adaptador a bornera](https://www.amazon.com/s?k=DC+barrel+jack+female+screw+terminal+adapter) | 1 + 1 | USD 10–15 | 5 V, 3 A |
| [Capacitores 1000 µF](https://www.amazon.com/s?k=1000uF+16V+electrolytic+capacitor) | 1 surtido | USD 6–8 | ≥ 16 V |
| [Cable JST 1,25 mm 4 pines](https://www.amazon.com/s?k=JST+1.25mm+4+pin+cable) | 1 pack | USD 7–10 | 1,25 mm (CN1 de la CYD) |
| [Protoboard + dupont](https://www.amazon.com/s?k=breadboard+jumper+wires+kit) | 1 | USD 10–12 | M-M, M-H, H-H |
| Si no hay: [soldador + multímetro](https://www.amazon.com/s?k=soldering+iron+kit+multimeter) | 1 | USD 25–45 | |
| Opcional: [gestos PAJ7620U2](https://www.amazon.com/s?k=PAJ7620U2+gesture+sensor) | 1 | USD 8–12 | |

## Compra B (diciembre 2026): hexápodo, solo lo faltante — USD 164–234

| Producto | Cant. | Aprox. |
|---|---|---|
| [Servos MG90S](https://www.amazon.com/s?k=MG90S+metal+gear+servo+10+pack) | 2 packs de 10 (30 en total) | USD 50–60 |
| [PCA9685](https://www.amazon.com/s?k=PCA9685+16+channel+servo+driver) | 2 | USD 10–14 |
| [LiPo 2S 7,4 V 2200 mAh XT60](https://www.amazon.com/s?k=2S+7.4V+2200mAh+lipo+XT60) | 1 | USD 25–35 |
| [Cargador balanceador LiPo](https://www.amazon.com/s?k=lipo+balance+charger+2S) | 1 | USD 25–50 |
| [UBEC 5/6 V 8–10 A](https://www.amazon.com/s?k=UBEC+10A+5V+6V) | 1 | USD 12–20 |
| [Buck 5 V 3 A](https://www.amazon.com/s?k=buck+converter+5V+3A+USB) | 1 | USD 8–10 |
| [Alarma de voltaje LiPo](https://www.amazon.com/s?k=lipo+low+voltage+alarm) | 1 | USD 6–8 |
| [XT60 + interruptor](https://www.amazon.com/s?k=XT60+connector+switch+pigtail) | 1 kit | USD 8–12 |
| [Alargues de servo](https://www.amazon.com/s?k=servo+extension+cable+20+pack) | 1 pack | USD 8–10 |
| [Tornillos M2/M3](https://www.amazon.com/s?k=M2+M3+screw+nut+assortment+kit) | 1 | USD 12–15 |
| Opcionales: [VL53L0X](https://www.amazon.com/s?k=VL53L0X+time+of+flight), [MPU6050](https://www.amazon.com/s?k=MPU6050+module) | 1 c/u | USD 16–20 |

Total del proyecto: USD 269–375, más piezas 3D (filamento USD 20–25 o impresión por encargo USD 40–100) y envío/impuestos.

Notas:
- Las LiPo suelen no enviarse fuera de EE. UU.: comprar batería y cargador localmente.
- Servos siempre con alimentación propia (fuente 5 V ahora; UBEC con LiPo después), tierra común con la S3.
- No comprar OLED: la cara es la CYD.
