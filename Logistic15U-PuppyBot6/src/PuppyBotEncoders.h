#pragma once
#include <pio_encoder.h>

// Keep the original namespace and pin assignments for API compatibility.
namespace PuppyBotIMU {
    static PioEncoder encoder_PIO_M1(17);
    static PioEncoder encoder_PIO_M2(19);
    static int state_begin_Encoder_M1 = 0;
    static int state_begin_Encoder_M2 = 0;

    void init_encoder(int position_motor) {
        if (position_motor == 1 && state_begin_Encoder_M1 == 0) {
            // The driver configures only this encoder's A/B pins as inputs.
            encoder_PIO_M1.begin();
            state_begin_Encoder_M1 = 1;
        } else if (position_motor == 2 && state_begin_Encoder_M2 == 0) {
            encoder_PIO_M2.begin();
            state_begin_Encoder_M2 = 1;
        }
    }

    void init_encoders() {
        init_encoder(1);
        init_encoder(2);
    }

    int get_pulse_Encoder(int position_motor) {
        init_encoder(position_motor);
        if (position_motor == 1) return encoder_PIO_M1.getCount();
        if (position_motor == 2) return encoder_PIO_M2.getCount();
        return 0;
    }

    void resetEncoder(int position_motor) {
        init_encoder(position_motor);
        if (position_motor == 1) encoder_PIO_M1.reset();
        else if (position_motor == 2) encoder_PIO_M2.reset();
    }
}
