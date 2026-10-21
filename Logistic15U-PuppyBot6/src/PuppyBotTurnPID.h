#include "PuppyBotIMU.h"
enum Direction_turnPID { NORTH = 0,
                         EAST = 1,
                         SOUTH = 2,
                         WEST = 3 };

int Direction_turnPIDIndex = NORTH;
float previousYaw_turnPID = 0;

float preverror_turnPID = 0;
float integral_turn = 0;
float turnDirection_PID_KP = 2, turnDirection_PID_KD = 0;
int speedMin_turnDirection = 15, speedMax_turnDirection = 60;
int error_for_turnPID = 1;
int state_begin_Encoder = 0;



void set_data_for_turnDirection(int error_PID, int speedMin, int speedMax, float KP, float KD) {
  turnDirection_PID_KP = KP;
  turnDirection_PID_KD = KD;
  speedMin_turnDirection = speedMin;
  speedMax_turnDirection = speedMax;
  error_for_turnPID = error_PID;
}

float shortestAngle(float currentAngle, float targetAngle) {
  float error = fmod((targetAngle - currentAngle + 540), 360) - 180;
  return error;
}

void turnPID(int condition, float targetYaw_turnPID, int speedTurn_min, int speedTurn_max, float kp_turnPID, float kd_turnPID) {
  unsigned long startTime = millis();
  integral_turn = 0;
  int stableCount = 0;
  int overshootCount = 0;
  bool hasOvershoot = false;

  float preverror_turnPID = shortestAngle(Con_yaw_loop1, targetYaw_turnPID);
  unsigned long now2 = millis();
  while (true) {
    float current_Yaw = Con_yaw_loop1;

    float error_turnPID = shortestAngle(current_Yaw, targetYaw_turnPID);

    // if ((preverror_turnPID > 0 && error_turnPID < 0) || (preverror_turnPID < 0 && error_turnPID > 0)) {
    //   hasOvershoot = true;
    //   overshootCount++;
    // }

    unsigned long now = millis();
    float deltaTime = max((now - startTime) / 1000.0, 0.001);
    startTime = now;

    integral_turn += error_turnPID * deltaTime;
    integral_turn = constrain(integral_turn, -50, 50);
    float derivative = (error_turnPID - preverror_turnPID) / deltaTime;
    float output = kp_turnPID * error_turnPID + kd_turnPID * derivative;

    int speed_L = constrain(abs(output), speedTurn_min, speedTurn_max);
    int speed_R = constrain(abs(output), speedTurn_min, speedTurn_max);

    if (output < 0) {
      // เลี้ยวซ้าย
      if (condition == 1) {
        // 2 ล้อ (ซ้ายหยุด ขวาหมุนหน้า)
        motor(1, 0);
        motor(2, speed_R);
        if (Set_Mode_Gyro == 0) {
          motor(3, 0);
          motor(4, speed_R);
        }
      } else if (condition == 0) {
        // 4 ล้อ (เลี้ยวรถถัง)
        motor(1, -speed_L);
        motor(2, speed_R);
        if (Set_Mode_Gyro == 0) {
          motor(3, -speed_L);
          motor(4, speed_R);
        }
      }
      if (condition == 2) {
        // 2 ล้อ (ซ้ายหยุด ขวาหมุนหน้า)
        motor(1, -speed_L);
        motor(2, 0);
        if (Set_Mode_Gyro == 0) {
          motor(3, -speed_L);
          motor(4, 0);
        }
      }
    } else {
      // เลี้ยวขวา
      if (condition == 1) {
        // 2 ล้อ (ขวาหยุด ซ้ายหมุนหน้า)
        motor(1, speed_L);
        motor(2, 0);
        if (Set_Mode_Gyro == 0) {
          motor(3, speed_L);
          motor(4, 0);
        }
      } else if (condition == 0) {
        // 4 ล้อ (เลี้ยวรถถัง)
        motor(1, speed_L);
        motor(2, -speed_R);
        if (Set_Mode_Gyro == 0) {
          motor(3, speed_L);
          motor(4, -speed_R);
        }
      }
      if (condition == 2) {
        // 2 ล้อ (ขวาหยุด ซ้ายหมุนหน้า)
        motor(1, 0);
        motor(2, -speed_R);
        if (Set_Mode_Gyro == 0) {
          motor(3, 0);
          motor(4,-speed_R);
        }
      }
    }

    // if (overshootCount > 5) {
    //   ao();
    //   break;
    // }

    if (abs(error_turnPID) < error_for_turnPID /*&& hasOvershoot && overshootCount >= 2*/) {
      // stableCount++;
      // if (stableCount > 5) {
        ao();
        break;
      // }
    } else {
      stableCount = 0;
    }

    if (millis() - now2 > 2500) {
      Serial.println("Timeout! Exiting loop...");
      ao();
      break;
    }

    preverror_turnPID = error_turnPID;
  }
}


