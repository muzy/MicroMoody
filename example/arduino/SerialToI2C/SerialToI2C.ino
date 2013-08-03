/*
 * I2C Master on an Arduino Nano. A5 is SCL, A4 is SDA.
 * 1k5 hardware pull-ups from SDA and SCL to VCC are required.
 * Also, make sure there is no connection between Pin 1
 * of any twoo MicroMoodies.
 *
 * Passes any serial data on to the micromoody bus.
 * Example input: "17,0,0,255,255,0,255,255" will set
 * all moodies to yellow.
 */

#include <Wire.h>

void setup()
{
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{
  byte data[8], i, ret;
  while (Serial.available()) {
    for (i = 0; i < 8; i++) {
      data[i] = (byte)Serial.parseInt();
    }
      
    Serial.print("sending:");
    Wire.beginTransmission(data[0]);
    for (i = 1; i < 8; i++) {
      Serial.print(" ");
      Serial.print(data[i], HEX);
      Wire.write(data[i]);
    }
    ret = Wire.endTransmission();
    switch (ret) {
      case 0: Serial.println(" -> OK"); break;
      case 1: Serial.println(" -> ERR: Data too long"); break;
      case 2: Serial.println(" -> ERR: address NAKd"); break;
      case 3: Serial.println(" -> OK/ERR: data NAKd"); break;
      case 4: Serial.println(" -> ERR: unknown error"); break;
    }
  }
}
