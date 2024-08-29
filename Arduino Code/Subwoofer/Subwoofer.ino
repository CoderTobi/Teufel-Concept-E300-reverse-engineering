#include <WiFi.h>
#include <ArduinoHA.h>
#include <Wire.h>
#include "secrets.hpp"

// Pin Config
const uint8_t CD_PIN = 23; // Zweck unbekannt - aktuell unused
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

// ===================== Call Backs ========================

void onSwitchCommand(bool state, HASwitch* sender)
{
  Serial.print("Standby is now: ");
  Serial.println(state);

  i2c_write_sep();
  i2c_write_power(state);

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

// ===================================== I2C ===================================

void i2c_write_sep()
{
  Wire.beginTransmission(0x46);
  Wire.write(0xC0);
  Wire.endTransmission();
}

void i2c_write_power(bool on)
{
  Wire.beginTransmission(0x46);
  if(on)
  {
    Wire.write(0xF8);
  }
  else
  {
    Wire.write(0xF9);
  }
  Wire.endTransmission();
}

void i2c_write_all(uint8_t value)
{

}

void i2c_write_ch(uint8_t value, uint8_t reg)
{

}

// ===================================== Setup ===================================

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

  // PIN Mode Setup
  pinMode(ST_PIN, OUTPUT);
  pinMode(MUTE_PIN, OUTPUT);
  //pinMode(CD_PIN, OUTPUT);
  digitalWrite(ST_PIN, LOW);
  digitalWrite(MUTE_PIN, LOW);

  // I2C Setup
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(64000); // Original arbeitet bei 64kHz - unsicher ob ESP kann langsamer als 100kHz

  // Wifi verbinden
  WiFi.begin(WIFI_SSID, WIFI_PW);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(500); // waiting for the connection
  }
  Serial.println("connected"); 

  // Device Types Setup
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

// ===================================== Loop ===================================

void loop() {
  mqtt.loop();

  // your loop logic goes here
}