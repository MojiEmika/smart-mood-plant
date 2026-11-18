#define BLYNK_TEMPLATE_ID "TMPL6-Cb6oeoF"
#define BLYNK_TEMPLATE_NAME "Smart Mood Plant"
#include "secrets.h"   // BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS, THINGSPEAK_API_KEY (not committed)

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "DHT.h"
#include <math.h>

// ==== CONFIG WIFI & THINGSPEAK ====
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;
String apiKey = THINGSPEAK_API_KEY;
const char* server = "http://api.thingspeak.com/update";

// ==== SENSOR PIN ====
#define SOIL_MOISTURE_PIN 34
#define LDR_PIN 35
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ==== OLED ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==== POS for Face (ขวาจอ) ====
int cx = 100;    // X center ของหน้า
int cyBase = 32; // Y center

// ==== MODE & Threshold ====
int mode = 0;   // 1=น้ำน้อย, 2=ปานกลาง, 3=มาก
int happyLevel = 40;
int loveLevel  = 60;

// ==== Animation State ====
int blinkPhase = 0;
unsigned long nextBlink = 0;
int offsetY = 0;
bool goingDown = true;

// ==== Plant Name ====
String plantName = "My Plant";   // ชื่อ default

// ==== รับค่าจากแอพ Blynk ====
// เลือกโหมดน้ำ
BLYNK_WRITE(V1) {
  mode = param.asInt();
  if (mode == 1) { happyLevel = 20; loveLevel = 40; }   // น้ำน้อย
  if (mode == 2) { happyLevel = 40; loveLevel = 60; }   // น้ำปานกลาง
  if (mode == 3) { happyLevel = 60; loveLevel = 80; }   // น้ำมาก
}

// ตั้งชื่อพืช
BLYNK_WRITE(V6) {
  plantName = param.asStr();
}

// ---- Helper Drawing ----
void drawHeart(int x, int y, int s) {
    display.fillCircle(x - s / 2, y, s / 2, SH110X_WHITE);
    display.fillCircle(x + s / 2, y, s / 2, SH110X_WHITE);
    display.fillTriangle(x - s, y, x + s, y, x, y + (s * 3) / 2, SH110X_WHITE);
}
void drawTear(int x, int y) {
    display.drawLine(x, y, x, y + 4, SH110X_WHITE);
    display.drawPixel(x - 1, y + 4, SH110X_WHITE);
    display.drawPixel(x + 1, y + 4, SH110X_WHITE);
}
// ฟังก์ชันวาด Arc (แทน drawArc)
void drawArcHelper(int x0, int y0, int r, int startAngle, int endAngle, uint16_t color) {
  for (int angle = startAngle; angle <= endAngle; angle++) {
    float rad = angle * 3.14159 / 180.0;
    int x = x0 + r * cos(rad);
    int y = y0 + r * sin(rad);
    display.drawPixel(x, y, color);
  }
}

// ---- Animation Update ----
void updateAnimation() {
  unsigned long now = millis();

  // กระพริบตา
  if (blinkPhase == 0 && now > nextBlink) {
    blinkPhase = 1;
    nextBlink = now + 120;
  } else if (blinkPhase == 1 && now > nextBlink) {
    blinkPhase = 2;
    nextBlink = now + 120;
  } else if (blinkPhase == 2 && now > nextBlink) {
    blinkPhase = 0;
    nextBlink = now + random(2500, 6000);
  }

  // เด้งขึ้นลง
  if (goingDown) offsetY++;
  else offsetY--;
  if (offsetY > 2) goingDown = false;
  if (offsetY < -2) goingDown = true;
}

