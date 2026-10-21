#include <Arduino.h>
// #include <KB_music.h>
#include <puppybot.h>

#ifndef I2C_ADDR
#define I2C_ADDR 0x69
#endif

#include "PuppyBotIMU.h"
#include "PuppyBotTurnPID.h"
#include "KB_PIDFollowHelpers.h"

typedef float Number;
typedef int Boolean;

void setup() {
  delay(1000);
  puppybot_setup();
  Serial.begin(115200);
}

void loop() {
}
void setup1() {
  delay(1000);
  initIMU();
  set_TCS_back();
  updateIMU_fine_offset_Yaw(300);
  offset_yaw_loop1 = getOffsetYaw();
}
void loop1() {
  // Single IMU update per loop1 to reduce repeated reads.
  // If this causes issues, revert by using
  // getPitch/getRoll/getYaw/getContinuousYaw directly.
  updateIMU();
  pitch_loop1 = getPitchCached();
  roll_loop1 = getRollCached();
  yaw_loop1 = getYawCached();
  updateContinuousYawCached();
  Con_yaw_loop1 = continuousYaw + Ref_offset_Con;
  if (status_resetYaw == 1) {
    status_resetYaw = 0;
    resetContinuousYaw();
  }
  if (status_resetYaw180 == 1) {
    resetGyro();
    status_resetYaw180 = 0;
  }
  TCS_R = Read_Color_TCS_B_Raw(0);
  TCS_G = Read_Color_TCS_B_Raw(1);
  TCS_B = Read_Color_TCS_B_Raw(2);
  TCS_T = Read_Color_TCS_B_Raw(3);
  TCS_L = Read_Color_TCS_B_Raw(4);
}
