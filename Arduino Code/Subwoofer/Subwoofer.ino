#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoHA.h>
#include <Wire.h>
#include "secrets.hpp"

// Pin Config - ESP-WROOM-32 Dev Kit
const uint8_t CD_PIN = 17;
const uint8_t SDA_PIN = 21;
const uint8_t SCL_PIN = 22;
const uint8_t MUTE_PIN = 19;
const uint8_t ST_PIN = 18;

// General Config
const int playingChangedDelay = 2000; // Wie lange(ms) das Signal auf CD gleich bleiben muss, bis es als ein neuer Status erkannt wird

// Globale Variablen
uint8_t targetVol = 0; // Wert zwischen 0 und 100
uint8_t targetBass = 0; // Wert zwischen 0 und 100
uint8_t currentVol = 0; // Wert zwischen 0 und 100
uint8_t currentBass = 0; // Wert zwischen 0 und 100
bool targetPower = false; // true=on und false=standby
bool currentPower = false; // true=on und false=standby
bool playing = false;
unsigned long playingChanged = 0;

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

// Meine Devices
// device types need to be constructed after HAMqtt class.
HANumber numberAll("VolAll");
HANumber numberBass("VolBass");
HASwitch switchPower("Power");
HABinarySensor sensorPlaying("Playing");

// ===================================== Other =================================

// Funktioniert nicht, da er nicht extra den Server fragt, sondern den lokalen cache verwendet
/*
void fetchVolFromHA()
{
  HANumeric all = numberAll.getCurrentState();
  targetVol = all.toUInt8();
  Serial.print("MQTT_fetchVol: ");
  Serial.println(targetVol);
  HANumeric bass = numberBass.getCurrentState();
  targetVol = bass.toUInt8();
  Serial.print("MQTT_fetchBass: ");
  Serial.println(targetBass);
}
*/

// Delay Ersatz, der trotzdem MQTT pflegt
void myDelay(int ms)
{
  unsigned long startMillis = millis();
  while((millis() - startMillis) < ms)
  {
    mqtt.loop();
    checkForAudio();
  }
}

void checkForAudio()
{
  if( playing != digitalRead(CD_PIN))
  {
    playingChanged = millis();
  }

  if ((millis() - playingChanged) > playingChangedDelay)
  {
    playingChanged = millis();
    playing = !playing;
    sensorPlaying.setState(playing);
  }
}

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

// -------------------- OTA -------------------
void onOTAStart()
{
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
  } else {  // U_SPIFFS
    type = "filesystem";
  }

  // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  Serial.println("Start updating " + type);
}


void onOTAEnd() 
{
  Serial.println("\nOTA Done");
}

void onOTAProgress(unsigned int progress, unsigned int total) 
{
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void onOTAError(ota_error_t error) 
{
  Serial.printf("Error[%u]: ", error);
  if (error == OTA_AUTH_ERROR) {
    Serial.println("Auth Failed");
  } else if (error == OTA_BEGIN_ERROR) {
    Serial.println("Begin Failed");
  } else if (error == OTA_CONNECT_ERROR) {
    Serial.println("Connect Failed");
  } else if (error == OTA_RECEIVE_ERROR) {
    Serial.println("Receive Failed");
  } else if (error == OTA_END_ERROR) {
    Serial.println("End Failed");
  }
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
  Serial.print("DEBUG volred: ");
  Serial.print(volReduction);
  Serial.print(" bassred: ");
  Serial.println(bassReduction);

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
  // Aus Standby wecken
  // ST muss GND gezogen werden, damit das Relais schaltet
  // Daher Pin auf high, damit der NPN Transistor durchschält und ST auf LOW zieht
  // Wichtig! Transistor nicht vergessen
  digitalWrite(ST_PIN, HIGH);

  myDelay(2491);

  i2c_write_power(true);
  i2c_write_sep();
  i2c_write_power(true);
  i2c_write_volume(76, 78); // Dämpfung manuell auf was sehr niedriges setzen
  
  sendUpdate(); // Tatsächliche Lautstärke(Dämpfung) senden

  myDelay(350);

  digitalWrite(MUTE_PIN, HIGH); // MUTE deaktivieren(HIGH)

  myDelay(3);

  sendUpdate();
}

void turnOff()
{
  i2c_write_power(false);
  digitalWrite(ST_PIN, LOW); // LOW -> NPN zieht nicht mehr auf GND -> Pull Up zieht auf 5V
  digitalWrite(MUTE_PIN, LOW);
}

void sendUpdate()
{
  currentBass=targetBass;
  currentVol=targetVol;

  i2c_write_sep();
  i2c_write_power(true);

  uint8_t volReduction = 0;
  uint8_t bassReduction = 0;
  int bassReductionInt = 0;

  volReduction = map(currentVol, 0, 100, 58, 0);

  bassReductionInt = volReduction + map(currentBass, 0, 100, 8, -4);
  if(bassReductionInt < 0)
  {
    bassReduction = 0;
  }
  else{
    bassReduction = (uint8_t) bassReductionInt;
  }

  // Vorsicht uint, somit gibt negative = Unfall
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
  Serial.begin(115200);
  Serial.print("Wifi"); 

  // PIN Mode Setup
  pinMode(ST_PIN, OUTPUT); 
  pinMode(MUTE_PIN, OUTPUT);
  pinMode(CD_PIN, INPUT);
  digitalWrite(ST_PIN, LOW); // LOW -> NPN sperrt -> Pull Up zieht auf 5V -> Relais schält nicht
  digitalWrite(MUTE_PIN, LOW);

  // I2C Setup
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(50000); // Original arbeitet bei 64kHz
  //Wire.setTimeout(0); // NUR ZUM TESTEN

  // Wifi verbinden
  WiFi.begin(WIFI_SSID, WIFI_PW);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    myDelay(500); // waiting for the connection
  }
  Serial.println("connected");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP()); <- IDK why aber die Zeile bricked alles...

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
  sensorPlaying.setName("Playing");
  sensorPlaying.setIcon("mdi:play-circle");
  sensorPlaying.setDeviceClass("sound");

  // LWT aktivieren
  device.enableSharedAvailability();
  device.enableLastWill();

  // MDNS
  MDNS.begin("Subwoofer");

  // OTA
  ArduinoOTA.setHostname("Subwoofer");
  ArduinoOTA.setPassword(OTA_PW);
  ArduinoOTA.onStart(onOTAStart);
  ArduinoOTA.onEnd(onOTAEnd);
  ArduinoOTA.onProgress(onOTAProgress);
  ArduinoOTA.onError(onOTAError);
  ArduinoOTA.begin();

  // CD Check Setup
  playingChanged = millis();

  // MQTT broker connection (muss ans Ende von Setup)
  mqtt.begin(MQTT_HOST, MQTT_USER, MQTT_PW);
}

// ===================================== Loop ===================================

void loop() {
  mqtt.loop();
  checkForAudio();

  if(targetPower != currentPower)
  {

    currentPower = targetPower;

    if (currentPower)
    {
      //fetchVolFromHA();
      turnOn();
    }
    else 
    {
      turnOff();
    }

    myDelay(1000); // Anti Spam
  }

  if( (currentPower == true) && ((currentBass != targetBass) || (currentVol != targetVol)) )
  {
    sendUpdate();

    myDelay(100); // Anti Spam
  }

  // Wenn wir im Standby sind
  if(currentPower == false)
  {
    // OTA Updates durchführen
    ArduinoOTA.handle();
  }
}