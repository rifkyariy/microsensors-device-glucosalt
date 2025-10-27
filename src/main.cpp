#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <string.h>
#include <ArduinoJson.h> 
MAX30105 particleSensor;

// --- WiFi / API configuration ---
const char* WIFI_SSID = "OJOKEPO";
const char* WIFI_PASSWORD = "mitlab910";
const char* API_BASE_URL = "https://api-glucosalt.heretichydra.xyz";
const char* API_PATH = "/health/metrics";
const char* DEVICE_ID = "ESP32_001";
const char* USER_ID = "64a6f31f-68c4-4122-b406-55dc2d75703e";
const char* API_KEY = "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZ6d3lqdGJhc3V6bnFkenhibXdpIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjEzMjYyODEsImV4cCI6MjA3NjkwMjI4MX0";

// --- Sensor settings ---
#define SAMPLE_RATE_HZ 100      // 100 samples per second
#define BUFFER_LENGTH 100       // We will keep a 1-second rolling buffer (100Hz * 1s)
#define UPDATE_INTERVAL_MS 250  // Send new data 4 times per second (every 250ms)

uint32_t irBuffer[BUFFER_LENGTH];
uint32_t redBuffer[BUFFER_LENGTH];

int32_t spo2;
int8_t validSpo2;
int32_t heartRate;
int8_t validHeartRate;

unsigned long lastUpdateTime = 0; // For timing the updates

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Connecting to WiFi '%s' ...\n", WIFI_SSID);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000) {
      Serial.println("\nWiFi connect timeout, rebooting...");
      ESP.restart();
    }
  }
  Serial.printf("\nConnected, IP: %s\n", WiFi.localIP().toString().c_str());
}

bool httpPostJson(const String &json) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  String url = String(API_BASE_URL) + String(API_PATH);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  if (strlen(API_KEY) > 0) {
    http.addHeader("Authorization", String("Bearer ") + API_KEY);
  }
  http.setTimeout(3000); // 3 second timeout
  int httpCode = http.POST(json);
  bool ok = false;
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.printf("POST -> %d\n", httpCode);
    ok = (httpCode >= 200 && httpCode < 300);
  } else {
    Serial.printf("HTTP POST failed, err: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  memset(irBuffer, 0, sizeof(irBuffer));
  memset(redBuffer, 0, sizeof(redBuffer));

  connectWiFi();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring.");
    while (1) delay(1000);
  }

  byte ledBrightness = 0x1F;
  byte sampleAverage = 4;
  byte ledMode = 2; // Note: Mode 2 is Red + IR.
  int pulseWidth = 411;
  int adcRange = 4096;
  int sampleRate = SAMPLE_RATE_HZ;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  
  // --- NEW: Prime the data buffers ---
  // We must fill the buffers with 1 second of data before we can start
  // the real-time sliding window.
  Serial.println("Priming data buffer (collecting first 1s of data)...");
  for (int i = 0; i < BUFFER_LENGTH; i++) {
    while (particleSensor.available() == false) {
      particleSensor.check();
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }
  Serial.println("Buffer primed. Starting real-time updates.");
  lastUpdateTime = millis(); // Set the timer to start now
}

void loop() {
  
  // This is the "Sliding Window" logic.
  // 1. Check if a new sample is available from the sensor.
  // (At 100Hz, this will be true every 10ms)
  
  particleSensor.check(); // Check for new data
  
  while (particleSensor.available()) {
    // 2. A new sample is available. "Slide" the buffers.
    
    // Shift all elements in the buffers to the left by one position
    // This discards the oldest sample (at index 0)
    memmove(irBuffer, irBuffer + 1, (BUFFER_LENGTH - 1) * sizeof(uint32_t));
    memmove(redBuffer, redBuffer + 1, (BUFFER_LENGTH - 1) * sizeof(uint32_t));

    // 3. Add the new sample to the end of the buffers
    irBuffer[BUFFER_LENGTH - 1] = particleSensor.getIR();
    redBuffer[BUFFER_LENGTH - 1] = particleSensor.getRed();
    
    particleSensor.nextSample(); // Move to the next sample in the FIFO
  }

  // 4. Check if it's time to process and send an update
  // We do this on a timer (every 250ms) instead of every single sample.
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL_MS) {
    lastUpdateTime = millis(); // Reset the update timer

    // Process the *entire* 1-second rolling buffer
    maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_LENGTH, redBuffer,
                                           &spo2, &validSpo2, &heartRate, &validHeartRate);

    Serial.printf("HR: %d (valid:%d) | SpO2: %d (valid:%d)\n",
                  heartRate, validHeartRate, spo2, validSpo2);

    // Send data if valid
    if (validHeartRate && validSpo2) {
      int hr = heartRate;
      if (hr < 30) hr = 30;
      if (hr > 220) hr = 220;
      int spo2_clamped = spo2;
      if (spo2_clamped < 50) spo2_clamped = 50;
      if (spo2_clamped > 100) spo2_clamped = 100;

      if (hr >= 30 && hr <= 220 && spo2_clamped >= 60) {
        
        StaticJsonDocument<4096> doc;
        doc["device_id"] = DEVICE_ID;
        doc["user_id"] = USER_ID;
        doc["heart_rate"] = hr;
        doc["blood_oxygen"] = spo2_clamped;

        JsonArray ppg_ir = doc.createNestedArray("ppg_ir");
        for (int i = 0; i < BUFFER_LENGTH; i++) {
          ppg_ir.add(irBuffer[i]);
        }

        JsonArray ppg_red = doc.createNestedArray("ppg_red");
        for (int i = 0; i < BUFFER_LENGTH; i++) {
          ppg_red.add(redBuffer[i]);
        }

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        // Send the payload
        // Serial.println(jsonPayload); // Uncomment to debug the JSON
        httpPostJson(jsonPayload);

      } else {
        Serial.println("Out of range, skipping send."); 
      }
    } else {
      Serial.println("Invalid reading, skipping send.");
    }
    
    Serial.println("---");
  }
}

