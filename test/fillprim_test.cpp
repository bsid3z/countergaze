// SquachWatch-CYD — the fill primitives write what drawPixel would have.
//
// What this guards: TFT_eSprite overrides fillSpan() and fillColumn() to
// write runs straight into the buffer instead of taking a virtual drawPixel
// call per pixel. That is worth real time on hardware -- a full-screen fill
// through the per-pixel path measured 225 ms on the ESP32-S3 -- and it is
// worth exactly nothing if the clipping drifts.
//
// The claim those overrides make is precise: every branch in them matches a
// branch in drawPixel, so a pixel they write is a pixel drawPixel would have
// written, in the same colour. This checks that claim the only way that
// means anything -- by drawing the same thing twice into two sprites, once
// through the fast path and once through drawPixel itself, and comparing the
// buffers byte for byte.
//
// The reference is drawPixel rather than a second copy of the fast path's
// own arithmetic, because a test that repeats the code's reasoning only
// proves the file was saved.
//
// This test did not exist when those overrides were written, and it should
// have. CI passed the whole time: shim_fidelity_test covers glyph cells and
// ellipse radii and never calls a fill primitive at all, so green meant
// "untested", not "correct".
#include "test_util.h"
#include <TFT_eSPI.h>
#include <cstdint>
#include <vector>

static const int W = 64, H = 48;

// Two sprites the same size: one drawn through the fast path, one through
// drawPixel. Fresh for every case, so nothing leaks between them.
struct Pair {
    TFT_eSPI    parent;
    TFT_eSprite fast, ref;
    Pair(uint8_t depth) : parent(W, H), fast(&parent), ref(&parent) {
        fast.setColorDepth(depth);
        ref.setColorDepth(depth);
        fast.createSprite(W, H);
        ref.createSprite(W, H);
    }
    // A viewport, applied identically to both. Never at (0,0) with a
    // different size -- the sprite reads that as resizeInPlace()'s landing
    // point and reallocates instead of clipping.
    void viewport(int32_t x, int32_t y, int32_t w, int32_t h, bool datum) {
        fast.setViewport(x, y, w, h, datum);
        ref.setViewport(x, y, w, h, datum);
    }
    bool identical() const { return fast.pixelsRGB565() == ref.pixelsRGB565(); }
};

static void hline(Pair& p, int32_t x, int32_t y, int32_t w, uint16_t c) {
    p.fast.drawFastHLine(x, y, w, c);                    // -> fillSpan
    for (int32_t i = 0; i < w; i++) p.ref.drawPixel(x + i, y, c);
}
static void vline(Pair& p, int32_t x, int32_t y, int32_t h, uint16_t c) {
    p.fast.drawFastVLine(x, y, h, c);                    // -> fillColumn
    for (int32_t i = 0; i < h; i++) p.ref.drawPixel(x, y + i, c);
}
static void rect(Pair& p, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) {
    p.fast.fillRect(x, y, w, h, c);                      // -> fillSpan per row
    for (int32_t j = 0; j < h; j++)
        for (int32_t i = 0; i < w; i++) p.ref.drawPixel(x + i, y + j, c);
}

// Every case is run at both depths. 8bpp is not a formality: the sprite
// quantises to RGB332 on the way in, and the fast paths hoist that
// conversion out of the loop. A fill that skipped it would look right in
// 16-bit and land as a different colour on the device.
static void bothDepths(const char* what, void (*body)(Pair&)) {
    bool ok = true;
    for (uint8_t d : {(uint8_t)16, (uint8_t)8}) {
        Pair p(d);
        body(p);
        if (!p.identical()) ok = false;
    }
    ck(what, ok);
}

