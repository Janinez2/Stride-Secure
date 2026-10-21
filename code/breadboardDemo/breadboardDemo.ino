
const int trigPin = A1;
const int echoPin = A0;
const int ledPin = A8;

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
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

    if (distance > 30) {
      int rate = map((int)distance, 30, 200, 500, 50);
      rate = constrain(rate, 10, 500);

      digitalWrite(ledPin, HIGH);
      delay(rate);
      digitalWrite(ledPin, LOW);
      delay(rate);
    }
    // else{
    //   Serial.print("in range");
    // }

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  delay(500);
}
