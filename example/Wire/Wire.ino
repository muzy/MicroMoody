/*
 * expects three micromoodies with i2c address 0x11 and moody
 * address 0x0001, 0x0002, 0x0003 on the bus.
 * Will do RGB fading on all three (with delays between the
 * individual moodies
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

void loop()
{
  i2csend(4, 4, 255, 0, 0, 0, 1);
  delay(esleep);
  i2csend(4, 4, 255, 0, 0, 0, 2);
  delay(esleep);
  i2csend(4, 4, 255, 0, 0, 0, 3);
  delay(lsleep);
  i2csend(4, 4, 255, 255, 0, 0, 1);
  delay(esleep);
  i2csend(4, 4, 255, 255, 0, 0, 2);
  delay(esleep);
  i2csend(4, 4, 255, 255, 0, 0, 3);
  delay(lsleep);
  i2csend(4, 4, 0, 255, 0, 0, 1);
  delay(esleep);
  i2csend(4, 4, 0, 255, 0, 0, 2);
  delay(esleep);
  i2csend(4, 4, 0, 255, 0, 0, 3);
  delay(lsleep);
  i2csend(4, 4, 0, 255, 255, 0, 1);
  delay(esleep);
  i2csend(4, 4, 0, 255, 255, 0, 2);
  delay(esleep);
  i2csend(4, 4, 0, 255, 255, 0, 3);
  delay(lsleep);
  i2csend(4, 4, 0, 0, 255, 0, 1);
  delay(esleep);
  i2csend(4, 4, 0, 0, 255, 0, 2);
  delay(esleep);
  i2csend(4, 4, 0, 0, 255, 0, 3);
  delay(lsleep);
  i2csend(4, 4, 255, 0, 255, 0, 1);
  delay(esleep);
  i2csend(4, 4, 255, 0, 255, 0, 2);
  delay(esleep);
  i2csend(4, 4, 255, 0, 255, 0, 3);
  delay(lsleep);
}
