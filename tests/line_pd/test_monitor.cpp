#include <Arduino.h>
#include "PuppyBotLinePD.h"
#include <iostream>
#include <stdexcept>
#include <string>
int glyphDelay=0;
void tftDelay() { fakeMillis+=glyphDelay; }
#define PUPPYBOT_TFT_OPERATION_HOOK tftDelay
#include <Adafruit_ST7735.h>
Adafruit_ST7735 tft_;
enum class LineSide { Front, Back };
bool frontSaved=true,backSaved=true;
int Sensor_mode_F=1,Sensor_mode_B=1,PID_NumPin=8,PID_NumPin_B=8;
int frontPosition=350,backPosition=350,frontReads=0,backReads=0;
uint16_t state_on_Line=1,state_on_Line_B=1;
int motors[2]={},stops=0,motorWrites=0;
float errors=0,derivative=0,output=0,previous_error=0;
float errors_B=0,derivative_B=0,output_B=0,previous_error_B=0;
bool hasLineCalibration(LineSide s){return s==LineSide::Front?frontSaved:backSaved;}
int readline(){++frontReads;return frontPosition;}
int readline_B(){++backReads;return backPosition;}
int Read_sumValue_sensor(){return 800;}
int Read_sumValue_sensor_B(){return 800;}
void ao(){resetLinePD();++stops;motors[0]=motors[1]=0;}
void motor(int n,int command){++motorWrites;motors[n-1]=command;}
#include "PuppyBotLinePDMonitor.h"
#include "ActualLegacyPD.h"
void require(bool x,const char *message){if(!x)throw std::runtime_error(message);}
bool near(float a,float b){return fabsf(a-b)<.01f;}

