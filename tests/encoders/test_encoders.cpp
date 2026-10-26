#include <iostream>
#include <stdexcept>
#include <string>
#ifndef ENCODER_HEADER
#define ENCODER_HEADER "PuppyBotEncoders.h"
#endif
#include ENCODER_HEADER
using namespace PuppyBotIMU;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
void checkBoth() {
    FakeEncoder::ticks(17, 23);
    FakeEncoder::ticks(19, -11);
    require(get_pulse_Encoder(1) == 23, "encoder 1 lost ticks");
    require(get_pulse_Encoder(2) == -11, "encoder 2 lost ticks");
}
int main(int argc, char **argv) {
    const std::string scenario = argc > 1 ? argv[1] : "";
    try {
        if (scenario == "order12" || scenario == "order21") {
            const int first = scenario == "order12" ? 1 : 2;
            init_encoder(first);
            init_encoder(3 - first);
            checkBoth();
        } else if (scenario == "init_both") {
            init_encoders();
            checkBoth();
            init_encoders();
            require(get_pulse_Encoder(1) == 23 && get_pulse_Encoder(2) == -11,
                    "init_encoders changed existing counts");
        } else if (scenario == "lazy") {
            require(get_pulse_Encoder(1) == 0, "initial count 1");
            require(get_pulse_Encoder(2) == 0, "initial count 2");
            checkBoth();
        } else if (scenario == "repeat_init") {
            init_encoder(1);
            init_encoder(2);
            FakeEncoder::ticks(17, 40);
            FakeEncoder::ticks(19, -50);
            init_encoder(1);
            init_encoder(2);
            require(get_pulse_Encoder(1) == 40 && get_pulse_Encoder(2) == -50,
                    "reinitialization changed counts");
            require(FakeEncoder::starts[17] == 1 && FakeEncoder::starts[19] == 1,
                    "driver started more than once");
        } else if (scenario == "reset_each") {
            get_pulse_Encoder(1);
            get_pulse_Encoder(2);
            checkBoth();
            resetEncoder(1);
            require(get_pulse_Encoder(1) == 0, "reset 1 failed");
            require(get_pulse_Encoder(2) == -11, "reset 1 changed encoder 2");
            FakeEncoder::ticks(17, 7);
            FakeEncoder::ticks(19, -3);
            resetEncoder(2);
            require(get_pulse_Encoder(2) == 0, "reset 2 failed");
            require(get_pulse_Encoder(1) == 7, "reset 2 changed encoder 1");
            FakeEncoder::ticks(19, 4);
            require(get_pulse_Encoder(2) == 4, "encoder 2 stopped after reset");
        } else if (scenario == "reset_before_init") {
            resetEncoder(1);
            resetEncoder(2);
            checkBoth();
        } else if (scenario == "invalid_id") {
            get_pulse_Encoder(1);
            get_pulse_Encoder(2);
            checkBoth();
            for (int id : {-1, 0, 3}) {
                init_encoder(id);
                resetEncoder(id);
                require(get_pulse_Encoder(id) == 0, "invalid id read a channel");
            }
            require(get_pulse_Encoder(1) == 23 && get_pulse_Encoder(2) == -11,
                    "invalid id changed a valid channel");
        } else {
            throw std::runtime_error("unknown scenario");
        }
        std::cout << "PASS " << scenario << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << scenario << ": " << error.what() << '\n';
        return 1;
    }
}
