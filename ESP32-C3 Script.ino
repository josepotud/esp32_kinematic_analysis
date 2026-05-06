#include <Wire.h>
#include <DFRobot_BMI160.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

const int i2c_sda = 8;
const int i2c_scl = 9;

const uint32_t SERIAL_BAUD = 115200;
const uint16_t SAMPLE_RATE_HZ = 200;
const uint32_t SAMPLE_INTERVAL_US = 1000000UL / SAMPLE_RATE_HZ;
const uint32_t DIAGNOSTIC_INTERVAL_MS = 5000;

DFRobot_BMI160 bmi160;
const int8_t i2c_addr = 0x69;

BLECharacteristic *pCharacteristicTX = nullptr;
bool deviceConnected = false;
bool sensorOk = false;
uint32_t sampleCounter = 0;
uint32_t lastDiagnosticMs = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    BLEDevice::startAdvertising();
  }
};

void printDiagnostics() {
  Serial.printf(
    "# diag ms=%lu rate_hz=%u samples=%lu ble=%d sensor=%d\n",
    millis(),
    SAMPLE_RATE_HZ,
    sampleCounter,
    deviceConnected ? 1 : 0,
    sensorOk ? 1 : 0
  );
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Wire.begin(i2c_sda, i2c_scl);
  Wire.setClock(400000);

  if (bmi160.softReset() != BMI160_OK || bmi160.I2cInit(i2c_addr) != BMI160_OK) {
    Serial.println("# warn BMI160 no detectado. Se enviaran datos planos.");
    sensorOk = false;
  } else {
    Serial.println("# info BMI160 detectado correctamente.");
    sensorOk = true;
  }

  BLEDevice::init("ESP32-C3-Serial");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicTX = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristicTX->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("# info Sistema listo. Streaming a %u Hz.\n", SAMPLE_RATE_HZ);
  lastDiagnosticMs = millis();
}

void loop() {
  static uint32_t lastSampleMicros = 0;
  const uint32_t nowMicros = micros();

  if ((uint32_t)(nowMicros - lastSampleMicros) >= SAMPLE_INTERVAL_US) {
    lastSampleMicros += SAMPLE_INTERVAL_US;
    sampleCounter++;

    const uint32_t t_esp = millis();
    int16_t raw_ax = 0, raw_ay = 0, raw_az = 16384;
    int16_t raw_gx = 0, raw_gy = 0, raw_gz = 0;

    if (sensorOk) {
      int16_t accelGyro[6] = {0};
      bmi160.getAccelGyroData(accelGyro);

      raw_gx = accelGyro[0];
      raw_gy = accelGyro[1];
      raw_gz = accelGyro[2];
      raw_ax = accelGyro[3];
      raw_ay = accelGyro[4];
      raw_az = accelGyro[5];
    }

    Serial.printf(
      "%lu,%d,%d,%d,%d,%d,%d\n",
      t_esp, raw_ax, raw_ay, raw_az, raw_gx, raw_gy, raw_gz
    );

    if (deviceConnected) {
      uint8_t blePayload[16];
      memcpy(&blePayload[0], &t_esp, sizeof(t_esp));
      const int16_t sensorData[6] = {raw_ax, raw_ay, raw_az, raw_gx, raw_gy, raw_gz};
      memcpy(&blePayload[4], sensorData, sizeof(sensorData));
      pCharacteristicTX->setValue(blePayload, sizeof(blePayload));
      pCharacteristicTX->notify();
    }
  }

  const uint32_t nowMs = millis();
  if ((uint32_t)(nowMs - lastDiagnosticMs) >= DIAGNOSTIC_INTERVAL_MS) {
    lastDiagnosticMs = nowMs;
    printDiagnostics();
  }
}
