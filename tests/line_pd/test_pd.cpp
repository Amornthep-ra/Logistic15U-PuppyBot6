#include "PuppyBotLinePD.h"
#include <iostream>
#include <stdexcept>
#include <limits>

void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
bool near(float a, float b) { return fabsf(a-b) < .001f; }
int main() {
 try {
  PuppyBotLinePDDetail::Controller pd;
  LinePDOptions options;
  require(PuppyBotLinePDDetail::monitorService==nullptr,"monitor disabled by default");
  require(pd.step(options,false,350,350,30,.02f,2,100,20,true),"first step");
  require(pd.telemetry().dTerm==0,"first step must not kick D");
  pd.step(options,false,370,350,30,.02f,2,120,20,true);
  auto v=pd.telemetry();
  require(near(v.dRaw,20)&&near(v.dFiltered,5)&&near(v.dTerm,10),"60ms lowpass on measured 20ms interval");
  require(near(v.command1,40.4f)&&near(v.command2,19.6f),"350->370 no reverse impulse");
  pd.step(options,false,350,350,30,.02f,2,140,20,true);
  require(near(pd.telemetry().dTerm,-2.5f),"370->350 derivative filtered instead of -40 impulse");
  pd.reset(); options.derivativeFilterMs=0;
  pd.step(options,false,350,350,30,0,1,0xfffffff0u,20,true);
  pd.step(options,false,390,350,30,0,1,24,20,true);
  require(pd.telemetry().dtMs==40&&near(pd.telemetry().dRaw,20),"real dt and millis wrap");
  require(near(pd.telemetry().dTerm,20),"filter0 bypass");
  pd.step(options,true,700,350,30,0,2,44,20,true);
  require(pd.telemetry().dTerm==0,"switch front/back clears D history");
  pd.reset(); pd.step(options,true,0,350,30,0,2,64,20,true);
  require(pd.telemetry().dTerm==0,"reset clears history");
  options=LinePDOptions{};
  for (int speed: {1,10,30,80,100,-1,-30,-100,0}) for(bool back:{false,true}) {
   pd.reset();
   for(int pos:{0,700,0,500,350}) {
    require(pd.step(options,back,pos,350,speed,1000,1000,v.sampledAt+=20,20,true),"large finite gains");
    v=pd.telemetry();
    require(fabsf(v.correction)<=20&&fabsf(v.command1)<=100.001f&&fabsf(v.command2)<=100.001f,"turn and motor caps");
    const int direction=back?-speed:speed;
    require(direction>=0?(v.command1>=0&&v.command2>=0):(v.command1<=0&&v.command2<=0),"default PD cannot reverse a wheel");
    if(speed==0) require(v.command1==0&&v.command2==0,"zero speed cannot spin");
   }
  }
  options.maxCorrection=100;options.allowWheelReverse=true;pd.reset();
  pd.step(options,false,700,350,30,1,0,100,20,true);
  require(pd.telemetry().command2<0,"reverse requires explicit opt-in");
  options.maxCorrection=101; require(!setLinePDOptions(options),"invalid cap rejected");
  options=LinePDOptions{};options.derivativeFilterMs=-1;require(!setLinePDOptions(options),"negative filter rejected");
  options=LinePDOptions{};
  require(!pd.step(options,false,0,350,30,std::numeric_limits<float>::quiet_NaN(),0,0,20,true),"NaN rejected");
  require(!pd.step(options,false,0,350,30,0,0,0,0,true),"invalid nominal period rejected");
  std::cout<<"PASS PD: filtered D, actual dt/wrap, reset/side switch, caps, no wheel reversal, explicit reverse, invalid settings\n";
 } catch(const std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n';return 1; }
}
