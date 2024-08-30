#include <Wire.h>

void setup() {
  Wire.begin(0x46); // Start I2C Bus als Slave mit der Adresse 0x46
  Wire.setClock(64000);
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600); // Starten der seriellen Kommunikation mit einer Baudrate von 9600
}

void loop() {
  //delay(100);
}

// Funktion, die aufgerufen wird, wenn Daten empfangen werden
// Darf nicht zu viele Prints enthalten Siehe Issue #1 auf GH
void receiveEvent(int bytes) {
  while(Wire.available()) {
    uint8_t data = Wire.read(); // Empfangen von einem Byte als Zeichen
    //Serial.print("HEX: ");
    //Serial.println(data, HEX); // Ausgabe des Wertes in HEX
    //Serial.print("BIN: ");
    //Serial.println(data, BIN); // Ausgabe des Wertes in BIN
    //Serial.print("DEC: ");
    //Serial.println(data, DEC); // Ausgabe des Wertes in DEC
  }
}