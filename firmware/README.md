# Prebuilt firmware — Guition JC3248W535EN

For people who own the board and would rather not install a toolchain.
Everything here is built from this repo by `pio run -e jc3248`.

If you own a **CYD** instead, none of this is for you — go to
[squachwatch.com](https://squachwatch.com/) and flash from the browser.

## Why there is no web flasher for this board

[ESP Web Tools](https://esphome.github.io/esp-web-tools/) reads the chip ID
over USB and refuses anything that does not match the manifest. Every manifest
upstream declares `"chipFamily": "ESP32"`; this board is an **ESP32-S3**. A
manifest for it is included here (`../web-flasher/manifest-jc3248.json`) and
will work if you host the `web-flasher/` directory yourself — GitHub Pages is
enough. It is not hosted anywhere by default.

## Flash it

Install [esptool](https://github.com/espressif/esptool) (`pip install esptool`),
plug the board in, and run **one** command from this directory:

```sh
esptool.py --chip esp32s3 --port COM7 --baud 460800 --no-stub write_flash -z \
  0x0      jc3248/bootloader.bin \
  0x8000   jc3248/partitions.bin \
  0xe000   jc3248/boot_app0.bin \
  0x10000  jc3248/firmware.bin
```

Replace `COM7` with your port (`/dev/ttyACM0` or similar on Linux/macOS).

### The four things that will otherwise waste your evening

**The bootloader goes at `0x0`, not `0x1000`.** On a classic ESP32 it is
`0x1000`, and every CYD guide you will find says so. On an ESP32-S3 that is
wrong and the board will not boot.

**`--no-stub` is not optional.** This board's USB is the S3's own
Serial/JTAG unit. esptool's stub loader re-enumerates the USB device as it
starts, the port disappears mid-write, and the flash dies with
`No serial data received` *after* it has already identified the chip.

**Press RESET when it finishes.** esptool's automatic post-flash reset does
not work reliably here, so until you reset it by hand the board keeps running
whatever was on it before. A flash that appears to have done nothing has
almost always just not restarted yet.

**If esptool cannot connect at all:** hold **BOOT**, tap **RESET**, release
**BOOT**, then run the command again. That parks it in the ROM bootloader,
which always answers.

## What you get

No USB serial console. The S3's USB Serial/JTAG unit resets the chip whenever
a host opens the port, and with USB CDC enabled and nothing draining its
buffer the firmware blocks in `setup()` — that is why an earlier build of this
port ran on a PC and hung on a power bank. Serial is on UART0 (GPIO43/44)
instead.

If you want the console, build `pio run -e jc3248-debug -t upload` from source
and accept that the board resets when you connect.

## Contents

| File | Offset | What |
|---|---|---|
| `jc3248/bootloader.bin` | `0x0` | second-stage bootloader |
| `jc3248/partitions.bin` | `0x8000` | partition table — two 1.92 MB OTA app slots |
| `jc3248/boot_app0.bin` | `0xe000` | OTA data: boot the first slot |
| `jc3248/firmware.bin` | `0x10000` | the firmware |

Settings, PINs and mesh pairings live in NVS at `0x9000`, which none of the
above overwrite — so this is safe to re-run over an existing install. Do **not**
flash a single merged image at offset 0 unless you mean to erase them: the
padding between these regions covers NVS.
