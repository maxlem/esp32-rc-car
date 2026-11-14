# esp32-rc-car

Remote-controlled hobby car driven by an **ESP-WROOM-32** (ESP32) using PWM for throttle and steering, with Wi-Fi / Bluetooth connectivity.

## Hardware

- **MCU**: ESP-WROOM-32 (Wi-Fi + Bluetooth)
- **Control**: PWM signals for ESC / motor driver (throttle) and servo (steering)

## Getting Started

This project uses [PlatformIO](https://platformio.org/) with the Arduino framework.

### Build & Upload

```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```
