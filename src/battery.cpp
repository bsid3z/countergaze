// SquachWatch-CYD — battery level. See battery.h for why only one board has one.
#include "battery.h"

#if defined(JC3248)

#include <Arduino.h>

namespace Battery {
namespace {

// The rail is divided 2:1 into this pin, so the ADC sees half the cell.
// From a working third-party firmware for this board, not from a datasheet
// -- Guition publish no schematic. See the header.
const int      PIN        = 5;
const float    DIVIDER    = 2.0f;

// What a single lithium cell can actually be. Outside this, something is
// not a battery: a floating input wanders across the whole range and an
// absent one sits near zero.
const float    SANE_MIN   = 2.60f;
const float    SANE_MAX   = 4.45f;

// Above this the rail is being held up by the charger rather than by the
// cell -- a cell on its own does not sit here for long.
const float    CHARGING_V = 4.25f;

// Consecutive in-range readings before believing there is a battery, and
// consecutive out-of-range ones before giving up on it. Asymmetric on
// purpose: appearing is a claim, disappearing is a retraction, and a
// percentage that blinks in and out is worse than none.
const uint8_t  CONFIRM    = 5;

const uint32_t SAMPLE_MS  = 2000;
const float    SMOOTH     = 0.25f;   // EMA weight for each new sample

bool     s_begun   = false;
bool     s_present = false;
uint8_t  s_good    = 0, s_bad = 0;
float    s_volts   = 0.0f;
uint32_t s_lastMs  = 0;

// One cell's discharge curve, as volts at each 10% from empty to full.
// A straight 3.0-4.2 map spends most of its time reading nearly full and
// then collapses; this at least falls at something like the right rate.
const float CURVE[11] = {
    3.20f, 3.55f, 3.65f, 3.70f, 3.74f, 3.78f,
    3.83f, 3.89f, 3.96f, 4.06f, 4.20f
};

void sample() {
    // analogReadMilliVolts applies the chip's own ADC calibration, which
    // matters here: the raw count on an ESP32 ADC is not linear and reading
    // it directly is good for about half a volt of error at the top end.
    const uint32_t mv = analogReadMilliVolts(PIN);
    const float    v  = (mv / 1000.0f) * DIVIDER;

    // The very first reading, said immediately and unconditionally. The
    // confirmed verdict below needs several samples and arrives seconds
    // later, which on this board is often after the USB console has already
    // stopped draining -- so the one number worth having is printed while
    // anything is still listening.
    static bool first = true;
    if (first) {
        first = false;
        Serial.printf("[batt] GPIO%d first read: %lu mV -> %.2f V at the cell\n",
                      PIN, (unsigned long)mv, v);
    }

    const bool sane = (v > SANE_MIN && v < SANE_MAX);
    if (sane) { if (s_good < CONFIRM) s_good++; s_bad = 0; }
    else      { if (s_bad  < CONFIRM) s_bad++;  s_good = 0; }

    if (!s_present && s_good >= CONFIRM) {
        s_present = true;  s_volts = v;
        Serial.printf("[batt] cell detected: %.2f V on GPIO%d (raw %lu mV)\n",
                      v, PIN, (unsigned long)mv);
    } else if (s_present && s_bad >= CONFIRM) {
        s_present = false; s_volts = 0.0f;
        Serial.printf("[batt] cell gone: %.2f V is outside %.2f-%.2f\n",
                      v, SANE_MIN, SANE_MAX);
    } else if (s_present && sane) {
        s_volts += (v - s_volts) * SMOOTH;
    }

    // Said once, whatever the verdict, so "why is there no percentage" has
    // an answer on the console instead of needing a meter. A pin reading
    // near zero is no cell; one wandering mid-range with nothing attached
    // is the thing this would otherwise invent a battery from.
    static bool announced = false;
    if (!announced && (s_good >= CONFIRM || s_bad >= CONFIRM)) {
        announced = true;
        Serial.printf("[batt] GPIO%d reads %lu mV -> %.2f V at the cell: %s\n",
                      PIN, (unsigned long)mv, v,
                      s_present ? "battery" : "no battery, nothing drawn");
    }
}

void tick() {
    begin();
    const uint32_t now = millis();
    // The first sample is taken immediately, so a board that has been up a
    // while does not wait two seconds to find its own battery.
    if (s_lastMs != 0 && (now - s_lastMs) < SAMPLE_MS) return;
    s_lastMs = now ? now : 1;
    sample();
}

}  // namespace

void begin() {
    if (s_begun) return;
    s_begun = true;
    // 11 dB: the full 0-~3.1 V input range. At 2:1 that covers a cell up to
    // about 6 V, so a full 4.2 V one is nowhere near the ceiling where the
    // ADC stops being linear.
    analogSetPinAttenuation(PIN, ADC_11db);
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
