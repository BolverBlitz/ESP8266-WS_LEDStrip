#include "secret.h"

// ----------------------------
// Standard Libraries - Already Installed if you have ESP8266 set up
// ----------------------------

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <WiFiUDP.h>
#include <Hash.h>

#include <Ticker.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <Adafruit_NeoPixel.h>

// ----------------------------
// Setup of Libraries
// ----------------------------

#define PIN 2
#define LEDS 300

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
char packet_buf[1024*2];

const int avaiblePins[9] = { D0, D1, D2, D3, D4, D5, D6, D7, D8 };
int inputPins[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int outputPins[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int lastPinStates[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

char delimiter = ',';
bool rawMode = false;
bool debugMode = false;
bool udpMode = false;

unsigned long startTime = millis();

#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))
#define PACKET_SZ ((LEDS * 4) + 3)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, PIN, NEO_GRBW + NEO_KHZ800);

//------- Replace the following! ------
char ssid[] = WIFI_NAME;      // your network SSID (name)
char password[] = WIFI_PASS;  // your network key

WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  char xArray[4];
  int x;
  int y;
  int d;
  int r;
  int g;
  int b;
  int w;
  int commaCount = 0;
  String inPayload;

  int commas[] = { -1, -1, -1, -1, -1, -1 };  // using 6 for now (x,y,d,r,g,b,w)
  int command;
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
        char buffer[100];
        sprintf(buffer, "INFO:UDP.%s,RAW.%s,DEBUG.%s", udpMode ? "true" : "false", rawMode ? "true" : "false", debugMode ? "true" : "false");
        webSocket.sendTXT(num, buffer);

      }
      break;
    case WStype_TEXT:
      startTime = millis();
      // Serial.printf("[%u] get Text: %s\n", num, payload);
      inPayload = String((char *)payload);
      //Serial.println(inPayload);

      if (inPayload == "CLEAR") {  // Reset Strip
        clearStrip();
      } else if (inPayload == "SHOW") {  // Force show strip
        strip.show();
      } else if (inPayload == "STATUS") {
        char buffer[100];
        sprintf(buffer, "INFO:UDP.%s,RAW.%s,DEBUG.%s", udpMode ? "true" : "false", rawMode ? "true" : "false", debugMode ? "true" : "false");
        webSocket.sendTXT(num, buffer);
      } else if (inPayload == "DEBUG") {
        if (debugMode) {
          debugMode = false;
        } else {
          debugMode = true;
        }
      } else if (inPayload == "UDP") {
        if (!udpMode) {
          udpMode = true;
          char message_data[8] = "UDP:ON";
          webSocket.broadcastTXT(message_data);
        }
      }else if (inPayload == "RAW") {  // RAW Mode
        if (rawMode) {
          rawMode = false;
          char message_data[8] = "RAW:OFF";
          webSocket.broadcastTXT(message_data);
          Serial.println("Raw Mode Off");
        } else {
          rawMode = true;
          char message_data[8] = "RAW:ON";
          webSocket.broadcastTXT(message_data);
          Serial.println("Raw Mode On");
        }
      } else {

        if (rawMode) {

          int pixelIndex = 0;
          int colorIndex = 0;
          uint8_t colors[4] = { 0, 0, 0, 0 };

          int startIndex = 0;
          int commaIndex = inPayload.indexOf(delimiter);

          while (commaIndex != -1) {
            colors[colorIndex] = (uint8_t)inPayload.substring(startIndex, commaIndex).toInt();

            colorIndex++;
            if (colorIndex == 4) {
              strip.setPixelColor(pixelIndex, strip.Color(colors[0], colors[1], colors[2], colors[3]));
              pixelIndex++;
              colorIndex = 0;
            }
            startIndex = commaIndex + 1;
            commaIndex = inPayload.indexOf(delimiter, startIndex);
          }
          strip.show();

          if (debugMode) {
            unsigned long elapsedTime = millis() - startTime;

            float fps = 1000.0 / elapsedTime;  // FPS berechnen

            char fpsMessage[32];
            sprintf(fpsMessage, "FPS: %.2f", fps);
            webSocket.broadcastTXT(fpsMessage);
          }

        } else {

          // clear commas
          // need to makr this use size of
          for (int i = 0; i < ELEMENTS(commas); i++)
            commas[i] = -1;

          // grab all comma positions
          int commaIndex = 0;
          for (int i = 0; i < inPayload.length(); i++) {
            if (inPayload[i] == ',')
              commas[commaIndex++] = i;
          }

          /* commands
              command: x, y, d?, r, g, b, w
              0 = draw pixel -> Resiving
              1 = draw line -> Resiving
              2 = set strip -> Resiving
              3 = set white strip -> Resiving
              7 = emit PIN change -> Sending
              8 = set Pin Mode -> Resiving
              9 = setPin -> Resiving
              */

          // grab command
          int commandSeperator = inPayload.indexOf(":");
          command = inPayload.substring(0, commandSeperator).toInt();
          // Serial.println(command);

          x = inPayload.substring(commandSeperator + 1, commas[0]).toInt();
          y = inPayload.substring(commas[0] + 1, commas[1]).toInt();

          // Serial.print(x);

          if (command == 0)  // draw pixel
          {
            d = inPayload.substring(commas[1] + 1).toInt();
            r = inPayload.substring(commas[2] + 1, commas[3]).toInt();
            g = inPayload.substring(commas[3] + 1, commas[4]).toInt();
            b = inPayload.substring(commas[4] + 1, commas[5]).toInt();
            w = inPayload.substring(commas[5] + 1).toInt();
            strip.setPixelColor(x, strip.Color(r, g, b, w));
            if (d == 1)
              strip.show();
          } else if (command == 1)  // draw line
          {
            d = inPayload.substring(commas[1] + 1).toInt();
            r = inPayload.substring(commas[2] + 1, commas[3]).toInt();
            g = inPayload.substring(commas[3] + 1, commas[4]).toInt();
            b = inPayload.substring(commas[4] + 1, commas[5]).toInt();
            w = inPayload.substring(commas[5] + 1).toInt();

            for (int i = x; i < y; i++) {
              strip.setPixelColor(i, strip.Color(r, g, b, w));
            }

            if (d == 1)
              strip.show();
          } else if (command == 2)  // set strip
          {
            d = inPayload.substring(commas[1] + 1).toInt();
            r = inPayload.substring(commas[2] + 1, commas[3]).toInt();
            g = inPayload.substring(commas[3] + 1, commas[4]).toInt();
            b = inPayload.substring(commas[4] + 1, commas[5]).toInt();
            w = inPayload.substring(commas[5] + 1).toInt();

            for (int i = 0; i < LEDS; i++) {
              strip.setPixelColor(i, strip.Color(r, g, b, w));
            }

            if (d == 1)
              strip.show();
          } else if (command == 3)  // set white strip
          {
            w = inPayload.substring(commas[5] + 1).toInt();
            setWhiteStrip(w);
          } else if (command == 8)  // set GPIO Pin Mode
          {
            if (y == 1) {
              Serial.printf("[%u] PIN %d set to Out\n", x, avaiblePins[x]);
              pinMode(avaiblePins[x], OUTPUT);
              inputPins[x] = 0;
              outputPins[x] = 1;
            } else {
              Serial.printf("[%u] PIN %d set to Inp\n", x, avaiblePins[x]);
              pinMode(avaiblePins[x], INPUT);
              inputPins[x] = 1;
              outputPins[x] = 0;
            }
          } else if (command == 9)  // set GPIO Pin
          {
            if (outputPins[x] == 1) {
              if (y == 1) {
                Serial.printf("[%u] PIN %d set to HIGH\n", x, avaiblePins[x]);
                digitalWrite(avaiblePins[x], HIGH);
              } else {
                Serial.printf("[%u] PIN %d set to LOW\n", x, avaiblePins[x]);
                digitalWrite(avaiblePins[x], LOW);
              }
            } else {
              sendErrorBrodcast(x, "PIN CAN`T BE SET! ITS NOT IN OUTPUT MODE");
            }
          }

          // Serial.print("X: ");
          // Serial.println(x);
          // Serial.print("Y: ");
          // Serial.println(y);
          // Serial.print("Colour: ");
          // Serial.println(colour, HEX);
        }

        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
      }
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }
}