void turnByAngle(int turnAngle,int  condition) {
  float currentYaw = Con_yaw_loop1;
  float targetYaw = currentYaw + turnAngle;
  targetYaw = fmod(targetYaw, 360);
  turnPID(condition,targetYaw, speedMin_turnDirection, speedMax_turnDirection, turnDirection_PID_KP, turnDirection_PID_KD);
  status_resetYaw = 1;
}


float normalizeAngle(float angle) {
  while (angle > 180) angle -= 360;
  while (angle <= -180) angle += 360;
  return angle;
}
void moveStraightPID_Accel(
  int   Movedirection,
  float targetYaw_straight,
  int   speedBase,          // 10..100 ตาม constrain เดิม
  float duration,           // ระยะเวลารวม (ms)
  float kp_straight,
  float ki_straight,
  float kd_straight,
  unsigned long accel_ms,   // เวลาเร่งขึ้น
  unsigned long decel_ms    // เวลาชะลอลง
) {
    unsigned long t0 = millis();
    unsigned long endTime = t0 + (unsigned long)duration;

    float integral_straight = 0.0f;
    float prev_error = 0.0f;

    // ใช้ตัวแยกสำหรับคำนวณ deltaTime เพื่อไม่ให้ชนกับตัวจับเวลาอื่น
    unsigned long lastUpdate = millis();

    while (millis() < endTime) {
        // ---------- เวลาปัจจุบัน / deltaTime ----------
        unsigned long now = millis();
        float deltaTime = max((now - lastUpdate) / 1000.0f, 0.001f);
        lastUpdate = now;

        // ---------- อ่านมุม/คำนวณ PID ----------
        float current_Yaw = Con_yaw_loop1;
        float error = normalizeAngle(targetYaw_straight - current_Yaw);

        integral_straight += error * deltaTime;
        integral_straight = constrain(integral_straight, -50, 50); // anti-windup

        float derivative = (error - prev_error) / deltaTime;
        float output = kp_straight * error + ki_straight * integral_straight + kd_straight * derivative;
        prev_error = error;

        // ---------- คำนวณโปรไฟล์ความเร็ว (ramp-up + ramp-down) ----------
        unsigned long elapsed = now - t0;
        unsigned long remain  = (endTime > now) ? (endTime - now) : 0;

        // 1) เริ่มจากเพดานที่ speedBase
        float speedTarget = speedBase;

        // 2) บังคับ ramp-up: linear จาก 0 → speedBase ใน accel_ms
        if (accel_ms > 0) {
            float rampUp = (float)elapsed / (float)accel_ms;
            rampUp = constrain(rampUp, 0.0f, 1.0f);
            speedTarget = min(speedTarget, speedBase * rampUp);
        }

        // 3) บังคับ ramp-down: linear จาก speedBase → 0 ใน decel_ms สุดท้าย
        if (decel_ms > 0 && remain <= decel_ms) {
            float rampDown = (float)remain / (float)decel_ms; // 1→0
            rampDown = constrain(rampDown, 0.0f, 1.0f);
            speedTarget = min(speedTarget, speedBase * rampDown);
        }

        // ป้องกันไม่ให้ต่ำกว่า 0 (ก่อนคูณด้วย PID) แล้วค่อย constrain ขั้นสุดท้าย
        speedTarget = max(speedTarget, 0.0f);

        // ---------- ผสม PID เข้ากับความเร็วซ้าย/ขวา ----------
        int rightSpeed = constrain((int)(speedTarget - output), 10, 100);
        int leftSpeed  = constrain((int)(speedTarget + output), 10, 100);

        // ---------- ส่งคำสั่งมอเตอร์ ----------
        if (Movedirection == 0) {  // เดินหน้า
            motor(1, leftSpeed);
            motor(2, rightSpeed);
            if (Set_Mode_Gyro == 0) {
                motor(3, leftSpeed);
                motor(4, rightSpeed);
            }
        } else {                   // ถอยหลัง
            motor(1, -rightSpeed);
            motor(2, -leftSpeed);
            if (Set_Mode_Gyro == 0) {
                motor(3, -rightSpeed);
                motor(4, -leftSpeed);
            }
        }
    }
    ao(); // หยุดเมื่อครบเวลา
}

