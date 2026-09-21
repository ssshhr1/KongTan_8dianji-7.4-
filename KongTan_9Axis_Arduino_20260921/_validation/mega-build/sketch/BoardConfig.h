#line 1 "C:\\Users\\Lenovo\\Desktop\\KongTan_8dianji\\KongTan_9Axis_Arduino_20260921\\firmware\\FeedAxis\\BoardConfig.h"
#pragma once
// Mega 2560 only. Replace -1 after POWER-OFF continuity checks of actual wiring.
// These are NOT inferred from previous photos. No pulses with default settings.
constexpr bool WIRING_CONFIRMED = false;
constexpr int STEP_PIN = -1;
constexpr int DIR_PIN = -1;
constexpr int ENABLE_PIN = -1;
constexpr int STEP_ACTIVE_LEVEL = HIGH;
constexpr int DIR_POSITIVE_LEVEL = HIGH;
constexpr int ENABLE_ACTIVE_LEVEL = LOW;
// Candidate timing, must be checked against the actual DM542 variant/interface.
constexpr unsigned PULSE_US = 10;
constexpr unsigned DIR_SETUP_US = 10;
constexpr bool PINS_VALID = STEP_PIN>=2 && STEP_PIN<=53 && DIR_PIN>=2 && DIR_PIN<=53 &&
    ENABLE_PIN>=2 && ENABLE_PIN<=53 && STEP_PIN!=DIR_PIN && STEP_PIN!=ENABLE_PIN && DIR_PIN!=ENABLE_PIN;
static_assert(!WIRING_CONFIRMED || PINS_VALID, "Confirm three distinct Mega digital pins (2..53)");
