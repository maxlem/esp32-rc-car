#include <Arduino.h>

#include "pwm_control.h"

#ifdef CONTROL_BLE
#include "ble_control.h"
#else
#include <Bluepad32.h>

constexpr uint8_t PIN_BOOT_BUTTON = 0;  // Hold BOOT for 1.5s while running to clear BT keys

// Input deadzones (Bluepad32 sticks: -511..512, triggers: 0..1023)
constexpr int32_t STICK_DEADZONE = 25;
constexpr int32_t TRIGGER_DEADZONE = 30;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

int32_t applyDeadzone(int32_t value, int32_t deadzone) {
  return (abs(value) <= deadzone) ? 0 : value;
}

int32_t mapSteeringUs(int32_t axisX) {
  int32_t filtered = applyDeadzone(axisX, STICK_DEADZONE);
  return map(filtered, -511, 512, PWM_MIN_US, PWM_MAX_US);
}

int32_t mapThrottleUs(int32_t throttle, int32_t brake, int32_t axisRY) {
  int32_t netTrigger = applyDeadzone(throttle - brake, TRIGGER_DEADZONE);
  if (netTrigger != 0) {
    return map(netTrigger, -1023, 1023, PWM_MIN_US, PWM_MAX_US);
  }
  // Fallback for controllers without analog triggers: Right Stick Y (up = forward)
  int32_t filteredRY = applyDeadzone(-axisRY, STICK_DEADZONE);
  return map(filteredRY, -512, 511, PWM_MIN_US, PWM_MAX_US);
}

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      Serial.printf("Controller connected (slot %d)\n", i);
      return;
    }
  }
  Serial.println("Controller connected, but no free slot available");
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      Serial.printf("Controller disconnected (slot %d) -> neutral failsafe\n", i);
      break;
    }
  }
  pwmWriteNeutral();
}

void processControllers() {
  static uint32_t lastLogMs = 0;

  for (auto ctl : myControllers) {
    if (ctl && ctl->isConnected() && ctl->hasData()) {
      int32_t steeringUs = mapSteeringUs(ctl->axisX());
      int32_t throttleUs = mapThrottleUs(ctl->throttle(), ctl->brake(), ctl->axisRY());

      pwmSetSteeringUs(steeringUs);
      pwmSetThrottleUs(throttleUs);

      uint32_t now = millis();
      if (now - lastLogMs >= 250) {
        lastLogMs = now;
        Serial.printf("ax=(%ld,%ld) r=(%ld,%ld) trig=(%ld,%ld) btn=0x%04X -> steer=%ld us | throt=%ld us\n",
                      static_cast<long>(ctl->axisX()),
                      static_cast<long>(ctl->axisY()),
                      static_cast<long>(ctl->axisRX()),
                      static_cast<long>(ctl->axisRY()),
                      static_cast<long>(ctl->throttle()),
                      static_cast<long>(ctl->brake()),
                      ctl->buttons(),
                      static_cast<long>(steeringUs),
                      static_cast<long>(throttleUs));
      }
      return;
    }
  }
}

void checkBootButtonClearKeys() {
  static uint32_t pressedSinceMs = 0;
  static bool clearedThisPress = false;

  if (digitalRead(PIN_BOOT_BUTTON) == LOW) {
    if (pressedSinceMs == 0) {
      pressedSinceMs = millis();
    } else if (!clearedThisPress && (millis() - pressedSinceMs >= 1500)) {
      BP32.forgetBluetoothKeys();
      clearedThisPress = true;
      Serial.println("Cleared stored Bluetooth pairing keys!");
    }
  } else {
    pressedSinceMs = 0;
    clearedThisPress = false;
  }
}
#endif  // CONTROL_BLE

void setup() {
  Serial.begin(115200);
  Serial.println("esp32-rc-car (HSP SP-03018) starting...");

  pwmInit();

#ifdef CONTROL_BLE
  bleControlSetup();
#else
  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);
#endif
}

void loop() {
#ifdef CONTROL_BLE
  bleControlLoop();
#else
  checkBootButtonClearKeys();
  if (BP32.update()) {
    processControllers();
  }
  delay(10);
#endif
}