void moveStraightPID(int Movedirection, float targetYaw_straight, int speedBase, float duration, float kp_straight, float ki_straight, float kd_straight,int Slow) {
    unsigned long startTime = millis();
    unsigned long startTime_reduc = millis();
    unsigned long endTime = startTime + duration; 
    float integral_straight = 0, preverror_straight = 0;

    while (millis() < endTime) {   
        float current_Yaw = Con_yaw_loop1;
        
        // 🔥 ใช้ normalizeAngle() เพื่อลดปัญหาการเปลี่ยนทิศทางผิดพลาด
        float error_straight = normalizeAngle(targetYaw_straight - current_Yaw);

        unsigned long now = millis();
        float deltaTime = max((now - startTime) / 1000.0, 0.001);
        startTime = now;

        integral_straight += error_straight * deltaTime;
        integral_straight = constrain(integral_straight, -50, 50);  
        float derivative = (error_straight - preverror_straight) / deltaTime;
        float output = kp_straight * error_straight + ki_straight * integral_straight + kd_straight * derivative;

        preverror_straight = error_straight;
        int rightSpeed;
        int leftSpeed;

        if(Slow == 1){
          unsigned long elapsedTime = now - startTime_reduc;
          long timeError = endTime - now;
          float speedReduction;
          if (timeError <= 500 && timeError > 0) {
            speedReduction = timeError * 0.01;
            speedReduction = constrain(speedReduction, 0, speedBase);
          } else if (timeError <= 0) {
            speedReduction = 0;  // หยุดเมื่อครบเวลา
          } else {
            speedReduction = speedBase; // ก่อนถึงช่วงชะลอ ให้เต็มที่
          }
          rightSpeed = constrain(speedReduction - output, 10, 100);
          leftSpeed = constrain(speedReduction + output, 10, 100);
        }
        else{
          rightSpeed = constrain(speedBase - output, 10, 100);
          leftSpeed = constrain(speedBase + output, 10, 100);
        }
        if (Movedirection == 0) {  // เดินหน้า
            motor(1, leftSpeed);
            motor(2, rightSpeed);
            if(Set_Mode_Gyro == 0){
              motor(3, leftSpeed);
              motor(4, rightSpeed);
            }
            
        } else {  // ถอยหลัง
            motor(1, -rightSpeed);
            motor(2, -leftSpeed);
            if(Set_Mode_Gyro == 0){
              motor(3, -rightSpeed);
              motor(4, -leftSpeed);
            } 
        }
    }
    ao(); 
}


