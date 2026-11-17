#include <Arduino_LSM6DS3.h>
#include <ArduinoBLE.h>
#include <math.h>

// ----------------------------
// Pin assignments
// ----------------------------
const int trigPin = A1;
const int echoPin = A0;
const int distPin = A7;
const int tiltPin = A6;
const int ledPin = LED_BUILTIN;

// ----------------------------
// IMU initializaiton
// ----------------------------
float x, y, z;
const float plusThreshold = 0.5;
const float minusThreshold = -0.5;

const float freeFallThresholdG = 0.5;
const float impactThresholdG = 2.5;
const float rotationThresholdDps = 180.0;
const float fallTiltThresholdDegrees = 45.0;
const unsigned long fallSequenceWindowMs = 1200;
const unsigned long rotationWindowMs = 700;
const unsigned long fallAlertDurationMs = 5000;

unsigned long freeFallStartedAt = 0;
unsigned long highRotationAt = 0;
unsigned long fallDetectedUntil = 0;
bool freeFallInProgress = false;
bool highRotationRecently = false;
bool hasAccelerationSample = false;

float latestDistanceCm = -1.0;
bool newDistanceReading = false;
bool distanceTriggerHigh = false;
bool waitingForEcho = false;
bool echoPulseStarted = false;
unsigned long lastDistanceTriggerMs = 0;
unsigned long echoWaitStartedAtUs = 0;
unsigned long echoPulseStartedAtUs = 0;
unsigned long distanceLedLastToggle = 0;
unsigned long tiltAlertUntil = 0;
bool distanceLedOn = false;

// ----------------------------
// BLE UUIDs
// ----------------------------
const char* SERVICE_UUID =
    "12345678-1234-5678-1234-56789abcdef0";

const char* COMMAND_UUID =
    "12345678-1234-5678-1234-56789abcdef1";

const char* TELEMETRY_UUID =
    "12345678-1234-5678-1234-56789abcdef2";

const char* THRESHOLD_UUID =
    "12345678-1234-5678-1234-56789abcdef3";

// ----------------------------
// BLE service and characteristics
// ----------------------------
BLEService deviceService(SERVICE_UUID);

BLEStringCharacteristic commandCharacteristic(
    COMMAND_UUID,
    BLEWrite,
    32
);

BLEStringCharacteristic telemetryCharacteristic(
    TELEMETRY_UUID,
    BLENotify,
    160
);

BLEStringCharacteristic thresholdCharacteristic(
    THRESHOLD_UUID,
    BLEWrite,
    32  // 32 bits enough for double and triple digits?
);

// for main code
unsigned long lastTelemetryTime = 0;

// Running distance statistics for valid sensor readings.
float distanceSumCm = 0.0;
unsigned long distanceSampleCount = 0;
unsigned long distanceOverCount = 0;
float distanceThreshold = 40;

// ----------------------------
// Sensor readings
// ----------------------------
void finishDistanceMeasurement(float distance) {
  latestDistanceCm = distance;
  newDistanceReading = distance >= 0.0;
  waitingForEcho = false;
  echoPulseStarted = false;

  if (distance >= 0.0) {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("No object detected");
  }
}

void updateDistanceSensor() {
  const unsigned long nowMs = millis();
  const unsigned long nowUs = micros();

  if (distanceTriggerHigh) {
    if (nowUs - echoWaitStartedAtUs >= 10) {
      digitalWrite(trigPin, LOW);
      distanceTriggerHigh = false;
      waitingForEcho = true;
      echoWaitStartedAtUs = nowUs;
    }
    return;
  }

  if (waitingForEcho) {
    if (!echoPulseStarted && digitalRead(echoPin) == HIGH) {
      echoPulseStartedAtUs = nowUs;
      echoPulseStarted = true;
    } else if (echoPulseStarted && digitalRead(echoPin) == LOW) {
      const unsigned long durationUs = nowUs - echoPulseStartedAtUs;
      finishDistanceMeasurement(durationUs * 0.0343 / 2.0);
    } else if (nowUs - echoWaitStartedAtUs >= 25000UL) {
      finishDistanceMeasurement(-1.0);
    }
    return;
  }

  if (nowMs - lastDistanceTriggerMs >= 500) {
    lastDistanceTriggerMs = nowMs;
    digitalWrite(trigPin, HIGH);
    echoWaitStartedAtUs = nowUs;
    distanceTriggerHigh = true;
  }
}

void updateDistanceIndicator() {
  const unsigned long now = millis();
  if (latestDistanceCm <= distanceThreshold) {
    distanceLedOn = false;
    digitalWrite(distPin, LOW);
    return;
  }

  int rate = map((int)latestDistanceCm, 40, 200, 500, 10);
  rate = constrain(rate, 10, 500);
  if (now - distanceLedLastToggle >= (unsigned long)rate) {
    distanceLedLastToggle = now;
    distanceLedOn = !distanceLedOn;
    digitalWrite(distPin, distanceLedOn ? HIGH : LOW);
  }
}

const char* imuData(){
  if (!hasAccelerationSample) {
    return "No IMU data";
  }

  const char* status = "Level";
  if (y > plusThreshold) {
    status = "Too backward";
  } else if (y < minusThreshold) {
    status = "Too forward";
  } else if (x < minusThreshold) {
    status = "Too right";
  } else if (x > plusThreshold) {
    status = "Too left";
  }

  if (y > plusThreshold || y < minusThreshold ||
      x < minusThreshold || x > plusThreshold) {
    Serial.println(status);
    digitalWrite(tiltPin, HIGH);
    tiltAlertUntil = millis() + 10;
  }

  return status;
}

