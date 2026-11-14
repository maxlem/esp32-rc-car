#include <Arduino.h>

// Default GPIO pins for PWM outputs (adjust to match your wiring)
constexpr uint8_t PIN_THROTTLE_PWM = 18;
constexpr uint8_t PIN_STEERING_PWM = 19;

// Standard 50 Hz RC PWM (20 ms period)
constexpr uint32_t PWM_FREQ_HZ = 50;
constexpr uint8_t PWM_RES_BITS = 16;

void setup() {
  Serial.begin(115200);
  Serial.println("esp32-rc-car starting...");

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_THROTTLE_PWM, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttach(PIN_STEERING_PWM, PWM_FREQ_HZ, PWM_RES_BITS);
#else
  ledcSetup(0, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PIN_THROTTLE_PWM, 0);
  ledcSetup(1, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PIN_STEERING_PWM, 1);
#endif
}

void loop() {
  // TODO: Add Wi-Fi / Bluetooth control loop and update PWM duty cycles
  delay(50);
}