void MoveStraightDirection(int Movedirection, int targetYaw_straight, int speedBase, float duration, float kp_straight, float ki_straight, float kd_straight, int Slow) {
  float targetYaw_MovePID = 0;
  float currentYaw = offset_yaw_loop1;  // อ่านค่าปัจจุบัน

  if (Con_yaw_loop1 >= 360 || Con_yaw_loop1 <= 0) {
    status_resetYaw = 1;
    //resetContinuousYaw();
  }

  // คำนวณเป้าหมายตามทิศทาง
  switch (targetYaw_straight) {
    case 0:  // N (North)
      targetYaw_MovePID = currentYaw;
      break;
    case 1:  // E (East)
      targetYaw_MovePID = currentYaw + 90;
      break;
    case 2:  // S (South)
      targetYaw_MovePID = currentYaw + 180;
      break;
    case 3:  // W (West)
      targetYaw_MovePID = currentYaw - 90;
      break;
    case 4:  // W (West)
      targetYaw_MovePID = currentYaw - 135;
      break;
    case 5:  // W (West)
      targetYaw_MovePID = currentYaw - 45;
      break;
    case 6:  // W (West)
      targetYaw_MovePID = currentYaw + 45;
      break;
    case 7:  // W (West)
      targetYaw_MovePID = currentYaw + 135;
      break;
  }

  // 🔥 ใช้ normalizeAngle() ป้องกันมุมเกิน 360° หรือติดลบ
  targetYaw_MovePID = normalizeAngle(targetYaw_MovePID);

  // Serial.print("Move Direction: "); Serial.print(Movedirection);
  // Serial.print(" Target Yaw MovePID: "); Serial.println(targetYaw_MovePID);

  moveStraightPID(Movedirection, targetYaw_MovePID, speedBase, duration, kp_straight, ki_straight, kd_straight, Slow);
}


void moveStraightPID_Encoder(int Movedirection, float targetYaw_straight, int speedBase, int target_encoder, float kp_straight, float ki_straight, float kd_straight, int Slow) {

  long initialEncoderCount = get_pulse_Encoder(selection_Encoder);
  unsigned long startTime = millis();
  float integral_straight = 0, preverror_straight = 0;

  while (abs(get_pulse_Encoder(selection_Encoder) - initialEncoderCount) < target_encoder) {
    //updateContinuousYaw();
    float current_Yaw = Con_yaw_loop1;
    float error_straight = normalizeAngle(targetYaw_straight - current_Yaw);

    unsigned long now = millis();
    float deltaTime = max((now - startTime) / 1000.0, 0.001);
    startTime = now;

    integral_straight += error_straight * deltaTime;
    integral_straight = constrain(integral_straight, -50, 50);
    float derivative = (error_straight - preverror_straight) / deltaTime;
    float output = kp_straight * error_straight + ki_straight * integral_straight + kd_straight * derivative;

    preverror_straight = error_straight;

    int rightSpeed;
    int leftSpeed;

    if (Slow == 1) {
      float kp_encoder = 0.05;
      long encoderMoved = abs(get_pulse_Encoder(selection_Encoder) - initialEncoderCount);
      long encoderError = target_encoder - encoderMoved;
      float speedReduction = encoderError * kp_encoder;
      speedReduction = constrain(speedReduction, 10, speedBase);

      rightSpeed = constrain(speedReduction - output, 0, 100);
      leftSpeed = constrain(speedReduction + output, 0, 100);
    } else {
      rightSpeed = constrain(speedBase - output, 0, 100);
      leftSpeed = constrain(speedBase + output, 0, 100);
    }
    if (Movedirection == 0) {  // เดินหน้า
      motor(1, leftSpeed);
      motor(2, rightSpeed);
      motor(3, leftSpeed);
      motor(4, rightSpeed);
    } else {  // ถอยหลัง
      motor(1, -rightSpeed);
      motor(2, -leftSpeed);
      motor(3, -rightSpeed);
      motor(4, -leftSpeed);
    }


    // Serial.print("Current Yaw: "); Serial.print(current_Yaw);
    // Serial.print(" Target Yaw: "); Serial.print(targetYaw_straight);
    // Serial.print(" Error: "); Serial.print(error_straight);
    // Serial.print(" PID Output: "); Serial.print(output);
    // Serial.print(" encoder.getCount() "); Serial.print(encoder.getCount());
    // Serial.print(" Right Speed: "); Serial.println(abs(encoder.getCount() - initialEncoderCount));
  }

  ao();
}
void MoveStraightDirection_Encoder(int Movedirection, int targetYaw_straight, int speedBase, float duration, float kp_straight, float ki_straight, float kd_straight, int Slow) {
  float targetYaw_MovePID = 0;
  float currentYaw = offset_yaw_loop1;  // อ่านค่าปัจจุบัน

  if (Con_yaw_loop1 >= 360 || Con_yaw_loop1 <= 0) {
    status_resetYaw = 1;
  }
  switch (targetYaw_straight) {
    case 0:  // N (North)
      targetYaw_MovePID = currentYaw;
      break;
    case 1:  // E (East)
      targetYaw_MovePID = currentYaw + 90;
      break;
    case 2:  // S (South)
      targetYaw_MovePID = currentYaw + 180;
      break;
    case 3:  // W (West)
      targetYaw_MovePID = currentYaw - 90;
      break;
    case 4:  // W (West)
      targetYaw_MovePID = currentYaw - 135;
      break;
    case 5:  // W (West)
      targetYaw_MovePID = currentYaw - 45;
      break;
    case 6:  // W (West)
      targetYaw_MovePID = currentYaw + 45;
      break;
    case 7:  // W (West)
      targetYaw_MovePID = currentYaw + 135;
      break;
  }
  targetYaw_MovePID = normalizeAngle(targetYaw_MovePID);
  moveStraightPID_Encoder(Movedirection, targetYaw_MovePID, speedBase, duration, kp_straight, ki_straight, kd_straight, Slow);
}

