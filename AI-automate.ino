#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

String command;

bool thinkingMode = false;

int pupilX = 0;
int direction = 1;

// =====================
// BASIC FACES
// =====================

void sleepyFace() {
  display.clearDisplay();
  display.fillRoundRect(25, 25, 25, 8, 4, WHITE);
  display.fillRoundRect(78, 25, 25, 8, 4, WHITE);
  display.display();
}

void happyFace() {
  display.clearDisplay();
  display.drawLine(25, 32, 37, 24, WHITE);
  display.drawLine(37, 24, 50, 32, WHITE);
  display.drawLine(78, 32, 90, 24, WHITE);
  display.drawLine(90, 24, 103, 32, WHITE);
  display.display();
}

void sadFace() {
  display.clearDisplay();
  display.drawLine(25, 24, 50, 32, WHITE);
  display.drawLine(78, 24, 103, 32, WHITE);
  display.display();
}

void angryFace() {
  display.clearDisplay();
  display.drawLine(20, 20, 50, 30, WHITE);
  display.drawLine(108, 20, 78, 30, WHITE);
  display.display();
}

void surprisedFace() {
  display.clearDisplay();
  display.drawCircle(37, 30, 5, WHITE);
  display.drawCircle(90, 30, 5, WHITE);
  display.display();
}

void winkFace() {
  display.clearDisplay();
  display.drawLine(25, 32, 37, 24, WHITE);
  display.drawLine(37, 24, 50, 32, WHITE);
  display.drawLine(78, 30, 103, 30, WHITE);
  display.display();
}

// =====================
// WAKE ANIMATION
// =====================

void wakeAnimation() {
  for (int h = 2; h <= 20; h += 2) {
    display.clearDisplay();
    display.fillRoundRect(25, 30 - h / 2, 25, h, 4, WHITE);
    display.fillRoundRect(78, 30 - h / 2, 25, h, 4, WHITE);
    display.display();
    delay(70);
  }
}

// =====================
// THINKING (continuous)
// =====================

void thinkingFrame() {
  display.clearDisplay();

  display.drawRoundRect(25, 20, 25, 20, 4, WHITE);
  display.drawRoundRect(78, 20, 25, 20, 4, WHITE);

  display.fillCircle(32 + pupilX, 30, 3, WHITE);
  display.fillCircle(85 + pupilX, 30, 3, WHITE);

  display.display();

  pupilX += direction;

  if (pupilX > 8) direction = -1;
  if (pupilX < 0) direction = 1;
}

// =====================
// TEXT DISPLAY
// =====================

void showText(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  int y = 0;
  int charsPerLine = 20;

  for (int i = 0; i < msg.length(); i += charsPerLine) {
    display.setCursor(0, y);
    display.print(msg.substring(i, min(i + charsPerLine, (int)msg.length())));
    y += 10;
    if (y > 54) break;
  }

  display.display();
}

// =====================
// SETUP
// =====================

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  sleepyFace();
}

// =====================
// LOOP
// =====================

void loop() {

  if (thinkingMode) {
    thinkingFrame();
    delay(60);
  }

  if (Serial.available()) {

    command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "WAKE") {
      thinkingMode = false;
      wakeAnimation();
    }

    else if (command == "THINK") {
      thinkingMode = true;
    }

    else if (command == "HAPPY") {
      thinkingMode = false;
      happyFace();
    }

    else if (command == "SLEEP") {
      thinkingMode = false;
      sleepyFace();
    }

    else if (command == "SAD") {
      thinkingMode = false;
      sadFace();
    }

    else if (command == "ANGRY") {
      thinkingMode = false;
      angryFace();
    }

    else if (command == "SURPRISED") {
      thinkingMode = false;
      surprisedFace();
    }

    else if (command == "WINK") {
      thinkingMode = false;
      winkFace();
    }

    else if (command.startsWith("TEXT:")) {
      thinkingMode = false;
      showText(command.substring(5));
    }
  }
}