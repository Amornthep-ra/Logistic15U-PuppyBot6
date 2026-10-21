#pragma once
#include <Arduino.h>
#include <ICM_20948.h>
#include <math.h>

#ifndef ICM_20948_USE_DMP
#error "Select the Logistic15U-PuppyBot6 board to enable SparkFun DMP support."
#endif

class PuppyBotImuDevice {
public:
    bool begin(TwoWire &wire, uint8_t address) {
        ready_ = false;
        fresh_ = false;
        hasSample_ = false;
        if (address != 0x68 && address != 0x69) return false;
        if (!ok(sensor_.begin(wire, address == 0x69))) return false;
        if (!ok(sensor_.initializeDMP())) return false;
        if (!ok(sensor_.enableDMPSensor(INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR))) return false;
        if (!ok(sensor_.setDMPODRrate(DMP_ODR_Reg_Quat6, 0))) return false;
        if (!ok(sensor_.enableFIFO())) return false;
        if (!ok(sensor_.enableDMP())) return false;
        if (!ok(sensor_.resetDMP())) return false;
        if (!ok(sensor_.resetFIFO())) return false;
        ready_ = true;
        return true;
    }

    void update() {
        fresh_ = false;
        if (!ready_) return;
        icm_20948_DMP_data_t data{};
        status_ = sensor_.readDMPdataFromFIFO(&data);
        if (status_ != ICM_20948_Stat_Ok && status_ != ICM_20948_Stat_FIFOMoreDataAvail) return;
        if (!(data.header & DMP_header_bitmap_Quat6)) return;
        const double q1 = static_cast<double>(data.Quat6.Data.Q1) / 1073741824.0;
        const double q2 = static_cast<double>(data.Quat6.Data.Q2) / 1073741824.0;
        const double q3 = static_cast<double>(data.Quat6.Data.Q3) / 1073741824.0;
        const double vectorNorm = q1 * q1 + q2 * q2 + q3 * q3;
        if (!isfinite(vectorNorm) || vectorNorm > 1.00001) return;
        quaternion_[0] = sqrt(fmax(0.0, 1.0 - vectorNorm));
        quaternion_[1] = q1;
        quaternion_[2] = q2;
        quaternion_[3] = q3;
        sampleTimeMs_ = millis();
        hasSample_ = true;
        fresh_ = true;
    }

    bool ready() const { return ready_; }
    bool available() const { return fresh_; }
    bool hasSample() const { return hasSample_; }
    uint32_t sampleTimeMs() const { return sampleTimeMs_; }
    ICM_20948_Status_e status() const { return status_; }
    void clearAvailable() { fresh_ = false; }

    // Preserve the original PuppyBot wrapper's [w,x,y,z] index order.
    // Its x/y/z/w method names did not match the physical quaternion components.
    float x() const { return quaternion_[0]; }
    float y() const { return quaternion_[1]; }
    float z() const { return quaternion_[2]; }
    float w() const { return quaternion_[3]; }

private:
    bool ok(ICM_20948_Status_e result) {
        status_ = result;
        return result == ICM_20948_Stat_Ok;
    }
    ICM_20948_I2C sensor_;
    ICM_20948_Status_e status_ = ICM_20948_Stat_Ok;
    bool ready_ = false;
    bool fresh_ = false;
    bool hasSample_ = false;
    uint32_t sampleTimeMs_ = 0;
    float quaternion_[4] = {1, 0, 0, 0};
};
