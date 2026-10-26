#define main previousAdapterTests
#include "test_adapter.cpp"
#undef main
#include <string>
#include <vector>
float previous_error = 0;
int legacyCalls = 0;
uint32_t crossingAt = 200, plantAt = 0, trialStart = 0, seenSamples = 0;
bool unplugged = false;
double pos1=0,pos2=0;
struct Command { uint32_t elapsed; float motor1, motor2; };
std::vector<Command> legacyCommands, encoderCommands;
std::string shownMode,shownDetail;
bool isLinePDMonitorActive(){return false;}
struct SerialLog { void println(const char*) {} } Serial;
void show_line_drive_status(const char *mode,const char *detail){shownMode=mode;shownDetail=detail;}
int Read_sumValue_sensor(){return uint32_t(fakeMillis-trialStart)>=crossingAt ? 500 : 100;}
void Run_PID(int speed,float,float){
 ++legacyCalls;legacyCommands.push_back({uint32_t(fakeMillis-trialStart),float(speed),float(speed)});
 motor(1,speed);motor(2,speed);delay(1);
}
#include "ActualUserFront.h"
void plant(){
 const auto t=getWheelSpeedTelemetry();
 if(t.status==WheelSpeedStatus::Running&&t.samples!=seenSamples){
  seenSamples=t.samples;
  // The real adapter calls delay(1) immediately after a control tick.
  encoderCommands.push_back({uint32_t(fakeMillis-trialStart-1),
                             t.motor1.targetTicksPerSecond/10.0f,t.motor2.targetTicksPerSecond/15.0f});
 }
 const uint32_t dt=fakeMillis-plantAt;plantAt=fakeMillis;
 if(!unplugged) pos1+=motors[0]*10.0*dt/1000;
 pos2-=motors[1]*15.0*dt/1000;
 FakeEncoder::counts[17]=int(pos1);FakeEncoder::counts[19]=int(pos2);
}
void resetJob(){
 fakeMillis=plantAt=trialStart=seenSamples=0;pos1=pos2=0;unplugged=false;legacyCalls=0;crossingAt=200;
 legacyCommands.clear();encoderCommands.clear();
 FakeEncoder::counts[17]=FakeEncoder::counts[19]=0;frontPosition=350;frontSaved=true;
 fakeDelayHook=plant;shownMode.clear();shownDetail.clear();ao();
}
void requireStopped(){require(motors[0]==0&&motors[1]==0,"both motors stopped");}
void requireRamp(const std::vector<Command>& commands,uint32_t duration){
 require(!commands.empty()&&commands.front().motor1>0&&commands.front().motor1<30,"starts below requested speed");
 float previous=0;bool reached=false;
 for(const auto &c:commands){
  require(c.motor1+0.001f>=previous&&c.motor1<=30.001f,"base target increases monotonically and stays bounded");
  require(fabs(c.motor1-c.motor2)<0.001f,"centered line keeps both command percentages equal");
  if(c.motor1>=29.999f&&!reached){
   require(c.elapsed>=duration&&c.elapsed<=duration+20,"reaches requested target at ramp end within one control period");
   reached=true;
  }
  previous=c.motor1;
 }
 require(reached,"reaches requested speed");
}
int main(){
 try{
  WheelSpeedSettings s;s.motor1.encoderSign=1;s.motor2.encoderSign=-1;
  s.motor1.referenceTicksPerSecond=300;s.motor2.referenceTicksPerSecond=450;
  resetJob();beginWheelSpeedControl(WheelSpeedSettings{});
  require(lineFrontCross(30,.05f,1,500,10000),"legacy fallback still reaches crossing when settings absent");
  require(legacyCalls>0&&shownDetail=="LINE PD ONLY"&&motors[0]==0&&motors[1]==0,"fallback explicitly displayed, stopped at crossing");
  resetJob();beginWheelSpeedControl(s);
  require(lineFrontCross(30,.05f,1,500,10000),"configured job reaches crossing");
  require(legacyCalls==0&&shownDetail=="ENCODER + LINE PD"&&motors[0]==0&&motors[1]==0,"configured job uses only closed loop and stops");
  resetJob();beginWheelSpeedControl(s);unplugged=true;crossingAt=99999;
  require(!lineFrontCross(30,.05f,1,500,5000),"encoder fault ends job before task deadline");
  require(legacyCalls==0&&motors[0]==0&&motors[1]==0&&getWheelSpeedTelemetry().status==WheelSpeedStatus::EncoderStalled,
          "fault stops both wheels without switching to legacy");
  resetJob();beginWheelSpeedControl(s);crossingAt=99999;
  require(!lineFrontCross(30,.05f,1,500,100)&&motors[0]==0&&motors[1]==0,"job timeout stops closed loop");
  resetJob();beginWheelSpeedControl(s);crossingAt=0;
  require(lineFrontCross(30,.05f,1,500,10000)&&getWheelSpeedTelemetry().samples==0,"initial crossing does not issue a movement tick");

  resetJob();beginWheelSpeedControl(WheelSpeedSettings{});crossingAt=600;
  require(lineFrontCross(30,.05f,1,500,10000,200),"legacy ramp reaches crossing");
  requireRamp(legacyCommands,200);requireStopped();
  resetJob();beginWheelSpeedControl(s);crossingAt=600;
  require(lineFrontCross(30,.05f,1,500,10000,200),"encoder ramp reaches crossing");
  requireRamp(encoderCommands,200);requireStopped();
  require(legacyCalls==0,"encoder ramp does not fall back");

  resetJob();beginWheelSpeedControl(s);crossingAt=600;
  require(lineFrontCross(30,.05f,1,500,10000),"default ramp enabled");
  requireRamp(encoderCommands,300);requireStopped();
  resetJob();beginWheelSpeedControl(s);
  require(lineFrontCross(30,.05f,1,500,10000,0),"zero duration disables encoder ramp");
  require(encoderCommands.front().elapsed==0&&encoderCommands.front().motor1==30,"disabled ramp starts at requested speed");
  resetJob();beginWheelSpeedControl(WheelSpeedSettings{});
  require(lineFrontCross(30,.05f,1,500,10000,0),"zero duration disables legacy ramp");
  require(legacyCommands.front().elapsed==0&&legacyCommands.front().motor1==30,"legacy ramp disabled");

  resetJob();beginWheelSpeedControl(s);crossingAt=100;
  require(lineFrontCross(30,.05f,1,500,10000,300),"crossing interrupts ramp");
  require(fakeMillis==100&&encoderCommands.back().motor1<30,"does not wait for ramp after crossing");requireStopped();
  resetJob();beginWheelSpeedControl(s);crossingAt=99999;
  require(!lineFrontCross(30,.05f,1,500,100,300)&&fakeMillis==100,"timeout interrupts ramp");requireStopped();
  resetJob();beginWheelSpeedControl(s);unplugged=true;crossingAt=99999;
  require(!lineFrontCross(30,.05f,1,500,5000,2000),"stall protection remains active during ramp");
  require(fakeMillis<2000&&getWheelSpeedTelemetry().status==WheelSpeedStatus::EncoderStalled,"stall stops before ramp ends");requireStopped();

  resetJob();fakeMillis=plantAt=trialStart=0xfffffff0u;beginWheelSpeedControl(s);crossingAt=600;
  require(lineFrontCross(30,.05f,1,500,10000,200),"ramp survives millis wrap");
  requireRamp(encoderCommands,200);requireStopped();
  resetJob();beginWheelSpeedControl(s);crossingAt=700;
  require(lineFrontCross(1,0,0,500,1000,500),"low speed waits for first positive ramp command");
  require(encoderCommands.front().elapsed==500&&fabs(encoderCommands.front().motor1-1)<0.001f,"zero ramp command is not sent to positive-speed API");requireStopped();
  resetJob();beginWheelSpeedControl(s);crossingAt=99999;
  require(!lineFrontCross(1,0,0,500,100,500)&&getWheelSpeedTelemetry().samples==0,"timeout before first ramp command keeps motors stopped");requireStopped();

  resetJob();beginWheelSpeedControl(s);frontPosition=200;crossingAt=600;
  require(lineFrontCross(30,.07f,.035f,500,10000,300),"PD continues steering during ramp");
  require(encoderCommands.front().motor1!=encoderCommands.front().motor2,"line error changes targets before ramp finishes");
  for(const auto &c:encoderCommands)require(c.motor1>=0&&c.motor2>=0,"ramp keeps existing no-reverse steering limit");
  requireStopped();
  resetJob();beginWheelSpeedControl(s);frontSaved=false;
  require(!lineFrontCross(30,.05f,1,500,10000)&&getWheelSpeedTelemetry().samples==0,"missing calibration inhibits ramp");requireStopped();
  resetJob();beginWheelSpeedControl(s);
  require(!lineFrontCross(0,.05f,1,500,10000)&&getWheelSpeedTelemetry().samples==0,"invalid command inhibits ramp");requireStopped();
  require(FakeEncoder::resets[17]==0&&FakeEncoder::resets[19]==0,"ramp never resets either encoder");
  std::cout<<"PASS actual lineFrontCross: encoder/legacy/default/disabled ramp, steering during ramp, crossing/timeout/stall interruption, low speed, millis wrap, calibration gate, no encoder resets\n";
 }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
