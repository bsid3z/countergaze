// SquachWatch-CYD — battery level. See battery.h for why only one board has one.
#include "battery.h"

// Guition JC3248W535EN only. See battery.h.
//
// READ THIS BEFORE CHANGING HOW THE PIN IS TOUCHED.
//
// A first attempt used analogSetPinAttenuation() + analogReadMilliVolts(),
// and the board then would not run on battery at all: it was fine on USB,
// nothing crashed, nothing logged, it simply died when USB came out.
// Removing the read brought battery power back. Whether that was the
// attenuation call, or GPIO5 being something other than a divider, was not
// established -- but it happened, and it cost a debugging session.
//
// So this now does EXACTLY what a working third-party firmware for this
// board does, and nothing more:
//
//   pinMode(PIN, INPUT)   once, at setup
//   analogRead(PIN)       plain, averaged over 16 samples
//   volts = (raw / 4095.0) * 3.3 * 2.0
//
// No analogSetPinAttenuation. No analogReadMilliVolts. Those are better
// calls in general -- the ADC is non-linear and analogReadMilliVolts
// applies the chip calibration for it -- and they are not used here,
// because "better" is worth less than "known to run on this board".
//
// If the board stops surviving USB removal again, this is the first thing
// to suspect, and the recovery is: flash with SQW_BATTERY_READ 0 below,
// then press RESET. The reset matters -- esptool's own does not land on
// this board, so without it the old image keeps running.
#define SQW_BATTERY_READ 1

#if SQW_BATTERY_READ && defined(JC3248)

#include <Arduino.h>

namespace Battery {
namespace {

const int      PIN        = 5;
const float    DIVIDER    = 2.0f;
const float    ADC_REF    = 3.3f;    // 11 dB attenuation, measured rather than nominal
const int      SAMPLES    = 16;

// What a single lithium cell can actually be. Outside this, something is
// not a battery: an absent one reads near zero.
const float    SANE_MIN   = 2.60f;
const float    SANE_MAX   = 4.45f;
const float    CHARGING_V = 4.25f;

// Consecutive in-range readings before believing there is a battery, and
// consecutive out-of-range ones before giving up on it. Asymmetric on
// purpose: appearing is a claim, disappearing is a retraction.
const uint8_t  CONFIRM    = 3;

const uint32_t SAMPLE_MS  = 2000;
const float    SMOOTH     = 0.25f;

bool     s_begun   = false;
bool     s_present = false;
uint8_t  s_good    = 0, s_bad = 0;
float    s_volts   = 0.0f;
uint32_t s_lastMs  = 0;

// One cell's discharge curve, volts at each 10% from empty to full. A
// straight 3.0-4.2 map reads nearly full for most of the discharge and
// then collapses; this descends at something like the right rate.
const float CURVE[11] = {
    3.20f, 3.55f, 3.65f, 3.70f, 3.74f, 3.78f,
    3.83f, 3.89f, 3.96f, 4.06f, 4.20f
};

void sample() {
    // Averaged, because a single ESP32 ADC conversion is noisy enough to
    // move the percentage around on its own. No delay() between samples --
    // this runs inside the frame loop, not in setup like the firmware this
    // is copied from, and 16 sleeps of 5 ms would cost a frame and a half.
    uint32_t total = 0;
    for (int i = 0; i < SAMPLES; i++) total += (uint32_t)analogRead(PIN);
    const uint32_t raw = total / SAMPLES;
    const float    v   = (raw / 4095.0f) * ADC_REF * DIVIDER;

    static bool first = true;
    if (first) {
        first = false;
        Serial.printf("[batt] GPIO%d first read: raw %lu -> %.2f V at the cell\n",
                      PIN, (unsigned long)raw, v);
    }

    const bool sane = (v > SANE_MIN && v < SANE_MAX);
    if (sane) { if (s_good < CONFIRM) s_good++; s_bad = 0; }
    else      { if (s_bad  < CONFIRM) s_bad++;  s_good = 0; }

    if (!s_present && s_good >= CONFIRM) {
        s_present = true; s_volts = v;
        Serial.printf("[batt] cell detected: %.2f V\n", v);
    } else if (s_present && s_bad >= CONFIRM) {
        s_present = false; s_volts = 0.0f;
    } else if (s_present && sane) {
        s_volts += (v - s_volts) * SMOOTH;
    }
}

void tick() {
    begin();
    const uint32_t now = millis();
    if (s_lastMs != 0 && (now - s_lastMs) < SAMPLE_MS) return;
    s_lastMs = now ? now : 1;
    sample();
}

}  // namespace

void begin() {
    if (s_begun) return;
    s_begun = true;
    pinMode(PIN, INPUT);   // and nothing else -- see the note at the top
}

bool present()  { tick(); return s_present; }
float volts()   { tick(); return s_present ? s_volts : 0.0f; }
bool charging() { tick(); return s_present && s_volts >= CHARGING_V; }

uint8_t percent() {
    tick();
    if (!s_present) return 0;
    const float v = s_volts;
    if (v <= CURVE[0])  return 0;
    if (v >= CURVE[10]) return 100;
    for (int i = 0; i < 10; i++) {
        if (v < CURVE[i + 1]) {
            const float span = CURVE[i + 1] - CURVE[i];
            const float frac = span > 0.0f ? (v - CURVE[i]) / span : 0.0f;
            const int   pct  = (int)(i * 10 + frac * 10.0f + 0.5f);
            return (uint8_t)(pct < 0 ? 0 : (pct > 100 ? 100 : pct));
        }
    }
    return 100;
}

}  // namespace Battery

#else   // every other board: no battery input exists, so nothing is claimed

namespace Battery {
    void begin() {}
    bool present() { return false; }
    float volts() { return 0.0f; }
    uint8_t percent() { return 0; }
    bool charging() { return false; }
}

#endif  // JC3248
