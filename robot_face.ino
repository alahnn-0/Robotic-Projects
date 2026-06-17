// =========================================================
//  ESP8266 Robot Face — Time + OpenWeatherMap Climate
//  Hardware:
//    - ESP8266 (NodeMCU / Wemos D1 Mini)
//    - SSD1306 OLED 128x64  (I2C: SDA=D2, SCL=D1)
//    - Touch sensor or button on D5
//
//  Libraries (install via Arduino Library Manager):
//    - Adafruit GFX Library
//    - Adafruit SSD1306
//    - NTPClient (by Fabrice Weinberg)
//    - ArduinoJson (by Benoit Blanchon)  ← NEW
// =========================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------
// CONFIG
// ---------------------------------------------------------

const char* WIFI_SSID     = "RABBITSQUARE_4G";
const char* WIFI_PASSWORD = "rsq@9846";

const char* OWM_API_KEY   = "494902342fc5e2a0bce6deb47f6f7d15";
const char* OWM_CITY      = "Idukki";
const char* OWM_COUNTRY   = "IN";

// UTC offset in seconds (IST = +5:30 = 19800)
const long UTC_OFFSET_SEC = 19800;

// Pins
#define TOUCH_PIN D5

// OLED
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

// Timeouts
#define INFO_TIMEOUT      10000   // ms info screen stays on
#define BLINK_INTERVAL     5000   // ms between idle blinks
#define WEATHER_INTERVAL  300000  // fetch weather every 5 min

// ---------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_SEC, 60000);

// Touch
int  touchCount     = 0;
bool lastTouchState = LOW;

// Info screen
bool          showInfo  = false;
unsigned long infoStart = 0;
int           infoScreen = 0;   // 0 = time, 1 = weather

// Blink
unsigned long lastBlink = 0;

// Weather cache
float         weatherTemp     = NAN;
float         weatherFeels    = NAN;
int           weatherHumidity = -1;
String        weatherDesc     = "";
String        weatherCity     = "";
unsigned long lastWeatherFetch = 0;
bool          weatherFetched   = false;


// =========================================================
//  WIFI — show progress on OLED
// =========================================================

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi");
  display.println(WIFI_SSID);
  display.display();

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    display.print(".");
    display.display();
    dots++;
    if (dots > 30) ESP.restart();
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi connected!");
  display.setCursor(0, 16);
  display.println(WiFi.localIP().toString());
  display.display();
  delay(1200);
}


// =========================================================
//  WEATHER FETCH
// =========================================================

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;

  String url = "http://api.openweathermap.org/data/2.5/weather?q=";
  url += OWM_CITY;
  url += ",";
  url += OWM_COUNTRY;
  url += "&appid=";
  url += OWM_API_KEY;
  url += "&units=metric";

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (!err) {
      weatherTemp     = doc["main"]["temp"].as<float>();
      weatherFeels    = doc["main"]["feels_like"].as<float>();
      weatherHumidity = doc["main"]["humidity"].as<int>();
      weatherDesc     = doc["weather"][0]["description"].as<String>();
      weatherCity     = doc["name"].as<String>();
      weatherFetched  = true;
    }
  }

  http.end();
  lastWeatherFetch = millis();
}


// =========================================================
//  DISPLAY HELPERS
// =========================================================

void drawEyes(int h) {
  display.clearDisplay();
  int y = 18 + (28 - h) / 2;
  display.fillRoundRect(18, y, 38, h, 8, WHITE);
  display.fillRoundRect(72, y, 38, h, 8, WHITE);
  display.display();
}


// =========================================================
//  FACE EXPRESSIONS
// =========================================================

void sleepFace() {
  display.clearDisplay();
  display.fillRoundRect(18, 30, 38, 4, 2, WHITE);
  display.fillRoundRect(72, 30, 38, 4, 2, WHITE);
  display.display();
}

void neutralFace() {
  display.clearDisplay();
  display.fillRoundRect(18, 18, 38, 28, 8, WHITE);
  display.fillRoundRect(72, 18, 38, 28, 8, WHITE);
  display.display();
}

void happyFace() {
  display.clearDisplay();
  display.fillTriangle(18, 35, 56, 18, 56, 35, WHITE);
  display.fillTriangle(72, 35, 110, 18, 110, 35, WHITE);
  display.display();
}

void sadFace() {
  display.clearDisplay();
  display.drawLine(18, 20, 56, 38, WHITE);
  display.drawLine(72, 38, 110, 20, WHITE);
  display.display();
}

void angryFace() {
  display.clearDisplay();
  display.fillTriangle(18, 18, 56, 35, 18, 35, WHITE);
  display.fillTriangle(72, 35, 110, 18, 110, 35, WHITE);
  display.display();
}

