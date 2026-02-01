#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>

// ======== BLE UUIDs ========
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ======== Motor Driver Direction Pins ========
// TB6612FNG #1 (Left Side)
#define LF_DIR1 15    // Left-Front Motor
#define LF_DIR2 2
#define LR_DIR1 4    // Left-Rear Motor
#define LR_DIR2 5

// TB6612FNG #2 (Right Side)
#define RF_DIR1 19    // Right-Front Motor
#define RF_DIR2 21
#define RR_DIR1 22    // Right-Rear Motor
#define RR_DIR2 23

// ======== Forward Declarations ========
void processCommand(const String& command);
int  getValue(const String& command, const String& key);
void handleMovement(int up, int down, int left, int right, int speed, int boost);
void stopAllMotors();
void controlMotor(int dir1Pin, int dir2Pin, int direction);

// ======== BLE Globals ========
BLEServer*        pServer         = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// ======== BLE Callback Classes ========
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("Flutter app connected!!");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("Flutter app disconnected. Waiting for reconnection...");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = String(pCharacteristic->getValue().c_str());
    if (value.length() > 0) {
      Serial.println("=== Command Received ===");
      Serial.println("Raw: " + value);
      processCommand(value);
      Serial.println("=======================");
    }
  }
};

// ======== Setup ========
void setup() {
  Serial.begin(115200);
  Serial.println("Starting E-ATV BLE Server...");

  // --- BLE init ---
  BLEDevice::init("E-ATV");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("E-ATV Ready");
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("E-ATV BLE server active - waiting for Flutter app...");

  // --- Configure motor direction pins ---
  pinMode(LF_DIR1, OUTPUT); pinMode(LF_DIR2, OUTPUT);
  pinMode(LR_DIR1, OUTPUT); pinMode(LR_DIR2, OUTPUT);
  pinMode(RF_DIR1, OUTPUT); pinMode(RF_DIR2, OUTPUT);
  pinMode(RR_DIR1, OUTPUT); pinMode(RR_DIR2, OUTPUT);

  // Ensure motors start stopped
  stopAllMotors();
}

// ======== Loop ========
void loop() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    BLEDevice::startAdvertising();
    Serial.println("Restarting BLE advertising...");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  delay(100);
}

// ======== Command Parsing ========
void processCommand(const String& command) {
  if (command.startsWith("MOVE:")) {
    int up    = getValue(command, "UP=");
    int down  = getValue(command, "DOWN=");
    int left  = getValue(command, "LEFT=");
    int right = getValue(command, "RIGHT=");
    int speed = getValue(command, "SPEED="); // kept for BLE compatibility
    int boost = getValue(command, "BOOST=");
    Serial.printf(
      "MOVEMENT: UP=%d DOWN=%d LEFT=%d RIGHT=%d SPEED=%d BOOST=%d\n",
      up, down, left, right, speed, boost
    );
    handleMovement(up, down, left, right, speed, boost);
  }
  else if (command.startsWith("LIGHT:")) {
    int state = getValue(command, "LIGHT:");
    Serial.println("LIGHTS: " + String(state ? "ON" : "OFF"));
    // TODO: light control
  }
  else if (command.startsWith("HORN:")) {
    Serial.println("HORN: Activated");
    // TODO: horn control
  }
  else if (command.startsWith("STOP:")) {
    Serial.println("EMERGENCY STOP: All motors stopped");
    stopAllMotors();
  }
  else {
    Serial.println("Unknown command: " + command);
  }
}

int getValue(const String& command, const String& key) {
  int start = command.indexOf(key);
  if (start < 0) return 0;
  start += key.length();
  int end = command.indexOf(',', start);
  if (end < 0) end = command.length();
  return command.substring(start, end).toInt();
}