void sendErrorBrodcast(int pin, String error) {
  char pinChar[8];
  char message_data[100] = "ERROR:";

  itoa(pin, pinChar, 10);

  //combining data
  strcat(message_data, pinChar);
  strcat(message_data, ",");
  strcat(message_data, error.c_str());

  webSocket.broadcastTXT(message_data);
}

void setWhiteStrip(int w) {
  for (int i = 0; i < LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, w));
  }

  strip.show();
}

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

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    strip.setPixelColor(0, strip.Color(25, 0, 0, 0)); // Set LED red to indicate connection not yet made
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

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  // Handle WebSocket messages
  webSocket.loop();

  if (udpMode) {
    int noBytes = Udp.parsePacket();

    if (noBytes) {
      // Serial.print("Received ");
      // Serial.print(noBytes);
      // Serial.print(" bytes\r\n");
      Udp.read(packet_buf, noBytes);

      if (noBytes == PACKET_SZ && packet_buf[0] == 0xAA) {
        unsigned short sum = 0;
        int checksum_0 = PACKET_SZ - 2;
        int checksum_1 = PACKET_SZ - 1;

        for (int i = 0; i < checksum_0; i++) {
          sum += packet_buf[i];
        }

        //Test if valid write packet
        if ((((unsigned short)packet_buf[checksum_0] << 8) | packet_buf[checksum_1]) == sum) {
          for (int i = 0; i < LEDS; i++) {
            int idx = 1 + (4 * i);

            strip.setPixelColor(i, strip.Color(packet_buf[idx], packet_buf[idx + 1], packet_buf[idx + 2], packet_buf[idx + 3]));
          }
          strip.show();
        }
      } else if (noBytes == 2 && packet_buf[0] == 0xAB) {
        udpMode = false;
        char message_data[8] = "UDP:OFF";
        webSocket.broadcastTXT(message_data);
      } else if (noBytes == 2 && packet_buf[0] == 0xAC){
        char buffer[100];
        sprintf(buffer, "INFO:UDP.%s,RAW.%s,DEBUG.%s", udpMode ? "true" : "false", rawMode ? "true" : "false", debugMode ? "true" : "false");
        webSocket.broadcastTXT(buffer);
      }
    }
  } else {
    //Serial.println(digitalRead(avaiblePins[3]));
    for (int i = 0; i < sizeof(avaiblePins); i++) {
      if (inputPins[i] == 1) {  // Check if PIN is in input Mode (Set by instruction 8)
        int PinVal = digitalRead(avaiblePins[i]);
        if (PinVal != lastPinStates[i]) {  // Check PINs last state and if its changed we emit a message
          char pinChar[8];
          char valueChar[8];
          // Int -> char*
          itoa(PinVal, valueChar, 10);
          itoa(i, pinChar, 10);
          itoa(PinVal, valueChar, 10);

          char message_data[100] = "7:";
          //combining data
          strcat(message_data, pinChar);
          strcat(message_data, ",");
          strcat(message_data, valueChar);
          strcat(message_data, ",0,0,0,0,0,0");

          webSocket.broadcastTXT(message_data);
          lastPinStates[i] = PinVal;
        }
      }
    }
  }
}