#include <Arduino.h>
#include "PuppyBotLinePD.h"
#include <iostream>
#include <stdexcept>
enum class LineSide { Front, Back };
bool frontSaved=true,backSaved=true;
int Sensor_mode_F=1,Sensor_mode_B=1,PID_NumPin=8,PID_NumPin_B=8;
int frontPosition=350,backPosition=350,frontReads=0,backReads=0;
uint16_t state_on_Line=1,state_on_Line_B=1;
int motors[2]={},stops=0;
bool hasLineCalibration(LineSide s){return s==LineSide::Front?frontSaved:backSaved;}
int readline(){++frontReads;return frontPosition;}
int readline_B(){++backReads;return backPosition;}
void ao(){resetLinePD();++stops;motors[0]=motors[1]=0;}
void motor(int n,int command){motors[n-1]=command;}
#include "PuppyBotEncoderLinePD.h"
void require(bool x,const char *message){if(!x)throw std::runtime_error(message);}
int main(){
 try{
  WheelSpeedSettings s;s.motor1.encoderSign=1;s.motor2.encoderSign=-1;
  s.motor1.referenceTicksPerSecond=300;s.motor2.referenceTicksPerSecond=450;
  require(beginWheelSpeedControl(s),"board config");
  require(Run_PID_Encoder(30,.05f,1),"centered front PD");
  require(motors[0]==30&&motors[1]==30&&frontReads==1&&backReads==0,"front source and command shape");
  require(FakeEncoder::resets[17]==0&&FakeEncoder::resets[19]==0,"controller never resets encoder counters");
  fakeMillis+=20;FakeEncoder::counts[17]=6;FakeEncoder::counts[19]=-9;frontPosition=450;
  require(Run_PID_Encoder(30,.05f,0),"front steering tick");auto t=getWheelSpeedTelemetry();
  require(t.motor1.targetTicksPerSecond==350&&t.motor2.targetTicksPerSecond==375,
          "PD steering produces distinct motor targets without equalizing counts");
  stopWheelSpeedControl();require(motors[0]==0&&motors[1]==0,"board stop removes both outputs");
  require(restartWheelSpeedControl(),"new segment re-baselines existing counts");
  backPosition=450;require(Run_PID_B_Encoder(30,.05f,1),"back mode starts without derivative kick");
  t=getWheelSpeedTelemetry();require(t.motor1.targetTicksPerSecond==-250&&t.motor2.targetTicksPerSecond==-525,
                                   "back PD uses reverse motor mix and back sensors");
  require(backReads==1&&FakeEncoder::resets[17]==0,"back read and no hidden encoder reset");
  fakeMillis+=300;require(!Run_PID_B_Encoder(30,.05f,1)&&motors[0]==0&&motors[1]==0,"adapter propagates watchdog fault and stops");
  require(!Run_PID_Encoder(30,.05f,1),"fault cannot silently resume");
  require(restartWheelSpeedControl(),"explicit restart");frontSaved=false;
  require(!Run_PID_Encoder(30,.05f,1)&&motors[0]==0&&motors[1]==0,"unavailable line calibration inhibits drive");frontSaved=true;
  s.motor1.encoderSign=0;require(!beginWheelSpeedControl(s)&&!isWheelSpeedControlConfigured(),"unset physical data leaves controller inactive");
  require(!Run_PID_Encoder(30,.05f,1)&&motors[0]==0&&motors[1]==0,"explicit encoder API never falls back silently");
  require(FakeEncoder::starts[17]==1&&FakeEncoder::starts[19]==1,"uses existing two encoder APIs and pins without rebinding");
  std::cout<<"PASS board adapter: real Encoder API, front/back steering, independent targets, stop/watchdog, unconfigured gate, no encoder resets\n";
  return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
