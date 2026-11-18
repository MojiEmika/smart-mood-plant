# Smart Mood Plant 🌱

An IoT smart plant pot built on the **ESP32**. It monitors soil moisture, temperature, humidity and light, shows the plant's "mood" as an animated face on an OLED screen, and sends live data to the **Blynk** mobile app and **ThingSpeak**.

## Demo

[![Smart Mood Plant demo video](https://img.youtube.com/vi/6kH08XCf7fA/hqdefault.jpg)](https://youtu.be/6kH08XCf7fA)

*Click the image to watch the presentation on YouTube.*

## Features

- **Sensor monitoring** — soil moisture, temperature & humidity (DHT11), and light level (LDR)
- **Animated mood face on OLED** — the plant shows one of five moods based on soil moisture: **LOVE → HAPPY → NEUTRAL → SAD → ANGRY**, with blinking eyes, a gentle bounce, tears and hearts
- **Blynk mobile app**
  - Live dashboard of all sensor values
  - Choose a watering mode for the plant type (low / medium / high water), which changes the mood thresholds
  - Give the plant a name that appears on the OLED screen
  - Push notification when the soil gets too dry
- **ThingSpeak** — logs sensor data to the cloud for charts and history

## Hardware

| Component | Pin |
| --- | --- |
| ESP32 dev board | — |
| Soil moisture sensor | GPIO 34 (analog) |
| LDR light sensor | GPIO 35 (analog) |
| DHT11 temperature & humidity sensor | GPIO 4 |
| 128×64 OLED (SH1106, I²C address 0x3C) | SDA / SCL |

## How it works

| Watering mode (Blynk V1) | HAPPY above | LOVE above |
| --- | --- | --- |
| 1 — Low water | 20% | 40% |
| 2 — Medium water (default) | 40% | 60% |
| 3 — High water | 60% | 80% |

Below the HAPPY threshold the face turns **NEUTRAL** (> 35%), **SAD** (> 20%) and finally **ANGRY**.

**Blynk virtual pins:** V1 watering mode · V2 soil % · V3 temperature · V4 humidity · V5 light % · V6 plant name · event `soillow`

**ThingSpeak fields:** 1 soil % · 2 temperature · 3 light % · 4 humidity

## Tech stack

- **Language:** C++ (Arduino)
- **Board:** ESP32 (Arduino IDE)
- **Libraries:** Blynk, Adafruit GFX, Adafruit SH110X, DHT sensor library, WiFi / HTTPClient
- **Cloud:** Blynk IoT, ThingSpeak

## My role

This was a team project. A teammate wired up the sensors and hardware. My part:

- Debugged the circuit and wiring
- Wrote the Blynk integration and designed the UI/UX of the Blynk app dashboard
- Designed the OLED display — the animated plant faces and moods
- Connected the ESP32 to ThingSpeak for cloud data logging

## Getting started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and the **ESP32 board package**.
2. Install the libraries from Library Manager: `Blynk`, `Adafruit GFX Library`, `Adafruit SH110X`, `DHT sensor library`.
3. Copy `SmartMoodPlant/secrets.example.h` to `SmartMoodPlant/secrets.h` and fill in your Wi-Fi, Blynk auth token and ThingSpeak write API key.
4. Open `SmartMoodPlant/SmartMoodPlant.ino`, select your ESP32 board and port, then upload.

## Future improvements

- Send data to ThingSpeak on a timer (free accounts accept one update every 15 seconds) instead of on every loop
- Send the "soil low" notification once per dry spell rather than repeatedly
- Use `BlynkTimer` instead of `delay()` so the loop stays non-blocking
