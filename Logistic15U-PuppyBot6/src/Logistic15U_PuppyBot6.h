#pragma once
#include <Arduino.h>
#if !defined(ARDUINO_ARCH_RP2040) || !defined(PICO_RP2350)
#error "Logistic15U-PuppyBot6 requires the Arduino-Pico RP2350 core."
#endif
#if PICO_RP2350A
#error "Select Logistic15U-PuppyBot6 (RP2350B), not an RP2350A board."
#endif
#include "puppybot.h"
#include "PuppyBotIMU.h"
#include "PuppyBotTurnPID.h"
#include "KB_PIDFollowHelpers.h"
