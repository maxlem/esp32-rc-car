#include <Arduino.h>
#include <Bluepad32.h>

// GPIO pins for PWM outputs (adjust to match your wiring)
constexpr uint8_t PIN_THROTTLE_PWM = 18;  // HSP SP-03018 ESC signal
constexpr uint8_t PIN_STEERING_PWM = 19;  // Steering servo signal
constexpr uint8_t PIN_BOOT_BUTTON = 0;    // Hold BOOT for 1.5s while running to clear BT keys

// LEDC PWM configuration: standard 50 Hz RC servo/ESC frame (20,000 us)
constexpr uint8_t LEDC_CH_THROTTLE = 0;
constexpr uint8_t LEDC_CH_STEERING = 1;
constexpr uint32_t PWM_FREQ_HZ = 50;
constexpr uint8_t PWM_RES_BITS = 16;
constexpr uint32_t PWM_PERIOD_US = 20000;
constexpr uint32_t PWM_MAX_DUTY = (1UL << PWM_RES_BITS) - 1;

// RC pulse bounds in microseconds
constexpr int32_t PWM_MIN_US = 1000;
constexpr int32_t PWM_NEUTRAL_US = 1500;
constexpr int32_t PWM_MAX_US = 2000;

// Input deadzones (Bluepad32 sticks: -511..512, triggers: 0..1023)
constexpr int32_t STICK_DEADZONE = 25;
constexpr int32_t TRIGGER_DEADZONE = 30;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

uint32_t usToDuty(int32_t us) {
  int32_t clamped = constrain(us, PWM_MIN_US, PWM_MAX_US);
  return static_cast<uint32_t>((static_cast<uint64_t>(clamped) * PWM_MAX_DUTY) / PWM_PERIOD_US);
}

void writePwmUs(uint8_t channel, int32_t us) {
  ledcWrite(channel, usToDuty(us));
}

void writeNeutral() {
  writePwmUs(LEDC_CH_THROTTLE, PWM_NEUTRAL_US);
  writePwmUs(LEDC_CH_STEERING, PWM_NEUTRAL_US);
}

int32_t applyDeadzone(int32_t value, int32_t deadzone) {
  return (abs(value) <= deadzone) ? 0 : value;
}

int32_t mapSteeringUs(int32_t axisX) {
  int32_t filtered = applyDeadzone(axisX, STICK_DEADZONE);
  return map(filtered, -511, 512, PWM_MIN_US, PWM_MAX_US);
}

int32_t mapThrottleUs(int32_t throttle, int32_t brake) {
  int32_t net = applyDeadzone(throttle - brake, TRIGGER_DEADZONE);
  return map(net, -1023, 1023, PWM_MIN_US, PWM_MAX_US);
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
  writeNeutral();
}

void processControllers() {
  static uint32_t lastLogMs = 0;

  for (auto ctl : myControllers) {
    if (ctl && ctl->isConnected() && ctl->hasData()) {
      int32_t steeringUs = mapSteeringUs(ctl->axisX());
      int32_t throttleUs = mapThrottleUs(ctl->throttle(), ctl->brake());

      writePwmUs(LEDC_CH_STEERING, steeringUs);
      writePwmUs(LEDC_CH_THROTTLE, throttleUs);

      uint32_t now = millis();
      if (now - lastLogMs >= 250) {
        lastLogMs = now;
        Serial.printf("steering=%ld us | throttle=%ld us\n",
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

void setup() {
  Serial.begin(115200);
  Serial.println("esp32-rc-car (Bluepad32 + HSP SP-03018) starting...");

  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);

  // Configure native LEDC PWM channels (avoids timer conflicts with Bluetooth)
  ledcSetup(LEDC_CH_THROTTLE, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PIN_THROTTLE_PWM, LEDC_CH_THROTTLE);

  ledcSetup(LEDC_CH_STEERING, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PIN_STEERING_PWM, LEDC_CH_STEERING);

  // Immediately output 1500 us neutral so HSP SP-03018 calibrates center on boot
  writeNeutral();

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);
}

void loop() {
  checkBootButtonClearKeys();
  if (BP32.update()) {
    processControllers();
  }
  delay(10);
}
