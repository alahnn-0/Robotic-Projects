#include <Servo.h>

// Motor Driver Pins
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5

// Ultrasonic Sensor Pins
#define ECHO_PIN 9
#define TRIG_PIN 10

// Servo Pin
#define SERVO_PIN 11

Servo scanner;

// ---------- Motor Functions ----------

void moveForward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void moveBackward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void turnLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// ---------- Ultrasonic Function ----------

float getDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0)
    return 400;

  return duration * 0.0343 / 2;
}

// ---------- Setup ----------

void setup() {

  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  scanner.attach(SERVO_PIN);
  scanner.write(90); // Center

  delay(1000);
}

// ---------- Main Loop ----------

void loop() {

  // Look forward
  scanner.write(90);
  delay(200);

  float frontDistance = getDistance();

  Serial.print("Front: ");
  Serial.println(frontDistance);

  // No obstacle
  if (frontDistance > 25) {

    moveForward();
  }

  // Obstacle detected
  else {

    Serial.println("Obstacle Detected");

    stopMotors();
    delay(200);

    // Move backward a little
    moveBackward();
    delay(500);

    stopMotors();
    delay(200);

    // Check RIGHT first
    scanner.write(180);
    delay(600);

    float rightDistance = getDistance();

    Serial.print("Right: ");
    Serial.println(rightDistance);

    if (rightDistance > 25) {

      Serial.println("Turning Right");

      turnRight();
      delay(700);

      stopMotors();

      scanner.write(90);
      delay(200);

      return;
    }

    // Check LEFT
    scanner.write(0);
    delay(600);

    float leftDistance = getDistance();

    Serial.print("Left: ");
    Serial.println(leftDistance);

    if (leftDistance > 25) {

      Serial.println("Turning Left");

      turnLeft();
      delay(700);

      stopMotors();

      scanner.write(90);
      delay(200);

      return;
    }

    // Both sides blocked
    Serial.println("No Path Available");

    stopMotors();

    scanner.write(90);
  }

  delay(100);
}
