/*
 * I2C Master on an Arduino Nano. A5 is SCL, A4 is SDA.
 * 1k5 hardware pull-ups from SDA and SCL to VCC are required.
 * Also, make sure there is no connection between Pin 1
 * of any twoo MicroMoodies.
 *
 * Expects three micromoodies with i2c address 0x11 and moody
 * addresses 0x0001, 0x0002, 0x0003 on the bus.
 * Will do fast RGB fading on all three (with delays between the
 * individual moodies: 0001 first, then 0002, then 0003)
 */

#include <Wire.h>

int esleep = 300;
int lsleep = 300;

void setup()
{
  Wire.begin(); // join i2c bus as master
}

void i2csend(byte mode, byte spd, byte red, byte green, byte blue, byte addrhi, byte addrlo)
{
  Wire.beginTransmission(17);
  Wire.write(mode);
  Wire.write(spd);
  Wire.write(red);
  Wire.write(green);
  Wire.write(blue);
  Wire.write(addrhi);
  Wire.write(addrlo);
  Wire.endTransmission();
}

void propcolour(byte red, byte green, byte blue)
{
  i2csend(0, 0, red, green, blue, 0, 1);
  delay(esleep);
  i2csend(0, 12, red, green, blue, 0, 2);
  delay(esleep);
  i2csend(0, 12, red, green, blue, 0, 3);
  delay(lsleep);
}

void loop()
{
  propcolour(255,   0,   0);
  propcolour(255, 255,   0);
  propcolour(  0, 255,   0);
  propcolour(  0, 255, 255);
  propcolour(  0,   0, 255);
  propcolour(255,   0, 255);
}
