/*
 * Cordia Labs — ESP32 Tactile UDP Bridge
 *
 * High-frequency (200 Hz) contact pressure streaming for dual robot jaws.
 * Reads 4x RP-S40-ST thin-film FSR pressure sensors via ADC1:
 *   GPIO 34: Left Fixed Jaw   (left_jaw_a)
 *   GPIO 35: Left Moving Jaw  (left_jaw_b)
 *   GPIO 32: Right Fixed Jaw  (right_jaw_a)
 *   GPIO 33: Right Moving Jaw (right_jaw_b)
 *
 * Broadcasts 24-byte little-endian telemetry packets over UDP.
 * Hardware timestamped via micros() at sample time.
 */

#include <WiFi.h>
#include <WiFiUdp.h>

#if __has_include("config.h")
#include "config.h"
#else
#include "config.h.example"
#endif

#ifndef UDP_TARGET_PORT
#define UDP_TARGET_PORT 5006
#endif

// ADC Pin Assignments (ADC1 only, guaranteed safe during Wi-Fi operation)
static const int PIN_LEFT_FIXED   = 34;
static const int PIN_LEFT_MOVING  = 35;
static const int PIN_RIGHT_FIXED  = 32;
static const int PIN_RIGHT_MOVING = 33;

// Telemetry Packet Structure (Exactly 24 Bytes, Little-Endian)
#pragma pack(push, 1)
struct TactilePacket {
  uint32_t seq;           // Monotonic sequence number
  uint32_t t_us;          // Hardware micros() timestamp at ADC sample time
  float left_fixed;       // 12-bit ADC raw reading (0 - 4095)
  float left_moving;
  float right_fixed;
  float right_moving;
};
#pragma pack(pop)
static_assert(sizeof(TactilePacket) == 24, "TactilePacket must be exactly 24 bytes");

static WiFiUDP udp;
static TactilePacket packet{};
static uint32_t sequenceCounter = 0;
static const uint32_t LOOP_HZ = 200;               // 200 Hz high-frequency tactile polling
static const uint32_t LOOP_US = 1000000 / LOOP_HZ; // 5000 microseconds (5 ms period)
static uint32_t nextLoopUs = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n==========================================");
  Serial.println(" Cordia ESP32 Tactile UDP Bridge (200 Hz) ");
  Serial.println("==========================================");

  // Configure ADC: 12-bit (0-4095) and full 0-3.3V range (11dB attenuation)
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  pinMode(PIN_LEFT_FIXED, INPUT);
  pinMode(PIN_LEFT_MOVING, INPUT);
  pinMode(PIN_RIGHT_FIXED, INPUT);
  pinMode(PIN_RIGHT_MOVING, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to Wi-Fi '%s' ...\n", WIFI_SSID);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 4000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi Connected! IP: %s  RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.println("WiFi auto-connecting in background...");
  }

  udp.begin(UDP_TARGET_PORT);
  Serial.printf("Streaming UDP to port %d @ %d Hz\n\n", UDP_TARGET_PORT, LOOP_HZ);
  nextLoopUs = micros() + LOOP_US;
}

void loop() {
  uint32_t now = micros();
  if ((int32_t)(now - nextLoopUs) < 0) {
    return;
  }
  nextLoopUs += LOOP_US;
  if ((int32_t)(now - nextLoopUs) > (int32_t)LOOP_US) {
    nextLoopUs = now + LOOP_US; // catch up on overflow
  }

  // Sample ADC1 channels
  packet.seq          = sequenceCounter++;
  packet.t_us         = now;
  packet.left_fixed   = (float)analogRead(PIN_LEFT_FIXED);
  packet.left_moving  = (float)analogRead(PIN_LEFT_MOVING);
  packet.right_fixed  = (float)analogRead(PIN_RIGHT_FIXED);
  packet.right_moving = (float)analogRead(PIN_RIGHT_MOVING);

  // Broadcast packet
  udp.beginPacket(UDP_TARGET_IP, UDP_TARGET_PORT);
  udp.write(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
  udp.endPacket();
}
