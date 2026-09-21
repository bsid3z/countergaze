// SquachWatch-CYD — Guition JC3248W535EN panel transport.
//
// This board is the odd one out. Every other board here hangs an SPI panel
// off TFT_eSPI; this one has an AXS15231B behind a FOUR-LANE QSPI bus, which
// TFT_eSPI cannot drive at all -- not a missing driver, a bus the library
// does not speak (Bodmer, TFT_eSPI discussion #3786). So the firmware's
// drawing does not go through TFT_eSPI here: gfx/TFT_eSPI.h (the portable
// rasterizer the web emulator already renders every screen with) fills an
// RGB565 buffer, and this file is the only thing that touches hardware --
// it takes that finished buffer and puts it on the glass.
//
// That split is the whole reason the port is tractable. The rasterizer is
// proven against ~97 files of UI code on the emulator; what was missing was
// a way out to a real panel, and this is it and nothing more.
//
// Pin map and init are NOT guesses. They come from a confirmed-working
// Arduino_GFX config for this exact board:
//
//     Arduino_ESP32QSPI(45, 47, 21, 48, 40, 39)   // CS, SCK, D0, D1, D2, D3
//     Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, 320, 480)
//
// Native orientation is 320x480 PORTRAIT. The UI reads its geometry from
// width()/height() rather than per-board constants, so portrait is a layout
// the screens already handle -- it is not the 3.5" board's 480x320, and this
// deliberately does not pretend to be that board.
#pragma once
#include <stdint.h>

namespace Jc3248Panel {

    // Native panel geometry, before any rotation.
    static const int NATIVE_W = 320;
    static const int NATIVE_H = 480;

    // Brings up the QSPI bus and the AXS15231B, then the backlight at full
    // duty. Call once from setup(), before anything draws.
    //
    // Returns false if the panel would not initialise, in which case nothing
    // else here is safe to call. There is no fallback: this is the only way
    // pixels reach this board's glass.
    bool begin();

    // Ships one finished RGB565 frame to the panel. `buf` is w*h pixels in
    // the rasterizer's own layout -- row-major, no padding, native byte
    // order -- landing with its top-left corner at (x, y).
    //
    // This is the ONLY path to the panel. The rasterizer calls it from
    // TFT_eSprite::pushSprite(), which is what every screen in this firmware
    // finishes its frame with.
    void pushFrame(const uint16_t* buf, int32_t w, int32_t h, int32_t x, int32_t y);

    // Backlight duty, 0-255. Driven on LEDC like every other board's, so
    // main.cpp's existing dimming still works -- the pin is GPIO1 here
    // rather than the CYD's 21.
    void setBacklight(uint8_t duty);

    // Panel size AFTER rotation, which is what the rasterizer's canvas is
    // sized from.
    int width();
    int height();

    // 0-3, applied to the panel. Kept because the firmware can rotate its
    // screen at runtime; the rasterizer resizes its own buffer to match.
    void setRotation(uint8_t r);
}
