#include <iostream>
#include <stdexcept>
#include <limits>
#include "PuppyBotWheelSpeedControl.h"
using namespace PuppyBotDrive;
void require(bool x, const char *message) { if (!x) throw std::runtime_error(message); }
WheelSpeedSettings fixture() {
    // Synthetic rates/signs for this test only, not measured robot configuration.
    WheelSpeedSettings s;
    s.motor1.encoderSign = 1; s.motor2.encoderSign = -1;
    s.motor1.referenceTicksPerSecond = 300; s.motor2.referenceTicksPerSecond = 450;
    return s;
}
int main() {
    try {
        WheelSpeedController c;
        WheelSpeedSettings unset;
        require(!c.begin(unset,0,0,0) && !c.ready(), "unset sign/rate cannot activate feedback");
        auto s = fixture();
        require(c.begin(s,0,0,0), "explicit reference settings ready");
        require(c.update(1000,0,0,40,20), "Ready state may wait in setup without false stale fault");
        auto t = c.telemetry();
        require(t.motor1.targetTicksPerSecond == 400 && t.motor2.targetTicksPerSecond == 300,
                "targets independent, not forced equal pulse counts");
        require(t.motor1.pwm == 40 && t.motor2.pwm == 20, "feedforward initially keeps each command");
        c.update(1010,4,-3,40,20); require(c.telemetry().samples == 1, "measurement period enforced");
        c.update(1020,8,-6,40,20); t=c.telemetry();
        require(t.motor1.ticksPerSecond == 400 && t.motor2.ticksPerSecond == 300 &&
                t.motor1.pwm == 40 && t.motor2.pwm == 20, "signed actual deltas and actual dt");
        require(c.update(1040,12,-12,40,20), "one wheel under speed"); t=c.telemetry();
        require(t.motor1.pwm > 40 && t.motor2.pwm == 20, "only slow wheel gets speed correction");
        c.update(1060,12,-12,0,0);t=c.telemetry();
        require(t.motor1.pwm==0 && t.motor2.pwm==0 && t.motor1.integral==0 && t.motor2.integral==0,
                "zero targets clear output and integrator");
        c.update(1080,12,-12,-20,-30);c.update(1100,8,-3,-20,-30);t=c.telemetry();
        require(t.motor1.pwm<0 && t.motor2.pwm<0 && t.motor1.ticksPerSecond<0 && t.motor2.ticksPerSecond<0,
                "reverse motion respects independent polarity");
        c.stop();require(!c.ready() && c.telemetry().motor1.pwm==0, "stop removes both outputs");
        c.begin(s,0,0,0);c.update(0,0,0,30,30);
        require(!c.update(300,0,0,30,30) && c.telemetry().status==WheelSpeedStatus::FeedbackLate &&
                c.telemetry().motor2.pwm==0, "late feedback stops both wheels");
        require(!c.update(320,6,-9,30,30), "fault stays latched until explicit restart");
        c.begin(s,0,0,0);c.update(0,0,0,30,30);
        for(uint32_t now=20;now<=1000;now+=20) c.update(now,0,-int32_t(now*450/1000),30,30);
        require(c.telemetry().status==WheelSpeedStatus::EncoderStalled && c.telemetry().motor1.pwm==0 &&
                c.telemetry().motor2.pwm==0,"unplugged/stuck encoder stops both outputs");
        c.begin(s,0,0,0);c.update(0,0,0,30,30);
        for(uint32_t now=20;now<=300;now+=20) c.update(now,-int32_t(now/2),-int32_t(now/2),30,30);
        require(c.telemetry().status==WheelSpeedStatus::EncoderDirection, "incorrect polarity faults instead of accelerating forever");
        c.begin(s,0,0,0);c.update(0,0,0,30,30);
        require(!c.update(20,0,0,std::numeric_limits<float>::quiet_NaN(),30) && c.telemetry().motor2.pwm==0,
                "nonfinite command stops both");
        s.motor1.pwmLimit=50;c.begin(s,0,0,0);c.update(0,0,0,100,20);
        for(uint32_t now=20;now<=400;now+=20) c.update(now,int32_t(now/2),-int32_t(now/4),100,20);
        t=c.telemetry();require(t.motor1.pwm==50 && t.motor1.integral==0,"saturation prevents integral windup");
        c.update(420,210,-105,10,20);require(c.telemetry().motor1.pwm<50,"smaller target escapes saturation");
        s=fixture();const uint32_t late=0xfffffff0u;
        c.begin(s,2147483640,0,late);c.update(late,2147483640,0,30,30);
        require(c.update(late+20,int32_t(-2147483646),-9,30,30),"count and millis wrap remain valid");
        require(c.telemetry().motor1.ticksPerSecond==500,"wrapped signed delta is correct");
        auto bad=s;bad.motor2.encoder=1;require(!c.begin(bad,0,0,0),"cannot attach two wheel loops to one encoder");
        bad=s;bad.motor1.encoderSign=0;require(!c.begin(bad,0,0,0),"unknown polarity is not guessed");
        bad=s;bad.motor2.referenceTicksPerSecond=0;require(!c.begin(bad,0,0,0),"unknown target conversion is not guessed");

        // A simulated load halves M1's speed gain. Check tracking over a settled window,
        // not exact internal arithmetic; M2 has a different count scale throughout.
        c.begin(s,0,0,0);double count1=0,count2=0; c.update(0,0,0,30,30);
        double start1=0,start2=0;
        for(uint32_t now=20;now<=12000;now+=20) {
            const auto old=c.telemetry();
            const double gain=now<2000 ? 10.0 : 5.0;
            count1 += old.motor1.pwm*gain*.020;
            count2 -= old.motor2.pwm*15.0*.020;
            require(c.update(now,int32_t(count1),int32_t(count2),30,30),"healthy plant must keep running");
            if(now==10000) {start1=count1;start2=count2;}
        }
        const double rate1=(count1-start1)/2,rate2=-(count2-start2)/2;
        require(fabs(rate1-300)<30 && fabs(rate2-450)<30,"independent loops recover their targets after simulated load");
        require(fabs(rate1-300)<fabs(150-300),"feedback improves over 30-percent open-loop under that simulated load");
        std::cout<<"PASS wheel PI: independent targets, signed rates, reversal, zero/stop, faults, saturation, wrap, required measurements; simulated loaded rates="<<rate1<<","<<rate2<<" ticks/s (fixture only)\n";
    } catch(const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
