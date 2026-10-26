#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <functional>
#include "PuppyBotLinePD.h"
using std::abs;
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
template<class T,class L,class H> T constrain(T v,L low,H high) { return v<low ? T(low) : v>high ? T(high) : v; }
uint32_t tick=0;
uint32_t millis() { tick+=10; return tick; }
void delay(unsigned long ms) { tick+=ms; }
std::vector<std::pair<int,int>> pwm;
int directions[48]={}, duty[48]={};
void analogWrite(int pin,int value) {
    if (pin!=21 && pin!=22 && pin!=23 && pin!=24) throw std::runtime_error("movement touched an auxiliary motor GPIO");
    pwm.emplace_back(pin,value); duty[pin]=value;
    if(pwm.size()>10000) throw std::runtime_error("movement did not terminate");
}
void digitalWrite(int pin,int value) {
    if(pin!=21 && pin!=23) throw std::runtime_error("movement changed auxiliary motor direction");
    directions[pin]=value;
}
struct SerialFake { void println(const char*) {} } Serial;
float Con_yaw_loop1=0, offset_yaw_loop1=180;
int status_resetYaw=0, selection_Encoder=1;
int counts[3]={}; bool encoderMoves=true;
int get_pulse_Encoder(int ch) { return encoderMoves ? ++counts[ch] : counts[ch]; }
void resetEncoder(int ch) { counts[ch]=0; }
long readEncoder() { return get_pulse_Encoder(selection_Encoder); }
int PID_NumPin=8, PID_NumPin_B=8;
uint16_t state_on_Line=1,state_on_Line_B=1;
float errors=0, integral=0, derivative=0, previous_error=0, output=0;
float errors_B=0, integral_B=0, derivative_B=0, previous_error_B=0, output_B=0;
int readline() { return 350; }
int readline_B() { return 350; }
int Read_sumValue_sensor() { return 1000; }
int Read_sumValue_sensor_B() { return 1000; }
#include "ActualDrive.h"
#include "ActualTurn.h"
#include "KB_PIDFollowHelpers.h"
void require(bool pass,const char *why) { if(!pass) throw std::runtime_error(why); }
void run(const std::function<void()> &fn, bool stops=true) {
    pwm.clear(); duty[22]=duty[24]=0;
    fn();
    require(!pwm.empty(),"test did not exercise motor outputs");
    bool one=false,two=false;
    for(auto p:pwm){one|=p.first==22;two|=p.first==24;}
    require(one&&two,"drive command must reach both motors");
    if(stops) require(duty[22]==0&&duty[24]==0,"blocking drive must stop both motors");
}
int main() {
    try {
        run([]{fd(25);},false); require(directions[21]==1&&directions[23]==1,"forward signs changed");
        run([]{bk(25);},false); require(directions[21]==0&&directions[23]==0,"reverse signs changed");
        for(auto fn:{fd, bk, tl, tr, sl, sr}) run([=]{fn(25);},false);
        run([]{fd2(20,30);},false); run([]{bk2(20,30);},false);
        run([]{motor(7,25);},false); require(duty[22]==255&&duty[24]==255,"group command scaled speed twice");
        for(int state=0;state<=9;++state) run([=]{motor_control(state,25);},state>=6);
        run([]{ao();}); run([]{aoS(25);}); run([]{motorStop(0);}); run([]{motorBreak();}); run([]{motorBreak(0);});
        for(int direction:{0,1}) for(int slow:{0,1}) {
            run([=]{moveStraightPID(direction,0,25,100,0.5,0,0,slow);});
            run([=]{moveStraightPID_Encoder(direction,0,25,20,0.5,0,0,slow);});
            run([=]{MoveStraightDirection(direction,0,25,100,0.5,0,0,slow);});
            run([=]{MoveStraightDirection_Encoder(direction,0,25,20,0.5,0,0,slow);});
            run([=]{moveStraightSnapToNearest(direction,25,20,0.5,0,0,slow);});
        }
        for(int direction:{0,1}) {
            run([=]{moveStraightPID_Accel(direction,0,25,100,0.5,0,0,20,20);});
            run([=]{moveStraightPID_step(direction,0,25,0.5,0);},false);
        }
        for(int condition:{0,1,2}) for(int target:{-10,10}) {
            run([=]{turnPID(condition,target,10,30,0.5,0);});
            run([=]{turnByAngle(target,condition);});
            run([=]{turnByAngle_fix(target,condition);});
        }
        run([]{Run_PID(25,0.5,0);},false); run([]{Run_PID_B(25,0.5,0);},false);
        run([]{Run_PID_until_frontSensor(25,0.5,0,500);},false);
        run([]{Run_PID_B_until_backSensor(25,0.5,0,500);},false);
        for(int speed:{25,-25}) {
            run([=]{follow_line_by_encoder_safe(speed,0.5,0,20,1000,readEncoder);});
            long traveled=0;
            run([&]{follow_line_by_encoder_store_safe(speed,0.5,0,20,1000,true,readEncoder,&traveled);});
            require(traveled>=20,"encoder helper no longer reports travel");
            for(int extra:{KB_EXTRA_ENC,KB_EXTRA_TIME})
                run([=]{follow_line_cross_safe(speed,0.5,0,500,1,20,extra,0,1000,true,readEncoder);});
        }
        encoderMoves=false;
        run([]{follow_line_by_encoder_safe(25,0.5,0,20,100,readEncoder);});
        std::cout<<"PASS 2WD movement: actual driver GPIO limited to M1/M2 for forward/reverse, pivots/spins, all IMU PID variants, front/back line PID, encoder/crossing helpers and stops; removed APIs absent\n";
    } catch(const std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n'; return 1; }
}
