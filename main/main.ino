#include "secret.h"

// ----------------------------
// Standard Libraries - Already Installed if you have ESP8266 set up
// ----------------------------

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
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

const int avaiblePins[9] = { D0, D1, D2, D3, D4, D5, D6, D7, D8 };
int inputPins[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int outputPins[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int lastPinStates[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

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
      }
      break;
    case WStype_TEXT:
      // Serial.printf("[%u] get Text: %s\n", num, payload);
      inPayload = String((char *)payload);
      //Serial.println(inPayload);

      // Reset Strip

      if (inPayload == "CLEAR") {
        clearStrip();
      } else if (inPayload == "SHOW")  // force show strip
      {
        strip.show();
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
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
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