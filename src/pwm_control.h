#pragma once

#include <stdint.h>

constexpr int32_t PWM_MIN_US = 1000;
constexpr int32_t PWM_NEUTRAL_US = 1500;
constexpr int32_t PWM_MAX_US = 2000;

void pwmInit();
void pwmWriteNeutral();
void pwmSetThrottleUs(int32_t us);
void pwmSetSteeringUs(int32_t us);
