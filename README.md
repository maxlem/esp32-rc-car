# esp32-rc-car

Remote-controlled Tamiya hobby car driven by an **ESP-WROOM-32** (ESP32) using 50 Hz LEDC PWM for an **HSP SP-03018** brushed ESC and steering servo, controlled via Bluetooth gamepad (**Bluepad32**).

## Hardware & Wiring

- **MCU**: ESP-WROOM-32
- **ESC**: HSP SP-03018 (30A brushed ESC, 50 Hz RC PWM, `1000–2000 µs`, `1500 µs` neutral)
- **Steering**: Standard hobby RC servo (50 Hz RC PWM)

| Signal | ESP32 Pin | Notes |
| --- | --- | --- |
| Throttle PWM | `GPIO 18` | HSP SP-03018 signal wire |
| Steering PWM | `GPIO 19` | Steering servo signal wire |
| 5V Power | `5V` / `VIN` | Powered from HSP SP-03018 built-in 5V BEC |
| Ground | `GND` | Common ground shared between ESC, servo, and ESP32 |

> **Note on HSP SP-03018 Arming:** The ESC performs automatic center-point calibration on startup. The firmware outputs `1500 µs` neutral immediately in `setup()` and on controller disconnect (failsafe).

## Gamepad Controls (Bluepad32)

- **Left Stick X**: Steering (`1000–2000 µs`)
- **Right Trigger (R2)**: Forward throttle (`1500–2000 µs`)
- **Left Trigger (L2)**: Brake / Reverse (`1500–1000 µs`)

## Getting Started

This project uses [PlatformIO](https://platformio.org/) with the Bluepad32-enabled Arduino-ESP32 core.

```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```
