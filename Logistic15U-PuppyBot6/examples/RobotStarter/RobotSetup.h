#pragma once
#include <Logistic15U_PuppyBot6.h>

namespace RobotSetup {
    // ค่าด้านล่างวัดจากหุ่นต้นแบบตัวเดียว หุ่นตัวอื่นต้องวัดใหม่ด้วย
    // File -> Examples -> Logistic15U-PuppyBot6 -> WheelSpeedMeasure แล้วกรอก encoderSign / ticks/s ของตัวเอง
    inline WheelSpeedSettings wheelSpeedSettings() {
        WheelSpeedSettings settings;
        // ยืนยันจากผู้ใช้: Encoder 1 คู่ M1 และ Encoder 2 คู่ M2
        settings.motor1.encoder = 1;
        settings.motor2.encoder = 2;
        // ผู้ใช้วัดที่ PWM +30%, แบตเตอรี่ 9.98–10 V; มอเตอร์บวกทั้งคู่เดินหน้า
        settings.motor1.encoderSign = -1;
        settings.motor2.encoderSign = 1;
        // อัตราพัลส์ที่ผู้ใช้วัดจริง หน่วย ticks/s; ไม่ใช่พัลส์ต่อรอบล้อ
        settings.motor1.referencePwm = 30;
        settings.motor2.referencePwm = 30;
        settings.motor1.referenceTicksPerSecond = 4667.0f;
        settings.motor2.referenceTicksPerSecond = 4667.0f;
        // PI ภายในเป็นค่าเริ่มต้นทดลองของ Lib; ต้องจูนกับหุ่นจริง
        return settings;
    }

    inline bool beginWheelSpeedControl() {
        const bool ready = ::beginWheelSpeedControl(wheelSpeedSettings());
        Serial.println(ready ? "Encoder wheel PI ready" : "Encoder wheel PI not configured: line PD only");
        return ready;
    }

    // เรียกใน setup() บน core 0 ก่อน wait_SW1()
    inline void beginEncoders() {
        PuppyBotIMU::init_encoders();
    }

    // เรียกหลังเลือก Start; การรีเซ็ตแยกตัวยังใช้ resetEncoder(1/2) ได้
    inline void resetEncoders() {
        PuppyBotIMU::resetEncoder(1);
        PuppyBotIMU::resetEncoder(2);
    }
}