int getClosestAngle(float currentAngle) {
  // ปรับให้อยู่ในช่วง 0-360
  while (currentAngle < 0) currentAngle += 360;
  while (currentAngle >= 360) currentAngle -= 360;
  int angles[] = { 0, 90, 180, 270, 360 };
  int closest = angles[0];
  float minDiff = fabs(currentAngle - angles[0]);
  for (int i = 1; i < 5; i++) {
    float diff = fabs(currentAngle - angles[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closest = angles[i];
    }
  }
  if (closest == 360) closest = 0;
  return closest + (offset_yaw_loop1 - 180);
}
int Fix_direction(){
  int currentAngle = Con_yaw_loop1;
  while (currentAngle < 0) currentAngle += 360;
  while (currentAngle >= 360) currentAngle -= 360;
  int angles[] = { 0, 90, 180, 270, 360 };
  int closest = angles[0];
  float minDiff = fabs(currentAngle - angles[0]);
  for (int i = 1; i < 5; i++) {
    float diff = fabs(currentAngle - angles[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closest = angles[i];
    }
  }
  if (closest == 360) closest = 0;
  return closest + (offset_yaw_loop1 - 180);

}
void turnByAngle_fix(int turnAngle, int condition) {
  float currentYaw = Con_yaw_loop1;                        // อ่านค่า Yaw ปัจจุบัน
  float roughTargetYaw = currentYaw + turnAngle;           // มุมเป้าหมายโดยตรง
  int snappedTargetYaw = getClosestAngle(roughTargetYaw);  // Snap ไปมุม 0,90,180,270,360
  turnPID(condition, snappedTargetYaw, speedMin_turnDirection, speedMax_turnDirection, turnDirection_PID_KP, turnDirection_PID_KD);
  status_resetYaw = 1;
}
void moveStraightSnapToNearest(int Movedirection, int speedBase, int target_encoder, float kp, float ki, float kd, int Slow) {
  // for (int i = 0; i < 5; i++) updateContinuousYaw();
  float currentYaw = Con_yaw_loop1;              // อ่านมุมจริงจาก Gyro
  int snappedYaw = getClosestAngle(currentYaw);  // หามุมเป้าหมายใกล้สุด เช่น 0, 90, ...
  moveStraightPID_Encoder(Movedirection, snappedYaw, speedBase, target_encoder, kp, ki, kd, Slow);
}

// Mecanum slide with snap-to-nearest heading (0/90/180/270)
static inline void mecanumSlideSnapPID_time(int dir, int speedBase, float duration,
                                            float kp, float ki, float kd, int Slow)
{
  float currentYaw = Con_yaw_loop1;
  int snappedYaw = getClosestAngle(currentYaw);

  unsigned long startTime = millis();
  unsigned long startTime_reduc = millis();
  unsigned long endTime = startTime + (unsigned long)duration;

  float integral = 0.0f;
  float prev_error = 0.0f;

  while (millis() < endTime) {
    float error = normalizeAngle(snappedYaw - Con_yaw_loop1);

    unsigned long now = millis();
    float deltaTime = max((now - startTime) / 1000.0f, 0.001f);
    startTime = now;

    integral += error * deltaTime;
    integral = constrain(integral, -50, 50);
    float derivative = (error - prev_error) / deltaTime;
    float output = kp * error + ki * integral + kd * derivative;
    prev_error = error;

    int baseSpeed;
    if (Slow == 1) {
      long timeError = endTime - now;
      float speedReduction;
      if (timeError <= 500 && timeError > 0) {
        speedReduction = timeError * 0.01f;
        speedReduction = constrain(speedReduction, 0, speedBase);
      } else if (timeError <= 0) {
        speedReduction = 0;
      } else {
        speedReduction = speedBase;
      }
      baseSpeed = (int)speedReduction;
    } else {
      baseSpeed = speedBase;
    }

    int fl, fr, rl, rr;
    if (dir == 0) { // slide left
      fl = -baseSpeed;
      fr = baseSpeed;
      rl = baseSpeed;
      rr = -baseSpeed;
    } else { // slide right
      fl = baseSpeed;
      fr = -baseSpeed;
      rl = -baseSpeed;
      rr = baseSpeed;
    }

    fl = constrain(fl + output, -100, 100);
    rl = constrain(rl + output, -100, 100);
    fr = constrain(fr - output, -100, 100);
    rr = constrain(rr - output, -100, 100);

    motor(1, fl);
    motor(2, fr);
    if (Set_Mode_Gyro == 0) {
      motor(3, rl);
      motor(4, rr);
    }
  }
  ao();
}

// Mecanum slide with fixed target heading (0/90/180/270 or custom)
static inline void mecanumSlidePID_time(int dir, int speedBase, float duration,
                                        float targetYaw, float kp, float ki, float kd, int Slow)
{
  unsigned long startTime = millis();
  unsigned long startTime_reduc = millis();
  unsigned long endTime = startTime + (unsigned long)duration;

  float integral = 0.0f;
  float prev_error = 0.0f;

  while (millis() < endTime) {
    float error = normalizeAngle(targetYaw - Con_yaw_loop1);

    unsigned long now = millis();
    float deltaTime = max((now - startTime) / 1000.0f, 0.001f);
    startTime = now;

    integral += error * deltaTime;
    integral = constrain(integral, -50, 50);
    float derivative = (error - prev_error) / deltaTime;
    float output = kp * error + ki * integral + kd * derivative;
    prev_error = error;

    int baseSpeed;
    if (Slow == 1) {
      long timeError = endTime - now;
      float speedReduction;
      if (timeError <= 500 && timeError > 0) {
        speedReduction = timeError * 0.01f;
        speedReduction = constrain(speedReduction, 0, speedBase);
      } else if (timeError <= 0) {
        speedReduction = 0;
      } else {
        speedReduction = speedBase;
      }
      baseSpeed = (int)speedReduction;
    } else {
      baseSpeed = speedBase;
    }

    int fl, fr, rl, rr;
    if (dir == 0) { // slide left
      fl = -baseSpeed;
      fr = baseSpeed;
      rl = baseSpeed;
      rr = -baseSpeed;
    } else { // slide right
      fl = baseSpeed;
      fr = -baseSpeed;
      rl = -baseSpeed;
      rr = baseSpeed;
    }

    fl = constrain(fl + output, -100, 100);
    rl = constrain(rl + output, -100, 100);
    fr = constrain(fr - output, -100, 100);
    rr = constrain(rr - output, -100, 100);

    motor(1, fl);
    motor(2, fr);
    if (Set_Mode_Gyro == 0) {
      motor(3, rl);
      motor(4, rr);
    }
  }
  ao();
}

static inline void mecanumSlideSnapPID_Encoder(int dir, int speedBase, long target_encoder,
                                               float kp, float ki, float kd, int Slow)
{
  float currentYaw = Con_yaw_loop1;
  int snappedYaw = getClosestAngle(currentYaw);

  long initialEncoder = get_pulse_Encoder(selection_Encoder);
  float integral = 0.0f;
  float prev_error = 0.0f;
  unsigned long lastTime = millis();

  while (abs(get_pulse_Encoder(selection_Encoder) - initialEncoder) < target_encoder) {
    float error = normalizeAngle(snappedYaw - Con_yaw_loop1);

    unsigned long now = millis();
    float deltaTime = max((now - lastTime) / 1000.0f, 0.001f);
    lastTime = now;

    integral += error * deltaTime;
    integral = constrain(integral, -50, 50);
    float derivative = (error - prev_error) / deltaTime;
    float output = kp * error + ki * integral + kd * derivative;
    prev_error = error;

    int baseSpeed = speedBase;
    if (Slow == 1) {
      long moved = abs(get_pulse_Encoder(selection_Encoder) - initialEncoder);
      long remain = target_encoder - moved;
      long s = (long)(remain * 0.05f);
      if (s < 10) s = 10;
      if (s < baseSpeed) baseSpeed = (int)s;
    }

    int fl, fr, rl, rr;
    if (dir == 0) { // slide left
      fl = -baseSpeed;
      fr = baseSpeed;
      rl = baseSpeed;
      rr = -baseSpeed;
    } else { // slide right
      fl = baseSpeed;
      fr = -baseSpeed;
      rl = -baseSpeed;
      rr = baseSpeed;
    }

    fl = constrain(fl + output, -100, 100);
    rl = constrain(rl + output, -100, 100);
    fr = constrain(fr - output, -100, 100);
    rr = constrain(rr - output, -100, 100);

    motor(1, fl);
    motor(2, fr);
    if (Set_Mode_Gyro == 0) {
      motor(3, rl);
      motor(4, rr);
    }
  }
  ao();
}

static inline void mecanumSlidePID_Encoder(int dir, int speedBase, long target_encoder,
                                           float targetYaw, float kp, float ki, float kd, int Slow)
{
  long initialEncoder = get_pulse_Encoder(selection_Encoder);
  float integral = 0.0f;
  float prev_error = 0.0f;
  unsigned long lastTime = millis();

  while (abs(get_pulse_Encoder(selection_Encoder) - initialEncoder) < target_encoder) {
    float error = normalizeAngle(targetYaw - Con_yaw_loop1);

    unsigned long now = millis();
    float deltaTime = max((now - lastTime) / 1000.0f, 0.001f);
    lastTime = now;

    integral += error * deltaTime;
    integral = constrain(integral, -50, 50);
    float derivative = (error - prev_error) / deltaTime;
    float output = kp * error + ki * integral + kd * derivative;
    prev_error = error;

    int baseSpeed = speedBase;
    if (Slow == 1) {
      long moved = abs(get_pulse_Encoder(selection_Encoder) - initialEncoder);
      long remain = target_encoder - moved;
      long s = (long)(remain * 0.05f);
      if (s < 10) s = 10;
      if (s < baseSpeed) baseSpeed = (int)s;
    }

    int fl, fr, rl, rr;
    if (dir == 0) { // slide left
      fl = -baseSpeed;
      fr = baseSpeed;
      rl = baseSpeed;
      rr = -baseSpeed;
    } else { // slide right
      fl = baseSpeed;
      fr = -baseSpeed;
      rl = -baseSpeed;
      rr = baseSpeed;
    }

    fl = constrain(fl + output, -100, 100);
    rl = constrain(rl + output, -100, 100);
    fr = constrain(fr - output, -100, 100);
    rr = constrain(rr - output, -100, 100);

    motor(1, fl);
    motor(2, fr);
    if (Set_Mode_Gyro == 0) {
      motor(3, rl);
      motor(4, rr);
    }
  }
  ao();
}

// Loop-only mecanum slide (snap heading); call repeatedly in loop
static inline void mecanumSlideSnapPID_step(int dir, int speedBase,
                                            float kp, float ki, float kd)
{
  static float integral = 0.0f;
  static float prev_error = 0.0f;
  static unsigned long lastTime = millis();

  int snappedYaw = getClosestAngle(Con_yaw_loop1);
  float error = normalizeAngle(snappedYaw - Con_yaw_loop1);

  unsigned long now = millis();
  float deltaTime = max((now - lastTime) / 1000.0f, 0.001f);
  lastTime = now;

  integral += error * deltaTime;
  integral = constrain(integral, -50, 50);
  float derivative = (error - prev_error) / deltaTime;
  prev_error = error;
  float output = kp * error + ki * integral + kd * derivative;

  int fl, fr, rl, rr;
  if (dir == 0) { // slide left
    fl = -speedBase;
    fr = speedBase;
    rl = speedBase;
    rr = -speedBase;
  } else { // slide right
    fl = speedBase;
    fr = -speedBase;
    rl = -speedBase;
    rr = speedBase;
  }

  fl = constrain(fl + output, -100, 100);
  rl = constrain(rl + output, -100, 100);
  fr = constrain(fr - output, -100, 100);
  rr = constrain(rr - output, -100, 100);

  motor(1, fl);
  motor(2, fr);
  if (Set_Mode_Gyro == 0) {
    motor(3, rl);
    motor(4, rr);
  }
}

// Loop-only mecanum slide with fixed target heading; call repeatedly in loop
static inline void mecanumSlidePID_step(int dir, int speedBase, float targetYaw,
                                        float kp, float ki, float kd)
{
  static float integral = 0.0f;
  static float prev_error = 0.0f;
  static unsigned long lastTime = millis();

  float error = normalizeAngle(targetYaw - Con_yaw_loop1);

  unsigned long now = millis();
  float deltaTime = max((now - lastTime) / 1000.0f, 0.001f);
  lastTime = now;

  integral += error * deltaTime;
  integral = constrain(integral, -50, 50);
  float derivative = (error - prev_error) / deltaTime;
  prev_error = error;
  float output = kp * error + ki * integral + kd * derivative;

  int fl, fr, rl, rr;
  if (dir == 0) { // slide left
    fl = -speedBase;
    fr = speedBase;
    rl = speedBase;
    rr = -speedBase;
  } else { // slide right
    fl = speedBase;
    fr = -speedBase;
    rl = -speedBase;
    rr = speedBase;
  }

  fl = constrain(fl + output, -100, 100);
  rl = constrain(rl + output, -100, 100);
  fr = constrain(fr - output, -100, 100);
  rr = constrain(rr - output, -100, 100);

  motor(1, fl);
  motor(2, fr);
  if (Set_Mode_Gyro == 0) {
    motor(3, rl);
    motor(4, rr);
  }
}

void moveStraightPID_step(int Movedirection, float targetYaw_straight, int speedBase, float kp_straight, float kd_straight) {
    static float integral_straight = 0;
    static float preverror_straight = 0;
    static unsigned long lastTime = millis();

   
    float current_Yaw = Con_yaw_loop1;
    float error_straight = normalizeAngle(targetYaw_straight - current_Yaw);

    unsigned long now = millis();
    float deltaTime = max((now - lastTime) / 1000.0, 0.001);
    lastTime = now;  // สำคัญ! ต้องใช้ static

    integral_straight += error_straight * deltaTime;
    integral_straight = constrain(integral_straight, -50, 50);
    float derivative = (error_straight - preverror_straight) / deltaTime;
    preverror_straight = error_straight;

    float output = kp_straight * error_straight + kd_straight * derivative;

    int rightSpeed = constrain(speedBase - output, 0, 100);
    int leftSpeed  = constrain(speedBase + output, 0, 100);

    if (Movedirection == 0) {
        motor(1, leftSpeed);
        motor(2, rightSpeed);
        if(Set_Mode_Gyro == 0){
            motor(3, leftSpeed);
            motor(4, rightSpeed);
        }
    } else {
        motor(1, -rightSpeed);
        motor(2, -leftSpeed);
        if(Set_Mode_Gyro == 0){
            motor(3, -rightSpeed);
            motor(4, -leftSpeed);
        } 
    }
}
