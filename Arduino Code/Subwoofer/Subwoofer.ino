#include <WiFi.h>
#include <ArduinoHA.h>
#include "secrets.hpp"

// Config
const uint8_t LED_PIN = 22;
const uint8_t CD_PIN = 23;
const uint8_t SDA_PIN = 15;
const uint8_t SCL_PIN = 2;
const uint8_t MUTE_PIN = 19;
const uint8_t ST_PIN = 22;

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

// Meine Devices
// device types need to be constructed after HAMqtt class.
HANumber numberAll("VolAll");
HANumber numberBass("VolBass");
HASwitch switchPower("Power");

void onSwitchCommand(bool state, HASwitch* sender)
{
  Serial.print("Standby is now: ");
  Serial.println(state);

  if(state == true)
  {
    digitalWrite(LED_PIN, LOW); // LED ist invertiert LOW => LED an
  }
  else
  {
    digitalWrite(LED_PIN, HIGH);
  }
  sender->setState(state); // report state back to the Home Assistant
}

void onVolAllChange(HANumeric number, HANumber* sender)
{
  if (!number.isSet()) {
      // the reset command was send by Home Assistant
  } else {
    // you can do whatever you want with the number as follows:
    int16_t value = number.toInt16();
    Serial.print("Vol set to: ");
    Serial.println(value);

  }

  sender->setState(number); // report the selected option back to the HA panel
}

void onVolBassChange(HANumeric number, HANumber* sender)
{
  if (!number.isSet()) {
      // the reset command was send by Home Assistant
  } else {
    // you can do whatever you want with the number as follows:
    int16_t value = number.toInt16();
    Serial.print("Bass set to: ");
    Serial.println(value);

  }

  sender->setState(number); // report the selected option back to the HA panel
}

void setup() 
{
  // Device Setup
  byte mac[6];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));
  device.setName("Teufel-Soundsystem");

  // Serial Setup
  Serial.begin(9600);
  Serial.print("Wifi"); 

  // Wifi verbinden
  WiFi.begin(WIFI_SSID, WIFI_PW);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(500); // waiting for the connection
  }
  Serial.println("connected"); 

  // your setup logic goes here
  pinMode(LED_PIN, OUTPUT);
  switchPower.onCommand(onSwitchCommand);
  switchPower.setIcon("mdi:power-standby");
  switchPower.setName("Power");
  numberAll.setName("Volume Allgemein");
  numberAll.setIcon("mdi:speaker-multiple");
  numberAll.onCommand(onVolAllChange);
  numberBass.setName("Volume Bass");
  numberBass.setIcon("mdi:volume-vibrate");
  numberBass.onCommand(onVolBassChange);

  // MQTT broker connection (muss ans Ende von Setup)
  mqtt.begin(MQTT_HOST, MQTT_USER, MQTT_PW);
}

void loop() {
  mqtt.loop();

  // your loop logic goes here
}