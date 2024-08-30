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

// Globale Variablen
uint8_t targetVol = 0; // Wert zwischen 0 und 100
uint8_t targetBass = 0; // Wert zwischen 0 und 100
uint8_t currentVol = 0; // Wert zwischen 0 und 100
uint8_t currentBass = 0; // Wert zwischen 0 und 100
bool targetPower = false; // true=on und false=standby
bool currentPower = false; // true=on und false=standby

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
  Serial.print("MQTT_Standby: ");
  Serial.println(state);

  targetPower = state;

  sender->setState(state); // report state back to the Home Assistant
}

void onVolAllChange(HANumeric number, HANumber* sender)
{
  if (!number.isSet()) {
      // the reset command was send by Home Assistant
  } else {
    // you can do whatever you want with the number as follows:
    uint8_t value = number.toUInt8();
    Serial.print("MQTT_Vol: ");
    Serial.println(value);
    targetVol = value;
  }

  sender->setState(number); // report the selected option back to the HA panel
}

void onVolBassChange(HANumeric number, HANumber* sender)
{
  if (!number.isSet()) {
      // the reset command was send by Home Assistant
  } else {
    // you can do whatever you want with the number as follows:
    uint8_t value = number.toUInt8();
    Serial.print("MQTT_Bass: ");
    Serial.println(value);
    targetBass = value;
  }

  sender->setState(number); // report the selected option back to the HA panel
}

// ===================================== I2C ===================================

// ------------------------- helper --------------------

// Nimmt eine Binärzahl und wandelt sie in zwei BCD Ziffern um
uint8_t binToBCD(uint8_t bin)
{
  // Beispiel: 36
  uint8_t bcd2 = bin % 10;    // 36 => 6
  uint8_t bcd1 = bin - bcd2;  // 36 => 30
  bcd1 = bcd1 / 10;   // 30 => 3
  bcd1 = bcd1 << 4;   // 0000 0011 => 0011 0000
  return bcd1 | bcd2; // 0011 0110
}

// ------------------------------------------------------

void i2c_write_sep()
{
  Wire.beginTransmission(0x46);
  Wire.write(0xC0);
  Wire.endTransmission(true);
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
  Wire.endTransmission(true);
}

// Wird von sendUpdate() und turnOn() aufgerufen
// schreibt nur die Payload
// volReduction und bassReduction sind beides Binärzahlen
void i2c_write_volume(uint8_t volReduction, uint8_t bassReduction)
{
  uint8_t volBCD = binToBCD(volReduction);
  uint8_t bassBCD = binToBCD(bassReduction);
  
  i2c_write_data(0x44, 0xED, volBCD);  // Volume All
  i2c_write_data(0x46, 0x45, volBCD);  // Volume CH1
  i2c_write_sep();
  i2c_write_data(0x46, 0x89, bassBCD); // Volume CH2 - Sub
  i2c_write_sep();
  i2c_write_data(0x46, 0x01, volBCD);  // Volume CH3
  i2c_write_sep();
  i2c_write_data(0x46, 0x23, volBCD);  // Volume CH4
  i2c_write_sep();
  i2c_write_data(0x46, 0x67, volBCD);  // Volume CH5
  i2c_write_sep();
  i2c_write_data(0x46, 0xAB, volBCD);  // Volume CH6
}

// Wird von i2c_write_volume() mehrfach aufgerufen
// Schreibt 2 Bytes wobei bei jedem Byte die ersten 4 bit die Adresse des Register sind und die restlichen 4 die BCD Ziffer 
void i2c_write_data(uint8_t address, uint8_t reg, uint8_t bcdNumbers)
{
  uint8_t reg1 = reg & 0xf0;
  uint8_t reg2 = reg & 0x0f;

  uint8_t bcd1 = bcdNumbers & 0xf0;
  uint8_t bcd2 = bcdNumbers & 0x0f;

  uint8_t data1 = bcd1;
  data1 = data1 >> 4;
  data1 = data1 | reg1;

  uint8_t data2 = reg2;
  data2 = data2 << 4;
  data2 = data2 | bcd2;
  
  Wire.beginTransmission(address);
  Wire.write(data1);
  Wire.write(data2);
  Wire.endTransmission(true);
}

// ==================================== Actions =================================

void turnOn()
{
  digitalWrite(ST_PIN, LOW); // Aus Standby wecken

  delay(2491);

  i2c_write_power(true);
  i2c_write_sep();
  i2c_write_power(true);
  i2c_write_volume(76, 78); // Dämpfung manuell auf was sehr niedriges setzen
  
  sendUpdate(); // Tatsächliche Lautstärke(Dämpfung) senden

  delay(350);

  digitalWrite(MUTE_PIN, HIGH); // MUTE deaktivieren(HIGH)

  delay(3);

  sendUpdate();
}

void turnOff()
{
  i2c_write_power(false);
  digitalWrite(ST_PIN, HIGH);
  digitalWrite(MUTE_PIN, LOW);
}

void sendUpdate()
{
  i2c_write_sep();
  i2c_write_power(true);

  uint8_t volReduction = 0;
  uint8_t bassReduction = 0;

  //TODO: Reduktion nochmal Prüfen, hatte bei meinen captures vom Original immer auf min
  volReduction = map(currentVol, 0, 100, 58, 0);

  // Vorsicht uint, somit gibt 3 - 4 = Unfall
  if(volReduction <= 4)
  {
    bassReduction = 12;
  }
  else{
    bassReduction = volReduction + map(currentBass, 0, 100, 8, -4);
  }

  if(bassReduction < 12)
  {
    bassReduction = 12;
  }

  i2c_write_volume(volReduction, bassReduction);
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
  digitalWrite(ST_PIN, HIGH);
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

  if(targetPower != currentPower)
  {
    if (targetPower)
    {
      turnOn();
    }
    else 
    {
      turnOff();
    }
    currentPower = targetPower;

    delay(1000); // Anti Spam
  }

  if( (currentPower == true) && ((currentBass != targetBass) || (currentVol != targetVol)) )
  {
    currentBass=targetBass;
    currentVol=targetVol;
    sendUpdate();

    delay(500); // Anti Spam
  }

}