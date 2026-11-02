#include <Arduino_LSM6DS3.h>
#include <ArduinoBLE.h>

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
float plusThreshold = 0.5, minusThreshold = -0.5;   //sin(15 degrees) = 0.2588, sin(30 deg) = 0.5

// ----------------------------
// BLE UUIDs
// ----------------------------
const char* SERVICE_UUID =
    "12345678-1234-5678-1234-56789abcdef0";

const char* COMMAND_UUID =
    "12345678-1234-5678-1234-56789abcdef1";

const char* TELEMETRY_UUID =
    "12345678-1234-5678-1234-56789abcdef2";

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
    128
);

// for main code
unsigned long lastTelemetryTime = 0;

// Running distance statistics for valid sensor readings.
double distanceSumCm = 0.0;
unsigned long distanceSampleCount = 0;
unsigned long distanceOver40Count = 0;

// ----------------------------
// Sensor readings
// ----------------------------
float proxData(){
  // Send an ultrasonic pulse.
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the echo duration.
  long duration = pulseIn(echoPin, HIGH);

  if (duration == 0) {
    Serial.println("No object detected");
  } else {
    // Speed of sound: approximately 0.0343 cm/us.
    float distance = duration * 0.0343 / 2.0;

    if (distance > 40) {  //anything greater than 40cm makes led flash
      int rate = map((int)distance, 40, 200, 500, 10);  // from range of 40cm-200cm, respectively flash at a rate of 500ms-10ms intervals
      rate = constrain(rate, 10, 500);

      digitalWrite(distPin, LOW);
      delay(rate);
      digitalWrite(distPin, HIGH);
      delay(rate);
      digitalWrite(distPin, LOW);
    }

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    return distance;
  }

  return -1.0;
}

const char* imuData(){
  if (!IMU.accelerationAvailable()) {
    return "No IMU data";
  }

  IMU.readAcceleration(x, y, z);
  // Serial.print("X = ");
  // Serial.print(x);
  // Serial.print(", Y = ");
  // Serial.print(y);
  // Serial.print(", Z = ");
  // Serial.println(z);

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
    delay(10);
    digitalWrite(tiltPin, LOW);
  }

  return status;
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

    BLE.addService(deviceService);

    //initializes telemetry data to be unavailable at first before connecting with device
    telemetryCharacteristic.writeValue(
        "{\"distance_cm\":-1,\"average_cm\":-1,\"tilt_status\":\"No IMU data\",\"over40_count\":0}"
    );

    BLE.advertise();

    Serial.println("BLE device is advertising!");
}

// ----------------------------
// Main loop
// ----------------------------


void loop() {
  BLE.poll();

  // Check for commands from the mobile app
  if (commandCharacteristic.written()) {
    String command = commandCharacteristic.value();
    handleCommand(command);
  }

  // Send sensor data every 500 ms
  if (millis() - lastTelemetryTime >= 500) {
    lastTelemetryTime = millis();

    const char* tiltStatus = imuData();
    float distance = proxData();

    if (distance >= 0.0) {
      distanceSumCm += distance;
      distanceSampleCount++;

      if (distance > 40.0) {
        distanceOver40Count++;
      }

      Serial.print("Average distance: ");
      Serial.print(distanceSumCm / distanceSampleCount);
      Serial.print(" cm | Readings: ");
      Serial.print(distanceSampleCount);
      Serial.print(" | Over 40 cm: ");
      Serial.println(distanceOver40Count);
  }

  double averageDistanceCm = distanceSampleCount > 0
    ? distanceSumCm / distanceSampleCount
    : -1.0;

  char payload[128];

  snprintf(
    payload,
    sizeof(payload),
    "{\"distance_cm\":%.1f,\"average_cm\":%.1f,\"tilt_status\":\"%s\",\"over40_count\":%lu}",
    distance,
    averageDistanceCm,
    tiltStatus,
    distanceOver40Count
  );

    telemetryCharacteristic.writeValue(payload);

    Serial.println(payload);
  }
}



// OLD CODE




// float readDistance() {
//     digitalWrite(trigPin, LOW);
//     delayMicroseconds(2);

//     digitalWrite(trigPin, HIGH);
//     delayMicroseconds(10);
//     digitalWrite(trigPin, LOW);

//     unsigned long duration =
//         pulseIn(echoPin, HIGH, 30000);

//     if (duration == 0) {
//         return -1.0;
//     }

//     return duration * 0.0343 / 2.0;
// }

// void setup() {
//   Serial.begin(9600);
//   while(!Serial);

//   // initialize analog pins
//   pinMode(trigPin, OUTPUT);
//   pinMode(echoPin, INPUT);
//   pinMode(distPin, OUTPUT);
//   pinMode(tiltPin, OUTPUT);


//   //initialize IMU
//   if (!IMU.begin()){
//     Serial.println("failed to initialize IMU!");
//     while(1);
//   }

//   // initialize BLE advertising
//   // set built in LED pin to output mode
//   pinMode(ledPin, OUTPUT);

//   // begin initialization
//   if (!BLE.begin()) {
//     Serial.println("starting Bluetooth® Low Energy failed!");

//     while (1);
//   }

//   // set advertised local name and service UUID:
//   BLE.setLocalName("LED");
//   BLE.setAdvertisedService(ledService);

//   // add the characteristic to the service
//   ledService.addCharacteristic(switchCharacteristic);

//   // add service
//   BLE.addService(ledService);

//   // set the initial value for the characteristic:
//   switchCharacteristic.writeValue(0);

//   // start advertising
//   BLE.advertise();

//   Serial.println("BLE LED Peripheral");
// }

// void loop() {
  
//   // proxData();
//   // imuData();

//   // listen for BLE peripherals to connect:
//   BLEDevice central = BLE.central();

//   // if a central is connected to peripheral:
//   if (central) {
//     Serial.print("Connected to central: ");
//     // print the central's MAC address:
//     Serial.println(central.address());

//     // while the central is still connected to peripheral:
//     while (central.connected()) {
//       // if the remote device wrote to the characteristic,
//       // use the value to control the LED:
//       if (switchCharacteristic.written()) {
//         if (switchCharacteristic.value()) {   // any value other than 0
//           Serial.println("LED on");
//           digitalWrite(ledPin, HIGH);         // will turn the LED on
//         } else {                              // a 0 value
//           Serial.println(F("LED off"));
//           digitalWrite(ledPin, LOW);          // will turn the LED off
//         }
//       }
//     }

//     // when the central disconnects, print it out:
//     Serial.print(F("Disconnected from central: "));
//     Serial.println(central.address());
//   }
//   // delay(500);
// }
