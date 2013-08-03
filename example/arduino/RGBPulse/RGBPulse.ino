/*
 * I2C Master on an Arduino Nano. A5 is SCL, A4 is SDA.
 * 1k5 hardware pull-ups from SDA and SCL to VCC are required.
 * Also, make sure there is no connection between Pin 1
 * of any twoo MicroMoodies.
 *
 * Makes all MicroMoodies on the bus pulse in RGB colors
 * (that is, color -> off -> next color -> off -> next color -> ...)
 *
 * Does NOT save the animation to EEPROM
 */

#include <Wire.h>

byte default_delay = 16;

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

void add_anim_step(byte transition_delay, byte red, byte green, byte blue)
{
  static int anim_cnt = 0;
  i2csend(64 + anim_cnt, transition_delay, red, green, blue, 255, 255);
  anim_cnt++;
}

void start_anim()
{
  i2csend(63, 0, 0, 0, 0, 255, 255);

  // uncomment the following to save the animation to EEPROM
  // i2csend(240, 0, 0, 0, 0, 255, 255);
}

void setup()
{
  Wire.begin(); // join i2c bus as master
  add_anim_step(default_delay, 255,   0,   0);
  add_anim_step(default_delay,   0,   0,   0);
  add_anim_step(default_delay, 255, 255,   0);
  add_anim_step(default_delay,   0,   0,   0);
  add_anim_step(default_delay,   0, 255,   0);
  add_anim_step(default_delay,   0,   0,   0);
  add_anim_step(default_delay,   0, 255, 255);
  add_anim_step(default_delay,   0,   0,   0);
  add_anim_step(default_delay,   0,   0, 255);
  add_anim_step(default_delay,   0,   0,   0);
  add_anim_step(default_delay, 255,   0, 255);
  add_anim_step(default_delay,   0,   0,   0);
  
  start_anim();
}

void loop()
{
}
