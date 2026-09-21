// SquachWatch-CYD — Guition JC3248W535EN board setup.
//
// NOT a TFT_eSPI user setup, unlike every other include/*_user_setup.h here.
// TFT_eSPI is not in this build at all (lib_ignore in [env:jc3248]): the
// panel is an AXS15231B on a four-lane QSPI bus, which that library cannot
// drive. Drawing goes through gfx/TFT_eSPI.h -- the portable rasterizer the
// web emulator uses -- and reaches the glass through jc3248_panel.cpp.
//
// What is left in a setup header, then, is the part that was never about the
// display driver: the geometry the UI lays itself out from, and the pins the
// shared code in main.cpp reads.
#pragma once
#define USER_SETUP_INFO "SquachWatch-CYD / Guition JC3248W535EN 3.5in / AXS15231B QSPI / ESP32-S3"

// Native panel geometry: 320x480 portrait glass. The firmware runs it at
// rotation 1, so the UI sees 480x320 landscape -- the same way round as
// every board that ships, and the same as main.cpp's own default for
// screenRotation. The turn happens in the canvas, not here.
#define TFT_WIDTH   320
#define TFT_HEIGHT  480
#define TFT_ROTATION 1

// Backlight: GPIO1 here, against the CYD's 21. main.cpp drives the other
// boards' backlight channels directly, so this board's lives on its own LEDC
// channel inside jc3248_panel.cpp and is reached through
// Jc3248Panel::setBacklight() instead.
#define TFT_BL              1
#define TFT_BACKLIGHT_ON    1
#define PWM_FREQ            5000
#define PWM_MAX_DUTY        255

// Capacitive touch — AXS15231B, I2C 0x3B. Same silicon as the display
// controller, on its own I2C bus. src/cap_touch.cpp speaks its protocol.
#define TOUCH_SDA_PIN   4
#define TOUCH_SCL_PIN   8
#define TOUCH_RST_PIN  12
#define TOUCH_INT_PIN  11

// The fonts the rasterizer carries, named the way the other boards' setups
// name them so shared #ifdefs still read true.
#define LOAD_GLCD
#define LOAD_FONT2
