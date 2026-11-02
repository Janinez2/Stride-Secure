// #include <Arduino_LSM6DS3.h>
// #include <ArduinoBLE.h>


// const int trigPin = A1; //used to be A1
// const int echoPin = A0; //used to be A0
// const int distPin = A7; //used to be A8, 
// const int tiltPin = A6; //imu feedback

// float x, y, z;
// float plusThreshold = 0.5, minusThreshold = -0.5;   //sin(15 degrees) = 0.2588, sin(30 deg) = 0.5

// // long previousMillis = 0;
// // int interval = 0;
// // int ledState = LOW;

// // BLEService ledService("180A"); // BLE LED Service

// // // BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
// // BLEByteCharacteristic switchCharacteristic("2A57", BLERead | BLEWrite);

// BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// // Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
// BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

// const int ledPin = LED_BUILTIN; // pin to use for the LED

// void proxData(){
//   // Send an ultrasonic pulse.
//   digitalWrite(trigPin, LOW);
//   delayMicroseconds(2);

//   digitalWrite(trigPin, HIGH);
//   delayMicroseconds(10);
//   digitalWrite(trigPin, LOW);

//   // Measure the echo duration.
//   long duration = pulseIn(echoPin, HIGH);

//   if (duration == 0) {
//     Serial.println("No object detected");
//   } else {
//     // Speed of sound: approximately 0.0343 cm/us.
//     float distance = duration * 0.0343 / 2.0;

//     if (distance > 40) {  //anything greater than 40cm makes led flash
//       int rate = map((int)distance, 40, 200, 500, 10);  // from range of 40cm-200cm, respectively flash at a rate of 500ms-10ms intervals to 
//       rate = constrain(rate, 10, 500);

//       digitalWrite(distPin, HIGH);
//       delay(rate);
//       digitalWrite(distPin, LOW);
//       // delay(rate);
//     }
//     // else{
//     //   Serial.print("in range");
//     // }

//     Serial.print("Distance: ");
//     Serial.print(distance);
//     Serial.println(" cm");
//   }
// }

// void imuData(){
//   //IMU data
//   if (IMU.accelerationAvailable()) {
//       IMU.readAcceleration(x, y, z);
//       // Serial.print("X = ");
//       // Serial.print(x);
//       // Serial.print(", Y = ");
//       // Serial.print(y);
//       // Serial.print(", Z = ");
//       // Serial.println(z);
//   }
//   if(y > plusThreshold)
//   {
//     Serial.println("Collision front");
//     digitalWrite(tiltPin, HIGH);
//     delay(10);
//     digitalWrite(tiltPin, LOW);
//     // delay(10);

//     // delay(500);
//   }
//   if(y < minusThreshold)
//   {
//     Serial.println("Collision back");
//     digitalWrite(tiltPin, HIGH);
//     delay(10);
//     digitalWrite(tiltPin, LOW);
//     // delay(10);
//   }
//   if(x < minusThreshold)
//   {
//     Serial.println("Collision right");
//     digitalWrite(tiltPin, HIGH);
//     delay(10);
//     digitalWrite(tiltPin, LOW);
//     // delay(10);
//   }
//   if(x > plusThreshold)
//   {
//     Serial.println("Collision left");
//     digitalWrite(tiltPin, HIGH);
//     delay(10);
//     digitalWrite(tiltPin, LOW);
//     // delay(10);
//   }
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
    100
);

// ----------------------------
// Sensor reading
// ----------------------------
float readDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long duration =
        pulseIn(echoPin, HIGH, 30000);

    if (duration == 0) {
        return -1.0;
    }

    return duration * 0.0343 / 2.0;
}

// ----------------------------
// Command handler
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
    pinMode(distPin, INPUT);
    pinMode(tiltPin, INPUT);
    pinMode(ledPin, OUTPUT);

    digitalWrite(ledPin, LOW);

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

    telemetryCharacteristic.writeValue(
        "{\"distance_cm\":-1,\"dist_raw\":0,\"tilt_raw\":0}"
    );

    BLE.advertise();

    Serial.println("BLE device is advertising!");
}

// ----------------------------
// Main loop
// ----------------------------
unsigned long lastTelemetryTime = 0;

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

        float distance = readDistance();
        int distRaw = analogRead(distPin);
        int tiltRaw = analogRead(tiltPin);

        char payload[100];

        snprintf(
            payload,
            sizeof(payload),
            "{\"distance_cm\":%.1f,\"dist_raw\":%d,\"tilt_raw\":%d}",
            distance,
            distRaw,
            tiltRaw
        );

        telemetryCharacteristic.writeValue(payload);

        Serial.println(payload);
    }
}