void surprisedFace() {
  display.clearDisplay();
  display.drawCircle(37, 32, 18, WHITE);
  display.fillCircle(37, 32, 10, WHITE);
  display.drawCircle(91, 32, 18, WHITE);
  display.fillCircle(91, 32, 10, WHITE);
  display.display();
}
void lookAroundAnimation() {

  // {offset, hold_ms}
  int moves[][2] = {
    {  0,  200 },   // center pause
    { -10, 400 },   // look left
    {  0,  200 },   // back to center
    {  10, 400 },   // look right
    {  0,  200 },   // back to center
  };

  int numMoves = 5;
  int slideSteps = 8;

  int currentOx = 0;

  for (int m = 0; m < numMoves; m++) {
    int targetOx = moves[m][0];
    int holdMs   = moves[m][1];

    // Slide smoothly to target
    for (int s = 1; s <= slideSteps; s++) {
      float t    = (float)s / slideSteps;
      float ease = t < 0.5 ? 2*t*t : -1 + (4 - 2*t)*t;
      int   ox   = (int)(currentOx + (targetOx - currentOx) * ease);

      display.clearDisplay();
      display.fillRoundRect(18 + ox, 18, 38, 28, 8, WHITE);
      display.fillRoundRect(72 + ox, 18, 38, 28, 8, WHITE);
      display.display();
      delay(18);
    }

    currentOx = targetOx;
    delay(holdMs);
  }

  neutralFace();
}

// =========================================================
//  ANIMATIONS
// =========================================================

void wakeAnimation() {
  for (int h = 2; h <= 20; h += 2) { drawEyes(h); delay(23); }
  neutralFace();
}

void blinkAnimation() {
  for (int h = 20; h >= 2; h -= 2) { drawEyes(h); delay(10); }
  for (int h = 2;  h <= 20; h += 2) { drawEyes(h); delay(10); }
  neutralFace();
}

void sleepAnimation() {
  for (int h = 28; h >= 4; h -= 2) { drawEyes(h); delay(30); }
  sleepFace();
}


// =========================================================
//  INFO SCREENS
// =========================================================

void showTimeScreen() {
  timeClient.update();

  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("  Current Time (IST)");
  display.drawLine(0, 10, 127, 10, WHITE);

  display.setTextSize(2);
  display.setCursor(8, 20);
  display.println(timeClient.getFormattedTime());

  const char* days[] = {
    "Sunday","Monday","Tuesday","Wednesday",
    "Thursday","Friday","Saturday"
  };
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println(days[timeClient.getDay()]);

  display.display();
}

void showWeatherScreen() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (!weatherFetched) {
    display.println("Fetching weather...");
    display.display();
    fetchWeather();
    if (!weatherFetched) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Weather fetch failed");
      display.setCursor(0, 16);
      display.println("Check WiFi / API key");
      display.display();
      return;
    }
  }

  // City name header
  String header = weatherCity;
  if (header.length() > 14) header = header.substring(0, 14);
  display.println(header);
  display.drawLine(0, 10, 127, 10, WHITE);

  // Temperature (large)
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print(weatherTemp, 1);
  display.print((char)247);  // degree symbol
  display.println("C");

  // Feels like
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.print("Feels: ");
  display.print(weatherFeels, 1);
  display.print((char)247);
  display.println("C");

  // Humidity
  display.setCursor(0, 46);
  display.print("Humidity: ");
  display.print(weatherHumidity);
  display.println("%");

  // Description
  String desc = weatherDesc;
  if (desc.length() > 0) desc[0] = toupper(desc[0]);
  if (desc.length() > 21) desc = desc.substring(0, 21);
  display.setCursor(0, 56);
  display.println(desc);

  display.display();
}


// =========================================================
//  TOUCH HANDLER
//  1 → Wake
//  2 → Time screen
//  3 → Weather screen
//  4 → Happy
//  5 → Sad
//  6 → Angry
//  7 → Surprised
//  8 → Sleep + reset
// =========================================================

void handleTouch() {
  bool touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH && lastTouchState == LOW) {
    delay(50);  // debounce
    touchCount++;
    showInfo  = false;
    infoStart = 0;

    switch (touchCount) {

      case 1:
        wakeAnimation();
        break;

      case 2:
        infoScreen = 0;
        showInfo   = true;
        infoStart  = millis();
        showTimeScreen();
        break;

      case 3:
        infoScreen = 1;
        showInfo   = true;
        infoStart  = millis();
        showWeatherScreen();
        break;

      case 4:
        happyFace();
        break;

      case 5:
        sadFace();
        break;

      case 6:
        angryFace();
        break;

      case 7:
        surprisedFace();
        // lookAroundAnimation();
        break;

      case 8:
        sleepAnimation();
        touchCount = 0;
        break;

      default:
        touchCount = 0;
        neutralFace();
        // lookAroundAnimation();
        break;
    }
  }

  lastTouchState = touchState;
}


// =========================================================
//  SETUP
// =========================================================

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);

  Wire.begin(D2, D1);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    for (;;);
  }

  display.clearDisplay();
  display.display();

  connectWiFi();
  timeClient.begin();

  // Initial weather fetch at startup
  fetchWeather();

  sleepFace();
}


// =========================================================
//  MAIN LOOP
// =========================================================

void loop() {
  handleTouch();

  // Background weather refresh every 5 minutes
  if (millis() - lastWeatherFetch > WEATHER_INTERVAL) {
    fetchWeather();
  }

  if (showInfo) {
    if (infoScreen == 0) {
      showTimeScreen();
    } else {
      showWeatherScreen();
    }

    if (millis() - infoStart > INFO_TIMEOUT) {
      showInfo = false;
      neutralFace();
      lookAroundAnimation();
    }

  } else {
    if (millis() - lastBlink > BLINK_INTERVAL) {
      blinkAnimation();
      lookAroundAnimation();
      lastBlink = millis();
    }
  }
}
