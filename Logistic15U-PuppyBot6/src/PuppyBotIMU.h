
#ifndef _PUPPYBOT_IMU_H
#define _PUPPYBOT_IMU_H

#include "PuppyBotImuDevice.h"
#include <Wire.h>
#include "PuppyBotEncoders.h"
#include "PuppyBotIMUStartup.h"

#ifndef I2C_ADDR
#define I2C_ADDR 0x69
#endif



namespace PuppyBotIMU {
    
    static PuppyBotImuDevice icm;
    static float continuousYaw = 0;
    static float offset_yaw = 0;
    static float read_Yaw = 0;
    static float newYaw = 0;
    static float pitch = 0;
    static float roll = 0;
    static float Ref_offset_Con = 0;

    void resetBoard() {
        Serial.println("Rebooting...");
        //while (1);
    }
    bool initIMU(bool enabled = true) {
        if (!startup.start(enabled)) return false;
        Wire1.setSDA(6);
        Wire1.setSCL(7);
        Wire1.begin();
        Wire1.setClock(400000);
        const bool initialized = icm.begin(Wire1, I2C_ADDR);
        Serial.println(initialized ? "ICM-20948 DMP ready" : "ICM-20948 initialization failed");
        return startup.initialized(initialized, int(icm.status()));
    }
    bool isIMUReady() { return icm.ready(); }
    bool hasIMUSample() { return icm.hasSample(); }
    uint32_t getIMUSampleTimeMs() { return icm.sampleTimeMs(); }
    ICM_20948_Status_e getIMUStatus() { return icm.status(); }
    bool updateIMU_fine_offset_Yaw(int round) {
      if (!startup.calibrate(round > 0 ? unsigned(round) : 0)) return false;
      unsigned validSamples = 0;
      uint32_t lastSampleAt = 0;
      for (int Rread = 0; Rread < round; Rread++) {
        if (getIMUStartupState() != StartupState::Calibrating) return false;
        icm.update();
        if (icm.available()) {
          float qx = icm.x();
          float qy = icm.y();
          float qz = icm.z();
          float qw = icm.w();
          const float yaw = atan2(2.0 * (qw * qx + qy * qz), 1.0 - 2.0 * (qx * qx + qy * qy)) * 180.0 / PI;
          if (isfinite(yaw)) {
            read_Yaw = yaw < 0 ? yaw + 360 : yaw;
            lastSampleAt = millis();
            startup.samples.store(++validSamples);
          }
          icm.clearAvailable();
        }
        delay(10);
        startup.completed.store(unsigned(Rread + 1));
      }
      // This verifies recent data, not physical stillness or gyro bias accuracy.
      const bool freshData = validSamples > 0 && uint32_t(millis() - lastSampleAt) <= 250;
      if (freshData) offset_yaw = read_Yaw;
      return startup.finish(freshData);
    }

    void updateIMU() {
        icm.update();
        if (!icm.available()) return;
        float qx = icm.x();
        float qy = icm.y();
        float qz = icm.z();
        float qw = icm.w();
        newYaw = atan2(2.0 * (qw * qx + qy * qz), 1.0 - 2.0 * (qx * qx + qy * qy)) * 180.0 / PI;
        
        float sinp = 2 * (qw * qy - qz * qx);
        pitch = (fabs(sinp) >= 1) ? copysign(M_PI / 2, sinp) : asin(sinp);
        pitch *= 180.0 / M_PI;

        float siny_cosp = 2 * (qw * qz + qx * qy);
        float cosy_cosp = 1 - 2 * (qy * qy + qz * qz);
        roll = atan2(siny_cosp, cosy_cosp) * 180.0 / M_PI;

        // Legacy behavior: swap pitch/roll to match older board orientation.
        // If this is wrong for your setup, remove the two lines below.
        float __tmp = roll;
        roll = pitch;
        pitch = __tmp;

        // Make "nose up" positive on the swapped pitch axis.
        // If you want the opposite sign, remove or negate this line.
        pitch = -pitch;

        // Make "left tilt" positive on the swapped roll axis.
        // If you want the opposite sign, remove or negate this line.
        roll = -roll;
    }

    float getYaw() {
        updateIMU();
        if (isnan(newYaw)) return read_Yaw;
        if (newYaw < 0) newYaw += 360;
        if (newYaw > 360) newYaw -= 360;
        read_Yaw = newYaw;
        return read_Yaw;
    }
    void updateContinuousYaw() {
    static float lastYaw = 0;
    static bool firstRun = true;

    updateIMU(); // อ่านค่าจาก IMU

    float currentYaw = getYaw(); // อ่านค่า getYaw() แค่ครั้งเดียว

    if (firstRun) {
      continuousYaw = currentYaw;
      lastYaw = currentYaw;
      firstRun = false;
      return;
    }

    // คำนวณ deltaYaw และ normalize ให้เป็นช่วง -180 ถึง 180 องศา
    float deltaYaw = currentYaw - lastYaw;
    if (deltaYaw > 180.0) deltaYaw -= 360.0;
    if (deltaYaw < -180.0) deltaYaw += 360.0;

    // อัปเดต continuousYaw
    continuousYaw += deltaYaw;
    
    // อัปเดต lastYaw
    lastYaw = currentYaw;
  }


    float getPitch() { updateIMU(); return pitch; }
    float getRoll() { updateIMU(); return roll; }
    // Cached getters: assume updateIMU() already called in this loop.
    float getPitchCached() { return pitch; }
    float getRollCached() { return roll; }
    float getYawCached() {
        if (isnan(newYaw)) return read_Yaw;
        float yaw = newYaw;
        if (yaw < 0) yaw += 360;
        if (yaw > 360) yaw -= 360;
        read_Yaw = yaw;
        return yaw;
    }
    void updateContinuousYawCached() {
      static float lastYaw = 0;
      static bool firstRun = true;

      float currentYaw = getYawCached();
      if (firstRun) {
        continuousYaw = currentYaw;
        lastYaw = currentYaw;
        firstRun = false;
        return;
      }

      float deltaYaw = currentYaw - lastYaw;
      if (deltaYaw > 180.0) deltaYaw -= 360.0;
      if (deltaYaw < -180.0) deltaYaw += 360.0;

      continuousYaw += deltaYaw;
      lastYaw = currentYaw;
    }
    float getOffsetYaw() { return offset_yaw; }
    float getContinuousYaw() { updateContinuousYaw(); return continuousYaw + Ref_offset_Con ; }
    
    void resetGyro() {float y = getContinuousYaw();   
        Ref_offset_Con = 180.0f - y;
        continuousYaw = y;  }
    void resetContinuousYaw() { continuousYaw = getYaw(); }
}

using namespace PuppyBotIMU;  // ทำให้เรียกใช้งานง่ายขึ้นโดยไม่ต้องพิมพ์ PuppyBotIMU::

#endif  // _PUPPYBOT_IMU_H