// ======== Movement Logic (no PWM) ========
// ======== Movement Logic (simple directional checks) ========
void handleMovement(int up, int down, int left, int right, int speed, int boost) {
  // Priority: emergency stop if conflicting inputs
  if ((up && down) || (left && right)) {
    stopAllMotors();
    Serial.println("Conflict input – stopping all motors");
    return;
  }

    // DIAGONAL CASES (NEW)
  if (up && right) {
    // Forward-Right diagonal: right side slower/stopped, left side forward
    controlMotor(LF_DIR1, LF_DIR2,  0);
    controlMotor(LR_DIR1, LR_DIR2,  1);
    controlMotor(RF_DIR1, RF_DIR2, 0);
    controlMotor(RR_DIR1, RR_DIR2, -1);
    Serial.println("Turning RIGHT");
  }
  else if (up && left) {
    // Forward-Left diagonal: left side slower/stopped, right side forward
    controlMotor(LF_DIR1, LF_DIR2, 0);  // Left-Front stopped
    controlMotor(LR_DIR1, LR_DIR2, 0);  // Left-Rear stopped
    controlMotor(RF_DIR1, RF_DIR2, 1);  // Right-Front forward
    controlMotor(RR_DIR1, RR_DIR2, -1); // Right-Rear forward
    Serial.println("Moving FORWARD-LEFT");
  }
  else if (down && right) {
    // Backward-Right diagonal: right side slower/stopped, left side reverse
    controlMotor(LF_DIR1, LF_DIR2, 1);  // Left-Front reverse
    controlMotor(LR_DIR1, LR_DIR2, -1); // Left-Rear reverse
    controlMotor(RF_DIR1, RF_DIR2, 0);  // Right-Front stopped
    controlMotor(RR_DIR1, RR_DIR2, 0);  // Right-Rear stopped
    Serial.println("Moving BACKWARD-RIGHT");
  }
  else if (down && left) {
    // Backward-Left diagonal: left side slower/stopped, right side reverse
    controlMotor(LF_DIR1, LF_DIR2, 0);  // Left-Front stopped
    controlMotor(LR_DIR1, LR_DIR2, 0);  // Left-Rear stopped
    controlMotor(RF_DIR1, RF_DIR2, -1); // Right-Front reverse
    controlMotor(RR_DIR1, RR_DIR2, 1);  // Right-Rear reverse
    Serial.println("Moving BACKWARD-LEFT");
  }

  if (up) {
    // Forward: all four motors forward
    controlMotor(LF_DIR1, LF_DIR2, -1);//front motor
    controlMotor(LR_DIR1, LR_DIR2, 1);
    controlMotor(RF_DIR1, RF_DIR2, 1);
    controlMotor(RR_DIR1, RR_DIR2, -1);// front right motor
    Serial.println("Moving FORWARD");
  }
  else if (down) {
    // Backward: all four motors reverse
    controlMotor(LF_DIR1, LF_DIR2, 1);
    controlMotor(LR_DIR1, LR_DIR2, -1);
    controlMotor(RF_DIR1, RF_DIR2, -1);
    controlMotor(RR_DIR1, RR_DIR2, 1);
    Serial.println("Moving BACKWARD");
  }
  else if (left) {
    // Left turn: left side backward, right side forward
    controlMotor(LF_DIR1, LF_DIR2, -1);     // Left Front: backward
    controlMotor(LR_DIR1, LR_DIR2,  0);     // Left Rear: stopped
    controlMotor(RF_DIR1, RF_DIR2,  1);     // Right Front: forward
    controlMotor(RR_DIR1, RR_DIR2,  0);
    Serial.println("Turning LEFT");
  }
  else if (right) {
    // Right turn: left side forward, right side backward
    controlMotor(LF_DIR1, LF_DIR2,  0);
    controlMotor(LR_DIR1, LR_DIR2,  1);
    controlMotor(RF_DIR1, RF_DIR2, 0);
    controlMotor(RR_DIR1, RR_DIR2, -1);
    Serial.println("Turning RIGHT");
  }
  else {
    // No directional input
    stopAllMotors();
    Serial.println("Stopped");
  }
}

// direction: +1 = forward, -1 = reverse, 0 = stop
void controlMotor(int dir1Pin, int dir2Pin, int direction) {
  if (direction > 0) {
    digitalWrite(dir1Pin, HIGH);
    digitalWrite(dir2Pin, LOW);
  } else if (direction < 0) {
    digitalWrite(dir1Pin, LOW);
    digitalWrite(dir2Pin, HIGH);
  } else {
    digitalWrite(dir1Pin, LOW);
    digitalWrite(dir2Pin, LOW);
  }
}

void stopAllMotors() {
  Serial.println("All motors stopped!");
  controlMotor(LF_DIR1, LF_DIR2, 0);
  controlMotor(LR_DIR1, LR_DIR2, 0);
  controlMotor(RF_DIR1, RF_DIR2, 0);
  controlMotor(RR_DIR1, RR_DIR2, 0);
}
