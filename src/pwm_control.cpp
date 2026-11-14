#include "pwm_control.h"

#include <Arduino.h>

// GPIO pins for PWM outputs (adjust to match your wiring)
constexpr uint8_t PIN_THROTTLE_PWM = 18;  // HSP SP-03018 ESC signal
constexpr uint8_t PIN_STEERING_PWM = 19;  // Steering servo signal

// LEDC PWM configuration: standard 50 Hz RC servo/ESC frame (20,000 us)
constexpr uint8_t LEDC_CH_THROTTLE = 0;
constexpr uint8_t LEDC_CH_STEERING = 1;
constexpr uint32_t PWM_FREQ_HZ = 50;
constexpr uint8_t PWM_RES_BITS = 16;
constexpr uint32_t PWM_PERIOD_US = 20000;
constexpr uint32_t PWM_MAX_DUTY = (1UL << PWM_RES_BITS) - 1;

namespace {
uint32_t usToDuty(int32_t us) {
  int32_t clamped = constrain(us, PWM_MIN_US, PWM_MAX_US);
  return static_cast<uint32_t>((static_cast<uint64_t>(clamped) * PWM_MAX_DUTY) / PWM_PERIOD_US);
}
}  // namespace

void pwmInit() {
  ledcSetup(LEDC_CH_THROTTLE, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PIN_THROTTLE_PWM, LEDC_CH_THROTTLE);

  ledcSetup(LEDC_CH_STEERING, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(PIN_STEERING_PWM, LEDC_CH_STEERING);

  // Immediately output 1500 us neutral so HSP SP-03018 calibrates center on boot
  pwmWriteNeutral();
}

void pwmWriteNeutral() {
  pwmSetThrottleUs(PWM_NEUTRAL_US);
  pwmSetSteeringUs(PWM_NEUTRAL_US);
}

void pwmSetThrottleUs(int32_t us) {
  ledcWrite(LEDC_CH_THROTTLE, usToDuty(us));
}

void pwmSetSteeringUs(int32_t us) {
  ledcWrite(LEDC_CH_STEERING, usToDuty(us));
}
