# esp32-rc-car

Remote-controlled Tamiya hobby car driven by an **ESP-WROOM-32** (ESP32) using 50 Hz LEDC PWM for an **HSP SP-03018** brushed ESC and steering servo.

Supports two build targets:
- `esp32dev` (default): **Bluetooth Gamepad** via **Bluepad32**
- `esp32dev-ble`: **BLE (Nordic UART Service)** for Web Bluetooth / mobile apps

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

> **Note on HSP SP-03018 Arming:** The ESC performs automatic center-point calibration on startup. Both targets output `1500 µs` neutral immediately in `setup()` and on disconnect/timeout failsafe.

## Mode 1: Bluetooth Gamepad (`esp32dev`)

- **Left Stick X**: Steering (`1000–2000 µs`)
- **Right Trigger (R2) / Left Trigger (L2)**: Forward / Brake & Reverse (`1000–2000 µs`)
- **Right Stick Y**: Fallback throttle/reverse when triggers are idle
- Hold **BOOT (`GPIO 0`)** for 1.5s while running to clear stored Bluetooth pairing keys.

```bash
pio run --target upload
```

## Mode 2: BLE Control (`esp32dev-ble`)

Advertises as **`esp32-rc-car`** with the **Nordic UART Service (NUS)**:

- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic (Write / Write No Response)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX Characteristic (Notify)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
- **Packet Format**: `<steer>,<throttle>` as signed integers from `-100` to `100` (e.g., `0,0` neutral, `-50,40` steer left + 40% throttle).
- **Failsafe**: Resets steering and throttle to `1500 µs` on disconnect or if no packet arrives within `300 ms`.

```bash
pio run -e esp32dev-ble --target upload
```

## Serial Monitor

```bash
pio device monitor
```
