#include <iostream>
#include <stdexcept>
#include <array>
#include <Arduino.h>
#include <Adafruit_ST7735.h>
int Sensor_mode_F = 0, Sensor_mode_B = 0, PID_NumPin = 3, PID_NumPin_B = 3;
int PID_SetupPin[16] = {}, PID_SetupPin_B[16] = {};
int PID_Min[16] = {}, PID_Max[16] = {}, PID_Min_B[16] = {}, PID_Max_B[16] = {};
int Front_color = 0, Back_color = 0, _Sensitive_F = 80, _Sensitive_B = 80;
float previous_error = 0, previous_error_B = 0;
uint32_t _lastPosition = 0, _lastPosition_B = 0;
bool first_state_for_calribrate = false, first_state_for_calribrate_B = false;
uint16_t state_on_Line = 0, state_on_Line_B = 0;
int swpin = 27, stops = 0;
Adafruit_ST7735 tft_;
void ao() { ++stops; }
namespace PuppyBotAdcDetail {
    int readMuxFront(int channel, int) { return (fakeMillis < 1000 ? 800 : 100) + channel; }
    int readMuxBack8(int channel) { return (fakeMillis < 3600 ? 700 : 150) + channel; }
}
#include "PuppyBotLineCalibrationMenu.h"
int sensor[8] = {}, sweepRead = 0, sweepFirst = -1;
int ADC_F(int channel) { return sweepFirst < 0 ? sensor[channel] : (++sweepRead <= 8 ? sweepFirst : 100); }
int ADC_B(int channel) { return ADC_F(channel); }
constexpr int ST77XX_BLACK = 0, ST77XX_RED = 1, ST77XX_GREEN = 2, ST77XX_YELLOW = 3;
void buzzer(int, int) {}
void printText(int, int, const char*, int, int, int) {}
#include "ActualLineReaders.h"
using namespace PuppyBotLineDetail;
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
Record example(bool reverse = false) {
    Record r;
    for (unsigned i = 0; i < channels; ++i) {
        r.floor[i] = {captureSamples, 800, 798, 802};
        r.line[i] = {captureSamples, 100, 98, 102};
        if (reverse) std::swap(r.floor[i], r.line[i]);
    }
    return r;
}
void collect(Session &s, int base) {
    require(s.select() == Action::None && s.collecting, "capture starts only on selection");
    int raw[8];
    for (unsigned n = 0; n < captureSamples; ++n) {
        for (int i = 0; i < 8; ++i) raw[i] = base + i + int(n % 3);
        s.sample(raw);
    }
    require(!s.collecting, "capture finishes after fresh samples");
}
std::vector<std::pair<uint32_t,bool>> clicks;
bool button(bool select) {
    if (fakeMillis > 6000) throw std::runtime_error("menu failed to exit");
    for (const auto &c : clicks) if (c.second == select && fakeMillis >= c.first && fakeMillis < c.first+60) return true;
    return false;
}
bool boot() { return button(false); }
int sw(int) { return button(true) ? LOW : HIGH; }
int main(int argc, char **argv) {
    try {
        require(argc == 2, "output path");
        const std::string out = argv[1];
        auto record = example();
        auto old = record;
        for (unsigned i=0;i<channels;++i) old.floor[i].count=old.line[i].count=32;
        uint8_t legacy[encodedSize]; encode(old,LineSide::Front,legacy); Record decodedOld;
        require(decode(legacy,LineSide::Front,decodedOld), "saved 32-sample calibration remains compatible");
        require(record.valid(), "separated surfaces pass");
        require(record.normalized(0,800)==0 && record.normalized(0,100)==100 && record.normalized(0,450)==50,
                "floor zero, line hundred, middle proportional");
        require(record.normalized(0,2000)==0 && record.normalized(0,0)==100, "normalization clamps");
        auto bad = record; bad.line[2] = bad.floor[2]; require(!bad.valid(), "identical surfaces rejected");
        bad = record; bad.line[3].count = 31; require(!bad.valid(), "missing sample rejected");
        bad = record; bad.floor[4].low = 200; require(!bad.valid(), "moving across mixed surfaces rejected");
        bad = record; bad.floor[5].low = 801; require(!bad.valid(), "invalid statistic rejected");
        auto reverse = example(true); require(reverse.normalized(0,800)==100, "reversed polarity supported");

        Session session; session.select(); session.select();
        collect(session,800); require(session.surface==1 && session.stage==Stage::Capture, "floor then line");
        collect(session,100); require(session.stage==Stage::Review && session.draft.valid(), "all-channel complete");
        require(session.select()==Action::Save, "explicit save only after validation");
        session.next(); session.select(); require(!session.draft.valid() && session.surface==0, "retry clears draft");
        session.select(); int raw[8]={800,800,800,800,800,800,800,800}; session.sample(raw); session.select();
        require(session.collecting && session.samples==1, "repeat SW1 cannot cancel or restart capture");
        for (unsigned n=1;n<captureSamples;++n) session.sample(raw);
        require(session.surface==1 && session.draft.floor[0].count==captureSamples, "repeated SW1 capture still completes");
        session.next(); session.select(); require(session.stage==Stage::Side, "Exit works during uncompleted wizard");
        session.selection=1; session.select(); session.next(); session.select();
        for (unsigned s=0;s<2;++s) for (unsigned i=0;i<8;++i) {
            require(session.channel==i && session.surface==s, "one-by-one channel sequence");
            collect(session,s ? 150 : 700);
        }
        require(session.draft.valid() && session.side==LineSide::Back, "all 16 individual captures required");

        EEPROM.begin(4096);
        for (int i=0;i<4096;++i) EEPROM.write(i,uint8_t(i));
        EEPROM.commit(); auto untouched = EEPROM.flash;
        require(save(EEPROM,LineSide::Front,record), "front save");
        require(save(EEPROM,LineSide::Back,reverse), "back save");
        for (unsigned i=0;i<4096;++i)
            if (!(i>=storageOffset && i<storageOffset+encodedSize) &&
                !(i>=storageOffset+slotSize && i<storageOffset+slotSize+encodedSize))
                require(EEPROM.flash[i]==untouched[i], "RGB and all unrelated bytes preserved");
        EEPROM.begin(4096); Record restored;
        require(load(EEPROM,LineSide::Front,restored) && restored.floor[0].mean==800, "front survives restart");
        require(load(EEPROM,LineSide::Back,restored) && restored.floor[0].mean==100, "back independent");
        auto saved = EEPROM.flash; EEPROM.fail = true;
        require(!save(EEPROM,LineSide::Front,reverse) && EEPROM.flash==saved && EEPROM.buffer==saved, "failed save rolls back buffer");
        EEPROM.fail=false;
        EEPROM.buffer[storageOffset+20]^=1;
        require(!load(EEPROM,LineSide::Front,restored) && restored.floor[0].mean==100, "corruption does not replace active data");
        EEPROM.begin(4096);
        uint8_t bytes[encodedSize]; encode(record,LineSide::Front,bytes);
        require(!decode(bytes,LineSide::Back,restored), "wrong bank rejected");
        bytes[6]=16; require(!decode(bytes,LineSide::Front,restored), "wrong count rejected");
        encode(record,LineSide::Front,bytes); bytes[4]=9; require(!decode(bytes,LineSide::Front,restored), "unknown schema rejected");

        require(loadLineCalibration(LineSide::Front) && loadLineCalibration(LineSide::Back), "public load applies both banks");
        require(Sensor_mode_F==1 && Sensor_mode_B==1 && PID_NumPin==8 && PID_NumPin_B==8, "loaded MUX configuration matches captured bank");
        for(int i=0;i<8;++i) sensor[i]=800;
        sensor[3]=100; require(readline()==300 && Read_sumValue_sensor()==100 && Read_status_sensor(3), "actual front reader uses captured references");
        for(int i=0;i<8;++i) sensor[i]=100;
        sensor[5]=800; require(readline_B()==500 && Read_sumValue_sensor_B()==100 && Read_status_sensor_B(5), "actual back reader uses independent reverse polarity");
        for(int i=0;i<8;++i) sensor[i]=450;
        require(readline()==350 && readline_B()==350, "symmetric centroid centered");
        _Sensitive_F=100; for(int i=0;i<8;++i) sensor[i]=790;
        require(readline()>=0, "weak sum does not divide by zero");
        loaded[0]=false; for(int i=0;i<8;++i) PID_Min[i]=PID_Max[i]=100;
        require(readline()>=0 && Read_sumValue_sensor()==0, "equal legacy endpoints do not divide by zero");
        for(bool back : {false,true}) for(int edge : {0,1023}) {
            first_state_for_calribrate=first_state_for_calribrate_B=false;
            sweepRead=0;sweepFirst=edge;
            if(back) setCalibrate_B(2); else setCalibrate(2);
            const int lo=(back?PID_Min_B:PID_Min)[0], hi=(back?PID_Max_B:PID_Max)[0];
            require(lo==std::min(edge,100) && hi==std::max(edge,100), "legacy collector preserves actual extrema");
        }
        sweepFirst=-1;

        for(bool portrait : {false,true}) {
            Adafruit_ST7735 display(portrait?128:160,portrait?160:128);
            PuppyBotLineDetail::Screen<Adafruit_ST7735> screen(display); screen.begin();
            Session state;
            for(int stage=0;stage<4;++stage) {
                state.stage=Stage(stage); state.draft=example();
                const char *notices[]={"Choose sensor bank", "One surface at a time", "Place on FLOOR", "Ready to Save"};
                state.notice=notices[stage];
                for(unsigned choice=0;choice<state.itemCount();++choice) {
                    state.selection=choice; screen.show(state,raw);
                    display.operations.clear(); screen.show(state,raw);
                    require(display.operations.empty(), "unchanged TFT has no redraw");
                    Adafruit_ST7735 fresh(display.width(),display.height());
                    PuppyBotLineDetail::Screen<Adafruit_ST7735> reference(fresh); reference.begin(); reference.show(state,raw);
                    require(display.pixels==fresh.pixels, "partial redraw equals clean screen across stage changes");
                }
                display.save(out+(portrait?"/portrait-":"/landscape-")+std::to_string(stage)+".ppm");
            }
            const int clears=display.fullClears; state.draft.line[0]=state.draft.floor[0]; screen.show(state,raw);
            require(display.fullClears==clears,"changed status never clears full screen");
        }

        clicks={{100,true},{300,true},{500,true},{1100,true},{1700,true},{1900,false},{2100,false},{2300,true},
                {2500,false},{2700,true},{2900,true},{3100,true},{3700,true},{4300,true},
                {4500,false},{4700,false},{4900,true},{5100,false},{5300,true}};
        fakeMillis=0;fakeBootHook=boot;fakeDigitalReadHook=sw;
        const int commits=EEPROM.commits;
        runMenu();
        fakeBootHook=nullptr;fakeDigitalReadHook=nullptr;
        require(EEPROM.commits==commits+2 && stops>=3,"actual menu commits only two Saves and stops motors");
        require(active[0].floor[0].mean==800 && active[0].line[0].mean==100 &&
                active[1].floor[0].mean==700 && active[1].line[0].mean==150,"actual BOOT/SW1 wizard saves and applies both banks");
        std::cout<<"PASS captures/all/single/repeat-click/retry; noise/polarity/invalid data; independent flash/rollback/RGB preservation; actual line readers/extrema; TFT bounds/partial redraw; BOOT/SW1 full journey\n";
        return 0;
    } catch(const std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n';return 1; }
}
