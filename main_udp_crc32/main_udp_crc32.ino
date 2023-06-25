#include "secret.h"

// ----------------------------
// Standard Libraries - Already Installed if you have ESP8266 set up
// ----------------------------

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <Hash.h>

#include <Ticker.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <Adafruit_NeoPixel.h>
#include <CRC32.h>

// ----------------------------
// Setup of Libraries
// ----------------------------

#define PIN 2
#define LEDS 300

CRC32 crc;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
char packet_buf[1024 * 2];

#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))
#define PACKET_SZ ((LEDS * 4) + 1 + 4)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, PIN, NEO_GRBW + NEO_KHZ800);

//------- Replace the following! ------
char ssid[] = WIFI_NAME;      // your network SSID (name)
char password[] = WIFI_PASS;  // your network key

void clearStrip() {
  for (int i = 0; i < LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 0));
  }

  strip.show();
}

void setup() {

  Serial.begin(115200);

  strip.begin();
  yield();

  Udp.begin(82);

  clearStrip();

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  unsigned long startTime = millis();  // Tracker for how long we've been trying to connect to WiFi

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // Restart ESP if it's been trying to connect for more than 10 seconds
    unsigned long elapsedTime = millis() - startTime;
    if (elapsedTime > 10000) {
      Serial.println("Failed to connect, restarting");
      ESP.restart();
    }

    strip.setPixelColor(0, strip.Color(25, 0, 0, 0));  // Set LED red to indicate connection not yet made
    strip.show();
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  // Light up LED green to indicate successful connection:
  strip.setPixelColor(0, strip.Color(0, 25, 0, 0));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 0, 0, 0));
  strip.show();
}

void loop() {
  int noBytes = Udp.parsePacket();

  if (noBytes) {
    //Serial.print("Received ");
    //Serial.print(noBytes);
    //Serial.print(" bytes\r\n");
    Udp.read(packet_buf, noBytes);

    if (noBytes == PACKET_SZ && packet_buf[0] == 0xAA) {
      uint32_t receivedCrc = ((uint32_t)packet_buf[PACKET_SZ - 1] << 24) | ((uint32_t)packet_buf[PACKET_SZ - 2] << 16) | ((uint32_t)packet_buf[PACKET_SZ - 3] << 8) | (uint32_t)packet_buf[PACKET_SZ - 4];

      crc.reset();
      crc.update(packet_buf, PACKET_SZ - 4);

      uint32_t calculatedCrc = crc.finalize();

      if (receivedCrc == calculatedCrc) {
        for (int i = 0; i < LEDS; i++) {
          int idx = 1 + (4 * i);

          strip.setPixelColor(i, strip.Color(packet_buf[idx], packet_buf[idx + 1], packet_buf[idx + 2], packet_buf[idx + 3]));
        }
        strip.show();
      } else {
        //Serial.print("CRC mismatch: received ");
        //Serial.print(receivedCrc, HEX);
        //Serial.print(", calculated ");
        //Serial.println(calculatedCrc, HEX);
      }
    } else if (noBytes != PACKET_SZ) {
      //Serial.print("Unexpected packet size: ");
      //Serial.println(noBytes);
    } else if (packet_buf[0] != 0xAA) {
      //Serial.println("Invalid start byte");
    }
  }
}