#include <iostream>
#include <climits>
#include <cmath>
#include "PuppyBotServos.h"

void require(bool pass, const char *why) { if (!pass) throw std::runtime_error(why); }
double pulseUs(int channel) {
    const uint pin = servoPins[channel], slice = pwm_gpio_to_slice_num(pin);
    return levels[slice][pin & 1] * double(configurations[slice].divider) * 1000000 / testClockHz;
}
void checkAngle(int channel, int angle) {
    require(getServoLastAngle(channel) == angle, "last angle differs from requested endpoint");
    require(std::abs(pulseUs(channel) - (300 + 15 * angle)) <= 2, "pulse duration not derived from system clock");
}
int main(int argc, char **argv) {
    try {
        if (argc > 1) testClockHz = std::stoul(argv[1]);
        require(getServoLastAngle(0) == -1 && !isServoActive(0), "no invented startup angle");
        require(!servoRun(-1,90) && !servoRun(6,90) && !servoRun(0,-1) && !servoRun(0,181), "reject invalid channel and angles");
        require(!servoSweep(0,0,10,0,0) && !servoSweep(0,0,10,-1,0) && !servoSweep(0,0,10,1,-1), "reject invalid steps and delays");
        require(muxCalls == 0 && writes.empty(), "invalid requests have no hardware side effects");
        require(setupServos(), "setup succeeds");
        for (uint slice : {9u,10u,11u}) {
            require(initCalls[slice] == 1 && enabledCalls[slice] == 1, "shared slices initialized once");
            require(levels[slice][0] == 0 && levels[slice][1] == 0, "startup has no pulses");
            const double period = (uint32_t(configurations[slice].top) + 1) * double(configurations[slice].divider) * 1000000 / testClockHz;
            require(std::abs(period - 20000) <= 2, "PWM period must be 20ms within one tick");
        }
        for (uint slice=0; slice<9; ++slice) require(initCalls[slice] == 0, "motor slices untouched");
        require(control_servo(0,0,90,0), "old default90 skipped move is fixed"); checkAngle(0,90);
        require(control_servo(1,0,90,0), "same target on second channel still sends command"); checkAngle(1,90);
        checkAngle(0,90);
        require(control_servo(0,90,10,0), "descending control works"); checkAngle(0,10); checkAngle(1,90);
        require(setupServos() && initCalls[9] == 1, "repeated setup cannot reset running neighbor"); checkAngle(1,90);
        const auto count = writes.size();
        require(!servoRun(0,INT_MIN) && !servoRun(0,INT_MAX) && !control_servo(0,-1,10,0), "extreme angles rejected");
        require(writes.size() == count, "invalid values preserve current output"); checkAngle(0,10);
        require(servoStop(0), "stop succeeds");
        require(levels[9][0] == 0 && enabled[9] && !isServoActive(0), "stop clears output without disabling shared slice");
        require(getServoLastAngle(0) == 10, "stop retains last commanded angle without claiming physical position");
        checkAngle(1,90);
        require(!servoStop(6) && !servoStop(-1), "invalid stop rejected");
        require(servoRun(0,10) && isServoActive(0), "same-angle resume after stop writes pulse again");
        for (auto args : {std::vector<int>{0,10,3}, {10,0,3}, {5,5,1}, {0,180,INT_MAX}}) {
            writes.clear(); delays.clear();
            require(servoSweep(2,args[0],args[1],args[2],7), "sweep succeeds"); checkAngle(2,args[1]);
            require(delays.size() + 1 == writes.size(), "delay occurs only between commands");
            for (auto ms:delays) require(ms==7, "requested step delay retained");
        }
        writes.clear();
        require(!setServoPulseRange(0,400,2400), "range cannot change while active");
        require(servoStop(0) && setServoPulseRange(0,400,2400), "measured range accepted while stopped");
        require(!isServoActive(0) && getServoLastAngle(0)==-1, "range change does not move and invalidates old mapping");
        // Synthetic test endpoints, not a recommendation for the robot.
        require(servoRun(0,90) && std::abs(pulseUs(0)-1400)<=2, "per-channel pulse mapping applied"); checkAngle(1,90);
        require(servoStop(0), "stop again"); const auto before = writes.size();
        for(auto limits : {std::vector<int>{0,2000}, {-1,2000}, {3000,1000}, {1000,1000}, {300,20000}, {INT_MIN,INT_MAX}})
            require(!setServoPulseRange(0,limits[0],limits[1]), "reject invalid pulse range");
        require(writes.size()==before, "invalid configuration does not generate pulses");
        require(angleToLevel(-1)==0 && angleToLevel(181)==0, "legacy helper rejects invalid angles");
        std::cout << "PASS clock=" << testClockHz << ": timing, no startup motion, independent channels, stop/resume, endpoints, invalid inputs, measured ranges and motor isolation\n";
    } catch(const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