void testScreens(const std::string &dir) {
 for(bool portrait:{false,true}) {
  Adafruit_ST7735 display(portrait?128:160,portrait?160:128);
  PuppyBotLineMonitorDetail::Screen<Adafruit_ST7735> screen(display);
  screen.begin(); LinePDOptions options; LinePDMonitorReading v;
  v.pd.valid=true;v.pd.lineSeen=true;v.pd.position=350;v.wheelStatus=WheelSpeedStatus::Ready;
  v.rate1Valid=v.rate2Valid=true;
  screen.show(v,options);auto original=display.pixels;
  display.operations.clear();screen.show(v,options);
  require(display.operations.empty(),"unchanged screen must perform no writes");
  auto big=v;big.preview=false;big.pd.position=700;big.pd.error=350;big.pd.pTerm=31.5f;
  big.pd.dTerm=-9.75f;big.pd.dRaw=-123.5;big.pd.dFiltered=-39;
  big.pd.correction=20;big.pd.limited=true;big.pd.source=LinePDSource::Encoder;big.pd.dtMs=20;
  big.target1=7778;big.target2=1556;big.actual1=7000;big.actual2=1800;
  big.pwm1=53;big.pwm2=9;big.wheelStatus=WheelSpeedStatus::Running;
  screen.show(big,options);display.save(dir+(portrait?"/monitor-run-portrait.ppm":"/monitor-run.ppm"));
  screen.show(v,options);require(display.pixels==original,"shrinking values and LIMIT removed without stale glyphs");
  require(display.fullClears==1,"no extra full-screen clear on update");
  for(auto op:display.operations) require(op.w==6&&op.h==8,"partial drawing is bounded to one opaque glyph");
  display.operations.clear();screen.show(big,options,true);const auto before=display.operations.size();
  require(before==0,"deferred preparation performs no I/O");
  require(screen.drawPending()&&display.operations.size()==before+1,"one service call draws one glyph");
  while(screen.drawPending()){}
  Adafruit_ST7735 fresh(portrait?128:160,portrait?160:128);
  PuppyBotLineMonitorDetail::Screen<Adafruit_ST7735> freshScreen(fresh);freshScreen.begin();freshScreen.show(big,options);
  require(display.pixels==fresh.pixels,"deferred screen converges to complete render");
  big.wheelStatus=WheelSpeedStatus::EncoderStalled;big.preview=true;
  big.target1=big.target2=big.pwm1=big.pwm2=0;screen.show(big,options);
  display.save(dir+(portrait?"/monitor-fault-portrait.ppm":"/monitor-fault.ppm"));
 }
}
int main(int argc,char **argv){
 try {
  require(argc==2,"output directory");testScreens(argv[1]);
  WheelSpeedSettings settings;settings.motor1.encoderSign=-1;settings.motor2.encoderSign=1;
  settings.motor1.referenceTicksPerSecond=settings.motor2.referenceTicksPerSecond=4667;
  require(beginWheelSpeedControl(settings),"real adapter begins with confirmed mapping");
  require(beginLinePDMonitor(30,.09f,.05f),"enable idle monitor");
  const auto writes=motorWrites,resets=FakeEncoder::resets[17]+FakeEncoder::resets[19];
  updateLinePDMonitor();fakeMillis+=100;FakeEncoder::counts[17]-=100;FakeEncoder::counts[19]+=200;
  frontPosition=450;updateLinePDMonitor();auto m=getLinePDMonitorReading();
  require(m.preview&&m.pd.position==450&&near(m.pd.pTerm,9),"idle reads selected saved line and evaluates P");
  require(near(m.actual1,1000)&&near(m.actual2,2000),"signed independent passive encoder rates");
  require(m.target1==0&&m.target2==0&&motorWrites==writes&&motors[0]==0&&motors[1]==0,"preview never actuates motors");
  require(!getLinePDTelemetry().valid,"preview does not overwrite actual control history");
  require(FakeEncoder::resets[17]+FakeEncoder::resets[19]==resets,"preview never resets counters");
  for(int i=0;i<400;++i) updateLinePDMonitor();
  Adafruit_ST7735 expectedPreview;
  PuppyBotLineMonitorDetail::Screen<Adafruit_ST7735> expectedPreviewScreen(expectedPreview);
  expectedPreviewScreen.begin();expectedPreviewScreen.show(getLinePDMonitorReading(),getLinePDOptions());
  require(tft_.pixels==expectedPreview.pixels,"runtime incremental preview converges to complete telemetry screen");
  tft_.save(std::string(argv[1])+"/monitor-preview.ppm");
  frontSaved=false;fakeMillis+=100;updateLinePDMonitor();
  require(!getLinePDMonitorReading().pd.valid,"missing saved calibration is explicit");frontSaved=true;
  require(restartWheelSpeedControl(),"restart after preview");frontPosition=350;
  fakeMillis+=100;require(Run_PID_Encoder(30,.09f,.05f),"run live encoder PD");
  m=getLinePDMonitorReading();
  require(!m.preview&&m.pd.source==LinePDSource::Encoder&&near(m.target1,4667),"automatic hook shows actual wheel target");
  require(m.pd.dTerm==0,"preview movement cannot seed live derivative");
  require(!beginLinePDMonitor(30,.09f,.05f),"reject full-screen initialization during drive");
  const int readBefore=frontReads;
  fakeMillis+=100;FakeEncoder::counts[17]-=467;FakeEncoder::counts[19]+=467;
  frontPosition=370;require(Run_PID_Encoder(30,.09f,.05f),"live correction tick");m=getLinePDMonitorReading();
  require(frontReads==readBefore+1,"live monitor never resamples line or alters live history");
  const auto w=getWheelSpeedTelemetry();const auto p=getLinePDTelemetry();
  require(near(m.pd.pTerm,p.pTerm)&&near(m.pd.dTerm,p.dTerm)&&near(m.target2,w.motor2.targetTicksPerSecond)&&near(m.actual2,w.motor2.ticksPerSecond),"monitor displays controller telemetry");
  stopWheelSpeedControl();fakeMillis+=100;updateLinePDMonitor();m=getLinePDMonitorReading();
  require(m.preview&&m.target1==0&&m.target2==0,"stop returns to passive preview and zero targets");
  Run_PID(30,1000,1000);require(motors[0]==50&&motors[1]==10,"actual legacy front uses same steering cap");
  fakeMillis+=100;Run_PID(30,1000,1000);m=getLinePDMonitorReading();
  require(!m.preview&&!m.targetsValid&&m.pd.source==LinePDSource::Legacy,"legacy PD has no invented ticks/s target");
  backPosition=700;Run_PID_B_until_backSensor(30,1000,1000,400);
  require(motors[0]==-10&&motors[1]==-50,"legacy back-until uses protected reverse mix");
  frontPosition=700;Run_PID_until_frontSensor(30,1000,1000,400);
  require(motors[0]==50&&motors[1]==10,"legacy front-until uses protected mix");
  ao();endLinePDMonitor();
  require(!isLinePDMonitorActive()&&PuppyBotLinePDDetail::monitorService==nullptr,"end unregisters hook");
  // Inject a 12 ms SPI cost per glyph. This exercises the real adapter with the live screen enabled.
  require(restartWheelSpeedControl(),"restart for delayed TFT");frontPosition=350;
  require(beginLinePDMonitor(30,.09f,.05f),"enable delayed monitor");glyphDelay=12;
  const auto started=fakeMillis;uint32_t previous=fakeMillis;
  float count1=float(FakeEncoder::counts[17]),count2=float(FakeEncoder::counts[19]);
  while(uint32_t(fakeMillis-started)<2000) {
   const uint32_t dt=fakeMillis-previous;previous=fakeMillis;
   count1-=motors[0]/30.f*4667*dt/1000;count2+=motors[1]/30.f*4667*dt/1000;
   FakeEncoder::counts[17]=int(count1);FakeEncoder::counts[19]=int(count2);
   require(Run_PID_Encoder(30,.09f,.05f),"slow TFT cannot trip wheel watchdog");
  }
  require(getWheelSpeedTelemetry().status==WheelSpeedStatus::Running,"controller remains running with delayed glyph I/O");
  glyphDelay=0;stopWheelSpeedControl();endLinePDMonitor();
  require(beginLinePDMonitor(30,.09f,.05f,LineSide::Back),"back preview selectable");
  updateLinePDMonitor();require(getLinePDMonitorReading().pd.back&&getLinePDMonitorReading().pd.position==backPosition,"back preview uses saved back sensor");
  endLinePDMonitor();
  std::cout<<"PASS monitor: passive preview, signed rates, no motor/reset/history side effects, actual live telemetry, legacy paths, bounded incremental TFT, slow-SPI control\n";
  return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