// ---- Draw Face (Right Half) ----
void drawFaceRight(const char* mood, int soilPercent, float temp, float hum, int lightPercent) {
    int cy = cyBase + offsetY;

    // ==== Left side: Sensor Values ====
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("Soil: "); display.print(soilPercent); display.println("%");

    display.setCursor(0, 12);
    display.print("Temp: "); display.print(temp); display.println("C");

    display.setCursor(0, 24);
    display.print("Hum: "); display.print(hum); display.println("%");

    display.setCursor(0, 36);
    display.print("Light: "); display.print(lightPercent); display.println("%");

    // ==== Plant Name (from Blynk V6) ====
    display.setCursor(0, 54);
    display.print("Name: ");
    display.print(plantName);

    // ==== Right side: Face ====
    display.drawCircle(cx, cy, 20, SH110X_WHITE);

    // 👀 Eyes
    if (blinkPhase == 0) {
      display.fillCircle(cx - 7, cy - 6, 3, SH110X_WHITE);
      display.fillCircle(cx + 7, cy - 6, 3, SH110X_WHITE);
    } else {
      display.drawLine(cx - 10, cy - 6, cx - 4, cy - 6, SH110X_WHITE);
      display.drawLine(cx + 4, cy - 6, cx + 10, cy - 6, SH110X_WHITE);
    }

    // 👄 Mouth & Eyebrows
    if (!strcmp(mood, "HAPPY")) {
        drawArcHelper(cx, cy + 10, 8, 20, 160, SH110X_WHITE);   // ยิ้ม
    } else if (!strcmp(mood, "NEUTRAL")) {
        display.drawLine(cx - 6, cy + 10, cx + 6, cy + 10, SH110X_WHITE); // ปากตรง
    } else if (!strcmp(mood, "SAD")) {
        drawArcHelper(cx, cy + 14, 8, 200, 340, SH110X_WHITE);  // ปากโค้งลง
        drawTear(cx - 12, cy);
        drawTear(cx + 12, cy);
    } else if (!strcmp(mood, "ANGRY")) {
        display.drawLine(cx - 6, cy + 12, cx + 6, cy + 8, SH110X_WHITE); // ปากเฉียง
        display.drawLine(cx - 12, cy - 10, cx - 4, cy - 6, SH110X_WHITE); // คิ้ว
        display.drawLine(cx + 12, cy - 10, cx + 4, cy - 6, SH110X_WHITE);
    } else if (!strcmp(mood, "LOVE")) {
        drawArcHelper(cx, cy + 10, 6, 20, 160, SH110X_WHITE);   // ยิ้มเล็ก
        display.fillCircle(cx - 7, cy - 6, 2, SH110X_WHITE);    // ตาโต
        display.fillCircle(cx + 7, cy - 6, 2, SH110X_WHITE);

        // หัวใจเล็กๆ ข้างๆ
        drawHeart(cx + 20, cy - 10, 6);  
        if ((millis() / 600) % 2) drawHeart(cx - 20, cy - 6, 5); // กระพริบหัวใจ
    }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // WIFI & Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // OLED
  if (!display.begin(0x3C, true)) {
    Serial.println("OLED init failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 20);
  display.print("Smart Plant Pot");
  display.display();
  delay(2000);
}

void loop() {
  Blynk.run();
  updateAnimation();

  // === Read sensors ===
  int soilValue = analogRead(SOIL_MOISTURE_PIN);
  int soilPercent = map(soilValue, 4095, 1000, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int lightValue = analogRead(LDR_PIN);
  int lightPercent = map(lightValue, 0, 4095, 0, 100);

  // === Mood by soil ===
  const char* mood;
  if (soilPercent > loveLevel) mood = "LOVE";
  else if (soilPercent > happyLevel) mood = "HAPPY";
  else if (soilPercent > 35) mood = "NEUTRAL";
  else if (soilPercent > 20) mood = "SAD";
  else mood = "ANGRY";

  // === Draw on OLED ===
  display.clearDisplay();
  drawFaceRight(mood, soilPercent, temp, hum, lightPercent);
  display.display();

  // === Send to Blynk ===
  Blynk.virtualWrite(V2, soilPercent);
  Blynk.virtualWrite(V3, temp);
  Blynk.virtualWrite(V4, hum);
  Blynk.virtualWrite(V5, lightPercent);

  // === Blynk Event Notification ===
  if (soilPercent < happyLevel) {
    Blynk.logEvent("soillow", "ดินแห้งแล้ว 🌱 น้องต้นไม้อยากได้น้ำเพิ่ม 💧");
  }

  // === Send to ThingSpeak ===
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + soilPercent +
                 "&field2=" + String(temp) +
                 "&field3=" + String(lightPercent) +
                 "&field4=" + String(hum);
    http.begin(url);
    http.GET();
    http.end();
  }

  delay(200);
}
