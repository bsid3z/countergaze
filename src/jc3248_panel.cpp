// SquachWatch-CYD — Guition JC3248W535EN panel transport. See the header
// for why this board does not go through TFT_eSPI at all.
//
// Arduino_GFX is used ONLY as a bus, a power-on command sequence, and a
// frame-shaped way out to the panel. Everything above the wire -- every
// rectangle, glyph and sprite in this firmware -- is drawn by gfx/TFT_eSPI.h
// before a single byte gets here.
//
// THE INIT BELOW IS NOT NEGOTIABLE, and was arrived at the hard way. Three
// things about it look like detail and are not:
//
//   A frame buffer has to exist before anything can be sent. Arduino_Canvas
//   owns it, and Arduino_Canvas::begin() is what allocates it -- the panel's
//   own begin() only brings the controller up. Both are called here, in that
//   order, because the canvas has to be built already the right way round
//   (see buildCanvas) and that means knowing the rotation first.
//
//   The frame reaches the glass through flush(), which writes the WHOLE
//   buffer in one draw16bitRGBBitmap. This panel does not take partial
//   window writes. Pushing a sub-rectangle to it -- which is the obvious
//   thing to do, and what this file did first -- lights ONE COLUMN OF PIXELS
//   down the left edge and nothing else. That symptom is the signature of
//   this mistake on this controller.
//
//   Rotation is the panel's job, not the canvas's. Arduino_Canvas applies
//   rotation when a pixel is DRAWN and keeps its buffer in the panel's native
//   portrait layout, so writing into that buffer directly -- which pushFrame()
//   does, to avoid a second copy -- would mean transposing every frame by
//   hand. Measured at 181 ms. One MADCTL write instead makes the copy a
//   memcpy. See SQW_JC3248_PANEL_ROTATE.
#include "jc3248_panel.h"

#if defined(JC3248)

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

namespace {

    // Confirmed-working pin map for this board (see header).
    const int8_t QSPI_CS   = 45;
    const int8_t QSPI_SCK  = 47;
    const int8_t QSPI_D0   = 21;
    const int8_t QSPI_D1   = 48;
    const int8_t QSPI_D2   = 40;
    const int8_t QSPI_D3   = 39;

    // Backlight. GPIO1 on this board, not the CYD's 21.
    const int    BL_PIN    = 1;
    // Its own LEDC channel, clear of the three main.cpp claims for the other
    // boards' backlights (0, 1 and 2) so a shared-code ledcWrite to any of
    // those cannot fight this one.
    const int    BL_CH     = 3;
    const int    BL_FREQ   = 5000;
    const int    BL_BITS   = 8;

    // Transpose tile, in pixels. 32x32 of RGB565 is 2KB a side, which both
    // buffers can keep resident while a tile is worked.
    const int32_t TILE = 32;

    // Landscape. The panel is portrait glass; the UI has been laid out wide
    // on every board that ships, so it is turned to match.
    const uint8_t ROTATION = 1;

    Arduino_DataBus* s_bus    = nullptr;
    Arduino_GFX*     s_panel  = nullptr;
    Arduino_Canvas*  s_canvas = nullptr;
    bool             s_ready  = false;
    uint8_t          s_rot    = ROTATION;

    // OFF, and it stays off: THIS PANEL IGNORES MADCTL.
    //
    // Rotating the controller instead of the canvas would make pushFrame() a
    // straight memcpy, and it was tried: Arduino_AXS15231B::setRotation()
    // writes MX|MV for landscape, the canvas was built 480x320 to match, and
    // the push dropped from 181 ms to 48 ms. The screen went back to ONE
    // COLUMN OF PIXELS down the left edge -- the same failure as pushing a
    // sub-rectangle, and for the same underlying reason. The controller keeps
    // taking its address window in native 320x480 portrait whatever MADCTL
    // says, so a 480-wide row is nonsense to it.
    //
    // So the canvas rotates, its buffer stays in the panel's portrait layout,
    // and pushFrame() transposes into it. The cost is real and is paid in
    // tiles below rather than wished away.
    #define SQW_JC3248_PANEL_ROTATE 0

    // Builds the canvas -- and its frame buffer -- the right way round for
    // the rotation in force. The panel must already be begun.
    bool buildCanvas(uint8_t rot) {
        const bool landscape = (rot & 1) != 0;
#if SQW_JC3248_PANEL_ROTATE
        const int16_t cw = landscape ? Jc3248Panel::NATIVE_H : Jc3248Panel::NATIVE_W;
        const int16_t ch = landscape ? Jc3248Panel::NATIVE_W : Jc3248Panel::NATIVE_H;
#else
        const int16_t cw = Jc3248Panel::NATIVE_W, ch = Jc3248Panel::NATIVE_H;
#endif
        delete s_canvas;
        s_canvas = new Arduino_Canvas(cw, ch, s_panel, 0, 0, 0);
        if (!s_canvas) return false;
        // GFX_SKIP_OUTPUT_BEGIN: the panel is already up. Re-running its init
        // here would be a second power-on sequence mid-flight.
        if (!s_canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) return false;
#if !SQW_JC3248_PANEL_ROTATE
        s_canvas->setRotation(rot);
#endif
        return true;
    }
}

