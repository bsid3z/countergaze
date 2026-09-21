// SquachWatch-CYD — capacitive touch driver.
//
// Two chips answer to this one interface, because two boards need it and
// main.cpp only ever asks "is a finger down, and where":
//
//   CST816/CST820 (JC2432W328C, RL Phantom ...S024C) — plain register
//       reads at I2C 0x15.
//   AXS15231B     (Guition JC3248W535EN)            — a command/response
//       protocol at 0x3B, where the touch controller is the same silicon
//       as the display controller and does not have a register map in the
//       usual sense.
//
// Both return NATIVE controller coordinates. Rotating them to the screen is
// main.cpp's job and is deliberately not done here -- that code already
// exists once, for every board, and a second copy in a driver is how the two
// drift apart.
#include "cap_touch.h"
#include <Wire.h>

namespace CapTouch {

#if defined(JC3248)

// ---- AXS15231B (Guition JC3248W535EN) --------------------------------
//
// Not a register map: the controller wants an 11-byte read command and
// answers with a small packet. The command and the packet layout below are
// from a confirmed-working driver for this exact board, not from guesswork
// against a datasheet.
//
//   packet[1]  — number of touches reported
//   packet[2]  — X high nibble (X[11:8])
//   packet[3]  — X low byte
//   packet[4]  — Y high nibble
//   packet[5]  — Y low byte
//
// Single touch only. The panel reports 5, and this firmware has never had a
// gesture that wanted a second finger.
static const uint8_t ADDR       = 0x3B;
static const int     MAX_TOUCH  = 1;
static const size_t  PKT_LEN    = MAX_TOUCH * 6 + 2;

void begin(int sda, int scl, int rst) {
    pinMode(rst, OUTPUT);
    digitalWrite(rst, LOW);
    delay(10);
    digitalWrite(rst, HIGH);
    delay(50);  // chip boot time after reset release
    Wire.begin(sda, scl);
    Wire.setClock(400000);
}

bool probe() {
    Wire.beginTransmission(ADDR);
    return Wire.endTransmission() == 0;
}

bool read(uint16_t& x, uint16_t& y) {
    const uint8_t readCmd[11] = {
        0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00,
        (uint8_t)(PKT_LEN >> 8),
        (uint8_t)(PKT_LEN & 0xff),
        0x00, 0x00, 0x00
    };

    Wire.beginTransmission(ADDR);
    Wire.write(readCmd, sizeof(readCmd));
    if (Wire.endTransmission() != 0) return false;

    uint8_t pkt[PKT_LEN] = {0};
    if (Wire.requestFrom((uint8_t)ADDR, (uint8_t)PKT_LEN) != PKT_LEN) return false;
    for (size_t i = 0; i < PKT_LEN; i++) pkt[i] = Wire.read();

    if (pkt[1] == 0 || pkt[1] > MAX_TOUCH) return false;

    const uint16_t rawX = ((uint16_t)(pkt[2] & 0x0F) << 8) | pkt[3];
    const uint16_t rawY = ((uint16_t)(pkt[4] & 0x0F) << 8) | pkt[5];

    // The controller emits occasional out-of-range packets; the working
    // driver for this board discards anything past 500 on either axis and
    // so does this. Panel is 320x480, so nothing real lands above that.
    if (rawX > 500 || rawY > 500) return false;

    x = rawX;
    y = rawY;
    return true;
}

#else

// ---- CST816/CST820 ---------------------------------------------------
// Standard Hynitron CST816-family register map (same layout used
// across the common open-source drivers for this chip family):
//   0x02: finger count (0 = up, 1 = down — single-touch chip)
//   0x03: X high byte (low nibble = X[11:8])
//   0x04: X low byte
//   0x05: Y high byte (low nibble = Y[11:8])
//   0x06: Y low byte
static const uint8_t ADDR = 0x15;

void begin(int sda, int scl, int rst) {
    pinMode(rst, OUTPUT);
    digitalWrite(rst, LOW);
    delay(10);
    digitalWrite(rst, HIGH);
    delay(50);  // chip boot time after reset release
    Wire.begin(sda, scl);
}

bool probe() {
    Wire.beginTransmission(ADDR);
    return Wire.endTransmission() == 0;
}

bool read(uint16_t& x, uint16_t& y) {
    Wire.beginTransmission(ADDR);
    Wire.write(0x02);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)ADDR, (uint8_t)5) != 5) return false;

    uint8_t fingerNum = Wire.read();
    uint8_t xh = Wire.read();
    uint8_t xl = Wire.read();
    uint8_t yh = Wire.read();
    uint8_t yl = Wire.read();
    if (fingerNum == 0) return false;

    x = ((uint16_t)(xh & 0x0F) << 8) | xl;
    y = ((uint16_t)(yh & 0x0F) << 8) | yl;
    return true;
}

#endif  // JC3248

}  // namespace CapTouch