int main() {
    suite("Horizontal runs land where drawPixel would put them");
    bothDepths("a plain run inside the sprite",
               [](Pair& p){ hline(p, 8, 10, 40, 0xF81F); });
    bothDepths("a run starting left of zero",
               [](Pair& p){ hline(p, -12, 5, 40, 0x07E0); });
    bothDepths("a run ending past the right edge",
               [](Pair& p){ hline(p, W - 6, 7, 40, 0x001F); });
    bothDepths("a run wider than the whole sprite",
               [](Pair& p){ hline(p, -20, 9, W + 60, 0xFFE0); });
    bothDepths("a run entirely off the left",
               [](Pair& p){ hline(p, -80, 11, 40, 0xF800); });
    bothDepths("a run entirely off the right",
               [](Pair& p){ hline(p, W + 10, 13, 40, 0xF800); });
    bothDepths("a run on a row above the sprite",
               [](Pair& p){ hline(p, 4, -3, 30, 0xF800); });
    bothDepths("a run on a row below the sprite",
               [](Pair& p){ hline(p, 4, H + 3, 30, 0xF800); });
    bothDepths("zero width draws nothing",
               [](Pair& p){ hline(p, 10, 10, 0, 0xF800); });
    bothDepths("negative width draws nothing",
               [](Pair& p){ hline(p, 10, 10, -9, 0xF800); });
    bothDepths("a single pixel",
               [](Pair& p){ hline(p, 31, 21, 1, 0x07FF); });

    suite("Vertical runs land where drawPixel would put them");
    bothDepths("a plain column inside the sprite",
               [](Pair& p){ vline(p, 20, 6, 30, 0xF81F); });
    bothDepths("a column starting above zero",
               [](Pair& p){ vline(p, 21, -10, 30, 0x07E0); });
    bothDepths("a column ending past the bottom",
               [](Pair& p){ vline(p, 22, H - 5, 30, 0x001F); });
    bothDepths("a column taller than the sprite",
               [](Pair& p){ vline(p, 23, -15, H + 40, 0xFFE0); });
    bothDepths("a column entirely above",
               [](Pair& p){ vline(p, 24, -60, 30, 0xF800); });
    bothDepths("a column entirely below",
               [](Pair& p){ vline(p, 25, H + 9, 30, 0xF800); });
    bothDepths("a column on a x left of the sprite",
               [](Pair& p){ vline(p, -4, 3, 20, 0xF800); });
    bothDepths("a column on a x right of the sprite",
               [](Pair& p){ vline(p, W + 4, 3, 20, 0xF800); });
    bothDepths("zero height draws nothing",
               [](Pair& p){ vline(p, 10, 10, 0, 0xF800); });
    bothDepths("negative height draws nothing",
               [](Pair& p){ vline(p, 10, 10, -9, 0xF800); });
    bothDepths("a single pixel",
               [](Pair& p){ vline(p, 40, 12, 1, 0x07FF); });

    suite("Filled rectangles, which are runs stacked up");
    bothDepths("a rectangle inside the sprite",
               [](Pair& p){ rect(p, 5, 5, 30, 20, 0xF81F); });
    bothDepths("a rectangle hanging off every edge",
               [](Pair& p){ rect(p, -8, -6, W + 20, H + 20, 0x07E0); });
    bothDepths("a rectangle entirely outside",
               [](Pair& p){ rect(p, W + 5, H + 5, 10, 10, 0xF800); });
    bothDepths("the whole sprite",
               [](Pair& p){ rect(p, 0, 0, W, H, 0x4208); });

    suite("A viewport clips the fast paths the same way");
    bothDepths("h-run inside a datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  hline(p, 2, 3, 20, 0xF81F); });
    bothDepths("h-run overflowing a datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  hline(p, -6, 3, 90, 0x07E0); });
    bothDepths("h-run past a datum viewport's bottom",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  hline(p, 2, 40, 20, 0x001F); });
    bothDepths("h-run inside a non-datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, false); hline(p, 8, 10, 20, 0xFFE0); });
    bothDepths("h-run starting left of a non-datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, false); hline(p, 0, 10, 60, 0xF800); });
    bothDepths("v-run inside a datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  vline(p, 3, 2, 20, 0xF81F); });
    bothDepths("v-run overflowing a datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  vline(p, 3, -8, 90, 0x07E0); });
    bothDepths("v-run past a datum viewport's right edge",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  vline(p, 50, 2, 20, 0x001F); });
    bothDepths("v-run inside a non-datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, false); vline(p, 10, 8, 20, 0xFFE0); });
    bothDepths("v-run starting above a non-datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, false); vline(p, 10, 0, 60, 0xF800); });
    bothDepths("rectangle clipped by a datum viewport",
               [](Pair& p){ p.viewport(5, 4, 40, 30, true);  rect(p, -4, -4, 80, 80, 0x4208); });

    suite("Every colour survives the run, at both depths");
    {
        // A run writes its colour once and repeats it; drawPixel converts
        // per pixel. At 8bpp those are the same only if the run quantises
        // too. Walk the whole 16-bit space coarsely and both depths fully.
        bool ok = true;
        for (uint32_t c = 0; c < 0x10000u && ok; c += 97) {
            for (uint8_t d : {(uint8_t)16, (uint8_t)8}) {
                Pair p(d);
                hline(p, 3, 4, 30, (uint16_t)c);
                vline(p, 9, 2, 20, (uint16_t)c);
                if (!p.identical()) { ok = false; break; }
            }
        }
        ck("675 colours, horizontal and vertical, 8bpp and 16bpp", ok);
    }

    return report();
}
