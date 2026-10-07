// SquachWatch-CYD — how much battery is left, where a board can tell.
//
// Only the Guition JC3248W535EN can. It carries an IP5306 power-management
// IC and a JST 1.25mm cell connector (P5), with the battery rail divided
// 2:1 into GPIO5 so the ADC can read it. No other board here has a battery
// input at all, so on every one of them this reports "no battery" and draws
// nothing -- the title bar is unchanged on the boards that ship.
//
// THE DIVIDER PIN IS NOT FROM A DATASHEET. GPIO5 and the 2:1 ratio come
// from a working third-party firmware for this exact board, and Guition
// publish no schematic. present() is written to be believed rather than
// assumed: a reading has to sit inside a range a real lithium cell could
// actually produce, for several samples running, before this says there is
// a battery. A floating pin wanders and a missing one reads near zero, and
// neither survives that test -- so a board with no cell attached shows no
// percentage instead of an invented one.
#pragma once
#include <stdint.h>

namespace Battery {

    // Sets up the ADC. Safe to call more than once; called lazily by the
    // readers, so nothing has to be added to setup().
    void begin();

    // True once several consecutive readings have landed in the range a
    // single lithium cell can actually sit in. False on every other board,
    // on this board with no cell attached, and for the first second or so
    // after boot while the filter fills.
    bool present();

    // Battery volts at the cell, already corrected for the 2:1 divider.
    // 0 when !present().
    float volts();

    // 0-100. Not a straight line from 3.0 to 4.2: a lithium cell spends
    // most of its life between 3.7 and 4.0 V, so a linear map reads 100%
    // for an hour and then falls off a cliff. The curve below is the usual
    // discharge shape, which at least moves at a believable rate.
    // 0 when !present().
    uint8_t percent();

    // Charging, as far as we can tell -- which is not very far. The IP5306
    // does not tell the ESP32 anything, so this is inferred from the rail
    // sitting above what a cell reaches on its own. It means "probably on
    // USB", not "the charger says so".
    bool charging();
}