namespace Jc3248Panel {

bool begin() {
    if (s_ready) return true;

    s_bus = new Arduino_ESP32QSPI(QSPI_CS, QSPI_SCK, QSPI_D0, QSPI_D1, QSPI_D2, QSPI_D3);
    if (!s_bus) return false;

    // GFX_NOT_DEFINED reset: this board ties the panel's reset to the
    // module's own, so there is no GPIO to pulse.
    s_panel = new Arduino_AXS15231B(s_bus, GFX_NOT_DEFINED, 0 /*rotation*/,
                                    false /*IPS*/, NATIVE_W, NATIVE_H);
    if (!s_panel) return false;

    // Brings up the QSPI bus and runs the panel's power-on sequence. This is
    // the same call Arduino_Canvas::begin() makes internally; it is made here
    // so the canvas can be built after the rotation is known.
    if (!s_panel->begin()) return false;
#if SQW_JC3248_PANEL_ROTATE
    s_panel->setRotation(ROTATION);
#endif
    s_rot = ROTATION;

    // Allocates the 480x320x2 frame buffer -- 307KB, which comes from PSRAM
    // because that is where a block that size can come from on this board.
    if (!buildCanvas(s_rot)) return false;

    s_canvas->fillScreen(0x0000);
    s_canvas->flush();

    // Backlight last, so boot does not flash whatever the panel powered up
    // holding. On LEDC rather than a plain digitalWrite, which is the one
    // deliberate departure from the reference config: the brightness slider
    // in Settings needs a duty cycle, and GPIO1 is clear of the QSPI lanes.
    ledcSetup(BL_CH, BL_FREQ, BL_BITS);
    ledcAttachPin(BL_PIN, BL_CH);
    ledcWrite(BL_CH, 255);

    s_ready = true;
    return true;
}

void pushFrame(const uint16_t* src, int32_t w, int32_t h, int32_t x, int32_t y) {
    if (!s_ready || !src || w <= 0 || h <= 0) return;
    (void)x; (void)y;   // whole-frame only: see the header comment

    uint16_t* fb = s_canvas->getFramebuffer();
    if (!fb) return;

    const int32_t rw = s_canvas->width();    // rotated, as the UI sees it
    const int32_t rh = s_canvas->height();
    if (w != rw || h != rh) return;          // not this frame's buffer

#if SQW_JC3248_PANEL_ROTATE
    // The panel is already turned, so the canvas buffer has the same shape
    // and the same row order as the frame the firmware just drew: one copy,
    // sequential, no per-pixel arithmetic at all.
    memcpy(fb, src, (size_t)w * h * sizeof(uint16_t));
#else
    // The same index arithmetic Arduino_Canvas::writePixelPreclipped() does
    // per pixel, hoisted out of the per-pixel path.
    switch (s_canvas->getRotation()) {
    case 0:
        // Already the buffer's own layout, so the whole thing moves at once.
        memcpy(fb, src, (size_t)w * h * sizeof(uint16_t));
        break;
    case 1:
        // Tiled, not row-by-row. A transpose touches one buffer along rows
        // and the other down columns, so one side always strides -- here by
        // 640 bytes, which misses on essentially every pixel and measured
        // 181 ms a frame. Working a TILE at a time keeps both sides inside a
        // few cache lines: each tile reads T short runs of src and writes T
        // short runs of fb, and both stay hot while it does.
        for (int32_t y0 = 0; y0 < h; y0 += TILE) {
            const int32_t y1 = (y0 + TILE < h) ? y0 + TILE : h;
            for (int32_t x0 = 0; x0 < w; x0 += TILE) {
                const int32_t x1 = (x0 + TILE < w) ? x0 + TILE : w;
                for (int32_t i = x0; i < x1; i++) {
                    const uint16_t* s = src + (size_t)y0 * w + i;
                    uint16_t* d = fb + (size_t)i * rh + (rh - 1 - y0);
                    for (int32_t j = y0; j < y1; j++) { *d-- = *s; s += w; }
                }
            }
        }
        break;
    case 2:
        for (int32_t j = 0; j < h; j++) {
            const uint16_t* s = src + (size_t)j * w;
            uint16_t* d = fb + (size_t)(rh - 1 - j) * rw + (rw - 1);
            for (int32_t i = 0; i < w; i++) { *d-- = s[i]; }
        }
        break;
    default: // 3 -- the other landscape, tiled for the same reason as 1
        for (int32_t y0 = 0; y0 < h; y0 += TILE) {
            const int32_t y1 = (y0 + TILE < h) ? y0 + TILE : h;
            for (int32_t x0 = 0; x0 < w; x0 += TILE) {
                const int32_t x1 = (x0 + TILE < w) ? x0 + TILE : w;
                for (int32_t i = x0; i < x1; i++) {
                    const uint16_t* s = src + (size_t)y0 * w + i;
                    uint16_t* d = fb + (size_t)(rw - 1 - i) * rh + y0;
                    for (int32_t j = y0; j < y1; j++) { *d++ = *s; s += w; }
                }
            }
        }
        break;
    }
#endif

    s_canvas->flush();
}

void setBacklight(uint8_t duty) {
    if (!s_ready) return;
    ledcWrite(BL_CH, duty);
}

void setRotation(uint8_t r) {
    if (!s_ready) return;
    r &= 3;
    if (r == s_rot) return;
#if SQW_JC3248_PANEL_ROTATE
    // Turning between portrait and landscape changes the buffer's shape, so
    // the canvas is rebuilt. Rare -- it happens when somebody taps rotate --
    // and the alternative is carrying the larger of the two allocations and
    // the transposing copy forever.
    s_panel->setRotation(r);
    const bool reshape = ((r & 1) != (s_rot & 1));
    s_rot = r;
    if (reshape && !buildCanvas(s_rot)) { s_ready = false; return; }
#else
    s_rot = r;
    s_canvas->setRotation(r);
#endif
}

int width()  { return s_ready ? s_canvas->width()  : NATIVE_H; }
int height() { return s_ready ? s_canvas->height() : NATIVE_W; }

}  // namespace Jc3248Panel

#endif  // JC3248
