#ifdef CONTROL_BLE

#include "ble_control.h"
#include "pwm_control.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <cstdlib>
#include <cstring>

namespace {
// Nordic UART Service (NUS) UUIDs
constexpr char BLE_DEVICE_NAME[] = "esp32-rc-car";
constexpr char NUS_SERVICE_UUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char NUS_RX_CHAR_UUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char NUS_TX_CHAR_UUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

constexpr uint32_t COMMAND_TIMEOUT_MS = 300;

bool deviceConnected = false;
uint32_t lastCommandMs = 0;
bool failsafeActive = true;

int32_t percentToUs(int32_t pct) {
  int32_t clamped = constrain(pct, -100, 100);
  return map(clamped, -100, 100, PWM_MIN_US, PWM_MAX_US);
}

void handleCommand(const char* raw) {
  char* endPtr = nullptr;
  long steerPct = strtol(raw, &endPtr, 10);
  if (endPtr == raw || *endPtr != ',') {
    return;
  }
  const char* throtStart = endPtr + 1;
  long throtPct = strtol(throtStart, &endPtr, 10);
  if (endPtr == throtStart) {
    return;
  }

  int32_t steerUs = percentToUs(static_cast<int32_t>(steerPct));
  int32_t throtUs = percentToUs(static_cast<int32_t>(throtPct));

  pwmSetSteeringUs(steerUs);
  pwmSetThrottleUs(throtUs);
  lastCommandMs = millis();
  failsafeActive = false;

  Serial.printf("BLE cmd (%ld,%ld) -> steer=%ld us | throt=%ld us\n",
                steerPct, throtPct,
                static_cast<long>(steerUs), static_cast<long>(throtUs));
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* /*server*/) override {
    deviceConnected = true;
    lastCommandMs = millis();
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer* server) override {
    deviceConnected = false;
    failsafeActive = true;
    pwmWriteNeutral();
    Serial.println("BLE client disconnected -> neutral failsafe");
    server->getAdvertising()->start();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    std::string value = characteristic->getValue();
    if (!value.empty()) {
      handleCommand(value.c_str());
    }
  }
};
}  // namespace

void bleControlSetup() {
  BLEDevice::init(BLE_DEVICE_NAME);

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(NUS_SERVICE_UUID);

  BLECharacteristic* txChar = service->createCharacteristic(
      NUS_TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());

  BLECharacteristic* rxChar = service->createCharacteristic(
      NUS_RX_CHAR_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rxChar->setCallbacks(new RxCallbacks());

  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(NUS_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("BLE NUS advertising as 'esp32-rc-car'");
}

void bleControlLoop() {
  if (deviceConnected && !failsafeActive &&
      (millis() - lastCommandMs > COMMAND_TIMEOUT_MS)) {
    pwmWriteNeutral();
    failsafeActive = true;
    Serial.println("BLE command timeout -> neutral failsafe");
  }
  delay(10);
}

#endif  // CONTROL_BLE
