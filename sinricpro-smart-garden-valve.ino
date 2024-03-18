/*
   Example for how to use SinricPro Switch device:
   - setup a switch device
   - handle request using callback (turn on/off builtin led indicating device power state)
   - send event to sinricPro server (flash button is used to turn on/off device manually)

*/

// Uncomment the following line to enable serial debug output
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>


#include "SinricPro.h"
#include "SinricProSwitch.h"

#define WIFI_SSID         "shankundan4G"
#define WIFI_PASS         "123SsCrackThePassword6568"
#define APP_KEY           "cadc947b-b59f-44df-9c43-918b958d39c1"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "6cf8577c-cf8a-4ae5-8af9-c627b1dc1de6-64d2b90c-687a-4812-83df-f99223fe96d4"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define SWITCH_ID         "623dde9bd0fd258c5200f634"    // Should look like "5dc1564130xxxxxxxxxxxxxx"
#define BAUD_RATE         9600                // Change baudrate to your need

#define CONTROL_PIN1 D1          // GPIO for control input 1
#define CONTROL_PIN2 D2          // GPIO for control input 2
#define BUTTON_PIN D8            // GPIO for manual BUTTON control
#define WIFI_PIN   LED_BUILTIN   // GPIO for WIFI LED (inverted)
#define VALVE_STATUS_PIN D7      // Pin that indictes valve status
#define TIMEOUT 30               // valve power timeout
#define BUTTON_TIMEOUT 1000      // minimum time the button should wait to change the state back

bool isValveOpen = false;     // valve status
unsigned long lastBtnPress = 0;
unsigned long lastValveOpen = 0;

void cutSupplyToValve() {
  digitalWrite(CONTROL_PIN1, LOW);
  digitalWrite(CONTROL_PIN2, LOW);
  Serial.println("Valve supply is cut off");
}

// Change D1, D2 to 1 0 for 25ms
void openValve() {
  lastValveOpen = millis();
  Serial.println("Opening valve");
  digitalWrite(CONTROL_PIN1, HIGH);
  digitalWrite(CONTROL_PIN2, LOW);
  digitalWrite(VALVE_STATUS_PIN, HIGH);
  isValveOpen = 1;
  delay(TIMEOUT);
  cutSupplyToValve();
}

// Chnage D1, D2 to 0 1 for 25ms
void closeValve() {
  Serial.println("Closing valve");
  digitalWrite(CONTROL_PIN1, LOW);
  digitalWrite(CONTROL_PIN2, HIGH);
  digitalWrite(VALVE_STATUS_PIN, LOW);
  isValveOpen = 0;
  delay(TIMEOUT);
  cutSupplyToValve();
}

/* bool onPowerState(String deviceId, bool &state)

   Callback for setPowerState request
   parameters
    String deviceId (r)
      contains deviceId (useful if this callback used by multiple devices)
    bool &state (r/w)
      contains the requested state (true:on / false:off)
      must return the new state

   return
    true if request should be marked as handled correctly / false if not
*/
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Device %s turned %s (via SinricPro) \r\n", deviceId.c_str(), state ? "on" : "off");
  state ? openValve() : closeValve();
  return true; // request handled properly
}

void handleButtonPress() {
  unsigned long actualMillis = millis(); // get actual millis() and keep it in variable actualMillis
  if (digitalRead(BUTTON_PIN) == HIGH && actualMillis - lastBtnPress > 1000)  { // is button pressed (inverted logic! button pressed = LOW) and debounced?
    if (isValveOpen) {     // flip myPowerState: if it was true, set it to false, vice versa
      closeValve();
    } else {
      openValve();
    }
    digitalWrite(VALVE_STATUS_PIN, isValveOpen ? LOW : HIGH); // if myPowerState indicates device turned on: turn on led (builtin led uses inverted logic: LOW = LED ON / HIGH = LED OFF)

    // get Switch device back
    SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];
    // send powerstate event
    mySwitch.sendPowerStateEvent(isValveOpen); // send the new powerState to SinricPro server
    Serial.printf("Device %s turned %s (manually via touchbutton)\r\n", mySwitch.getDeviceId().c_str(), isValveOpen ? "on" : "off");

    lastBtnPress = actualMillis;  // update last button press variable
  }
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

// setup function for SinricPro
void setupSinricPro() {
  // add device to SinricPro
  SinricProSwitch& mySwitch = SinricPro[SWITCH_ID];

  // set callback function to device
  mySwitch.onPowerState(onPowerState);

  // setup SinricPro
  SinricPro.onConnected([]() {
    Serial.printf("Connected to SinricPro\r\n");
  });
  SinricPro.onDisconnected([]() {
    Serial.printf("Disconnected from SinricPro\r\n");
  });
  //SinricPro.restoreDeviceStates(true); // Uncomment to restore the last known state from the server.
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  pinMode(BUTTON_PIN, INPUT); // GPIO 0 as input, pulled high
  pinMode(WIFI_PIN, OUTPUT); // define LED GPIO as output
  pinMode(CONTROL_PIN1, OUTPUT); // control pin
  pinMode(CONTROL_PIN2, OUTPUT); // control pin
  pinMode(VALVE_STATUS_PIN, OUTPUT); // valve status pin

  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  setupWiFi();
  setupSinricPro();
}

void loop() {
  handleButtonPress();
  SinricPro.handle();
}