float calculateTiltDegrees() {
  if (!hasAccelerationSample) {
    return -1.0f;
  }

  return atan2f(
    sqrtf(x * x + y * y),
    fabsf(z)
  ) * 180.0f / PI;
}

bool fallDetect() {
  const unsigned long now = millis();

  if (IMU.gyroscopeAvailable()) {
    float gyroX, gyroY, gyroZ;
    IMU.readGyroscope(gyroX, gyroY, gyroZ);
    const float rotationMagnitude = sqrtf(
      gyroX * gyroX + gyroY * gyroY + gyroZ * gyroZ
    );
    if (rotationMagnitude >= rotationThresholdDps) {
      highRotationAt = now;
      highRotationRecently = true;
    }
  }

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    hasAccelerationSample = true;

    const float accelerationMagnitude = sqrtf(x * x + y * y + z * z);
    if (calculateTiltDegrees() > fallTiltThresholdDegrees) {
      fallDetectedUntil = now + fallAlertDurationMs;
      freeFallInProgress = false;
    } else if (accelerationMagnitude < freeFallThresholdG) {
      if (!freeFallInProgress) {
        freeFallStartedAt = now;
        freeFallInProgress = true;
      }
    } else if (
      accelerationMagnitude >= impactThresholdG &&
      ((freeFallInProgress &&
        now - freeFallStartedAt <= fallSequenceWindowMs) ||
       (highRotationRecently &&
        now - highRotationAt <= rotationWindowMs))
    ) {
      fallDetectedUntil = now + fallAlertDurationMs;
      freeFallInProgress = false;
    }
  }

  if (freeFallInProgress &&
      now - freeFallStartedAt > fallSequenceWindowMs) {
    freeFallInProgress = false;
  }
  if (highRotationRecently &&
      now - highRotationAt > rotationWindowMs) {
    highRotationRecently = false;
  }

  return (long)(fallDetectedUntil - now) > 0;
}

void updateTiltIndicator() {
  if (tiltAlertUntil != 0 && millis() >= tiltAlertUntil) {
    digitalWrite(tiltPin, LOW);
    tiltAlertUntil = 0;
  }
}

// ----------------------------
// LED handler
// ----------------------------
void handleCommand(String command) {
    command.trim();

    if (command == "LED_ON") {
        digitalWrite(ledPin, HIGH);
        Serial.println("LED turned ON");
    }
    else if (command == "LED_OFF") {
        digitalWrite(ledPin, LOW);
        Serial.println("LED turned OFF");
    }
    else {
        Serial.print("Unknown command: ");
        Serial.println(command);
    }
}

void setDistanceThreshold(String value){
  if (value.length() == 0){ return;}

  distanceThreshold = value.toFloat();
}

// ----------------------------
// Setup
// ----------------------------
void setup() {
    Serial.begin(9600);

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(distPin, OUTPUT);
    pinMode(tiltPin, OUTPUT);
    pinMode(ledPin, OUTPUT);

    digitalWrite(ledPin, LOW);

    //initialize IMU
    if (!IMU.begin()){
      Serial.println("failed to initialize IMU!");
      while(1);
    }

    if (!BLE.begin()) {
        Serial.println("BLE initialization failed!");
        while (1);
    }

    BLE.setLocalName("Nano 33 IoT");
    BLE.setAdvertisedService(deviceService);

    deviceService.addCharacteristic(
        commandCharacteristic
    );

    deviceService.addCharacteristic(
        telemetryCharacteristic
    );

    deviceService.addCharacteristic(
        thresholdCharacteristic
    );

    BLE.addService(deviceService);

    //initializes telemetry data to be unavailable at first before connecting with device
    telemetryCharacteristic.writeValue(
        "{\"distance_cm\":-1,\"average_cm\":-1,\"tilt_degrees\":-1,\"tilt_status\":\"No IMU data\",\"over_count\":0,\"fall_detected\":false}"
    );

    BLE.advertise();

    Serial.println("BLE device is advertising!");
}

// ----------------------------
// Main loop
// ----------------------------


void loop() {
  BLE.poll();
  const bool fallDetected = fallDetect();
  updateDistanceSensor();
  updateDistanceIndicator();
  updateTiltIndicator();

  // Check for commands from the mobile app
  if (commandCharacteristic.written()) {
    String command = commandCharacteristic.value();
    handleCommand(command);
  }

  // Check for threshold adjustments from the mobile app
  if (thresholdCharacteristic.written()){
    String value = thresholdCharacteristic.value();
    setDistanceThreshold(value);
  }

  // Send sensor data every 500 ms
  if (millis() - lastTelemetryTime >= 500) {
  lastTelemetryTime = millis();

  const char* tiltStatus = imuData();
  const float distance = latestDistanceCm;

  if (newDistanceReading) {
    distanceSumCm += latestDistanceCm;
    distanceSampleCount++;

    if (latestDistanceCm > distanceThreshold) {
      distanceOverCount++;
    }
    newDistanceReading = false;
  }

  double averageDistanceCm = distanceSampleCount > 0
    ? distanceSumCm / distanceSampleCount
    : -1.0;
  const float tiltDegrees = calculateTiltDegrees();

  char payload[160];

  snprintf(
    payload,
    sizeof(payload),
    "{\"distance_cm\":%.1f,\"average_cm\":%.1f,\"tilt_degrees\":%.1f,\"tilt_status\":\"%s\",\"over_count\":%lu,\"fall_detected\":%s}",
    distance,
    averageDistanceCm,
    tiltDegrees,
    tiltStatus,
    distanceOverCount,
    fallDetected ? "true" : "false"
  );

  telemetryCharacteristic.writeValue(payload);

  }
}
