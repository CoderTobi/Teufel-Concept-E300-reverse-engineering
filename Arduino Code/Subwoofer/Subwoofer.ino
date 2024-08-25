#include <WiFi.h>
#include <ArduinoHA.h>
#include "secrets.hpp"

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

// device types need to be constructed after HAMqtt class.
HASwitch switchTest("TestSwitch");

void onSwitchCommand(bool state, HASwitch* sender)
{
  Serial.print("State is now: ");
  Serial.println(state);
  if(state == true)
  {
    digitalWrite(22, HIGH); // LED ist invertiert LOW => LED an
  }
  else
  {
    digitalWrite(22, LOW);
  }
  sender->setState(state); // report state back to the Home Assistant
}

void setup() 
{
  byte mac[6];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));
  device.setName("Teufel-Soundsystem");

  Serial.begin(9600);
  Serial.print("Wifi"); 

  // you can skip this part if you're already maintaining the connection logic
  WiFi.begin(WIFI_SSID, WIFI_PW);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(500); // waiting for the connection
  }
  Serial.println("connected"); 

  // your setup logic goes here
  pinMode(22, OUTPUT);
  switchTest.onCommand(onSwitchCommand);

  // MQTT broker connection (use your data here)
  mqtt.begin(MQTT_HOST, MQTT_USER, MQTT_PW);
}

void loop() {
  mqtt.loop();

  // your loop logic goes here
}