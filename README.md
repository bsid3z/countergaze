# SquachWatch for the Guition JC3248W535EN

> **A fork of [SquachWatch-CYD](https://github.com/skizzophrenic/SquachWatch-CYD)
> by [skizzophrenic](https://github.com/skizzophrenic)**, ported to the
> **Guition JC3248W535EN** — a 3.5" 320×480 ESP32-S3 board the original
> cannot run on.
>
> Surveillance-device detector. Everything except that port is his work.

### Two names, so neither surprises you

|  |  |
|---|---|
| **The software is `SquachWatch-CYD`.** | Written by **[skizzophrenic](https://github.com/skizzophrenic)** (Talking Sasquach). That is its name here too — this fork renames nothing. |
| **This repo is `countergaze`.** | Just the fork's address on GitHub, owned by [@bsid3z](https://github.com/bsid3z). It is not a product, a rebrand, or a competing project. |
| **The `-CYD` in the name is historical.** | It meant the Sunton "Cheap Yellow Display". The firmware now runs on seven boards, and the one this fork adds — the Guition JC3248W535EN — is **not** a CYD. The name stayed; the hardware list grew. |

> ### This is a fork. SquachWatch is skizzophrenic's work.
>
> Everything below — the firmware, the detection research, Squachy, SquachMesh,
> the SquachWare aesthetic, the emulator, the graphics on this page, every
> screen in the demo — was written by
> **[skizzophrenic](https://github.com/skizzophrenic)** (Talking Sasquach).
> Upstream is **[skizzophrenic/SquachWatch-CYD](https://github.com/skizzophrenic/SquachWatch-CYD)**
> and the project's home is **[talkingsasquach.com](https://talkingsasquach.com)**.
> **If SquachWatch is useful to you, support it there — not here.**
>
> This fork exists for exactly one reason: to add the **Guition JC3248W535EN**,
> a 3.5" ESP32-S3 board that upstream cannot run on. Every other board behaves
> identically to upstream, no feature has been added or removed, and the
> branding is untouched. See
> **[Changes from upstream](#changes-from-upstream)** for the complete list and
> **[docs/JC3248W535EN.md](docs/JC3248W535EN.md)** for how the port works.
>
> **Looking for the original, on a CYD?** Go to
> [skizzophrenic/SquachWatch-CYD](https://github.com/skizzophrenic/SquachWatch-CYD)
> or flash straight from [squachwatch.com](https://squachwatch.com/) — you do
> not need this fork unless you own a JC3248W535EN.
>
> GPL-3.0, same as upstream.

SquachWatch-CYD sniffs the 2.4 GHz airwaves for known wireless signatures
of Flock Safety cameras, Axon body cameras, recording glasses, card
skimmers, AirTags, drones, proximity beacons and pentest hardware. It runs
standalone on a bare CYD board — no PC, no extras, just plug it into USB.

The UI is a vaporwave-themed take on the **SquachWare** aesthetic: matrix
digital rain, Squachy the mascot, full-screen dramatic ALERT overlays, and
the glitchy SquachWatch wordmark.

<p align="center">
  <a href="https://squachwatch.com/emulator/" title="Drive it in your browser">
    <img src="docs/demo.gif" width="640"
         alt="SquachWatch booting, Squachy in the VOID EYE costume on the synthwave sunset, a Flock camera detection card, his reaction to it, and a visiting SquachWatch walking on to say hello">
  </a>
</p>

<p align="center">
  <b>That is the firmware itself, not a mockup.</b><br>
  Every frame above was rendered by the same C++ that runs on the board,
  compiled for a PC.<br>
  <a href="https://squachwatch.com/emulator/"><b>Click it to drive it in your browser &rarr;</b></a>
</p>

## What it detects

| Type | What | How |
|---|---|---|
| `FLOCK` | Flock Safety ALPR cameras | 29 WiFi OUI prefixes + BLE name + company ID `0x09C8` |
| `AXON` | Axon body cameras, TASERs, LE equipment | 3 WiFi OUI + SSID prefixes `AB2-`/`AB3-`/`AB4-`/`AXON-` |
| `META` | Camera glasses — Ray-Ban Meta, Snap Spectacles | BLE service UUID `0xFD5F` + Meta / Luxottica / Snap company IDs |
| `SKIMMER` | Bluetooth card skimmers (HC-05/06/03, RN42, BT04-A) | BT Classic name match + SPP UUID `0x1101` + 3 OUI |
| `RAVEN` | Raven gunshot detector | Service UUIDs `0x3100`–`0x3500` |
| `AIRTAG` | Apple AirTag / Find My trackers | Company ID `0x004C` + Find My payload check |
| `DRONE` | Remote ID drones | Service UUID `0xFFFA`, then the ASTM F3411 message **decoded** — aircraft position, altitude, serial, and the operator's location |
| `ALPR` | Motorola Solutions / Genetec plate readers | 7 WiFi OUI (5 Motorola, 2 Genetec) |
| `CAMERA` | Generic / covert IP cameras | 17 WiFi OUI (Wyze, Amazon, Tuya, Verkada, Avigilon, Axis, …) |
| `SAMSUNG_TAG` | Samsung Galaxy SmartTag / SmartTag+ | BLE service UUID `0xFD5A` |
| `GOOGLE_TAG` | Google Find My Device trackers (Chipolo, Pebblebee, Moto Tag) | BLE service UUID `0xFEAA` |
| `TILE` | Tile BLE trackers | BLE service UUID `0xFEED` / `0xFEEC` |
| `RING` | Ring doorbells / cameras | 15 WiFi OUI (Ring LLC's registered block + Amazon's) |
| `DEAUTH` | WiFi deauthentication floods | Rate-detected burst, not a signature |
| `EVILTWIN` | Rogue / spoofed access points | One SSID beaconing from two BSSIDs that disagree about encryption |
| `IBEACON` | Retail proximity beacons | Exact Apple header `4C 00 02 15` — **off by default**, see below |
| `HACKER` | Flipper Zero, Pwnagotchi, WiFi Pineapple, ESP deauthers | Flipper's service UUIDs `0x3081`–`0x3083`, company ID `0x0E29` and OUI `0C:FA:22`; the Pwnagotchi's own beacon payload; `Pineapple_` and `pwned` SSIDs |

### Confidence is per signature, not per type

Every hardware prefix in the firmware was checked against the IEEE registry
rather than against other detectors. Of the 77 OUI rows: **33 High, 4
Medium, 40 Low** — counted from `src/signatures.cpp`.

That grading matters most on `FLOCK`, where exactly **one** of 29 prefixes is
registered to Flock Safety and the rest are the generic Espressif and Liteon
parts they build on — real evidence, shared with every dev board on earth.
`ALERT FILTER` is a minimum-confidence gate, so setting it to High keeps a
passing ESP32 in the log without taking over the screen.

The audit also removed `00:0E:58`, which sat here for eleven releases
labelled "Vigilant" and is registered to **Sonos**. Every speaker in range
was being logged as a plate reader.

`IBEACON` ships switched off — not a judgement about importance, one about
volume. One shop can put more beacons in range than this device would
otherwise see all week. It is one tap away in `DETECTION FILTER`.

## Hardware

- **Sunton ESP32-2432S028R** ("Cheap Yellow Display" / CYD) — about $15.
  Built-in 320×240 ILI9341 TFT, XPT2046 resistive touch, and an
  onboard microSD card slot.

That's it. No buzzer, no GPS, no extra modules. The CYD is the
whole device.

### Every supported board

"CYD" properly means the Sunton ESP32-2432S028R and its siblings. The Guition
board at the bottom is **not** a CYD and is not compatible with a CYD build —
different manufacturer, different chip, different display bus. It is listed
here because this fork is the thing that added it.

| Board | Screen | Display driver | Touch | Chip | Build env |
|---|---|---|---|---|---|
| Sunton ESP32-2432S028R (CYD) | 2.8" 240×320 | ST7789, SPI | XPT2046 resistive | ESP32 | `cyd` |
| Sunton ESP32-2432S028R (CYD) | 2.8" 240×320 | ILI9341, SPI | XPT2046 resistive | ESP32 | `cyd-ili9341` |
| Sunton ESP32-2432S024R (RL Phantom) | 2.4" 240×320 | ILI9341, SPI | XPT2046 resistive | ESP32 | `rlphantom-r` |
| Sunton ESP32-2432S024C (RL Phantom) | 2.4" 240×320 | ILI9341, SPI | CST820 capacitive | ESP32 | `rlphantom` |
| AWOK ESP32-Marauder V6.1 | 2.4" 240×320 | ILI9341, SPI | XPT2046, shared bus | ESP32 | `awok` |
| Sunton ESP32-3248S035 | 3.5" 480×320 | ST7796, SPI | XPT2046, shared bus | ESP32 | `cyd35` |
| **[Guition JC3248W535EN](https://nl.aliexpress.com/item/1005007566315926.html)** | **3.5" 320×480** | **AXS15231B, QSPI** | **AXS15231B capacitive** | **ESP32-S3** | **`jc3248`** |

The two 2.8" rows are the same physical board — it shipped with either panel,
and the two cannot be told apart without flashing one and looking. `cyd35` is
frozen upstream and boot-loops on real hardware; it is not recommended.

**Where to buy the Guition JC3248W535EN** — the board this fork exists for:
[listing 1](https://nl.aliexpress.com/item/1005007566315926.html) ·
[listing 2](https://nl.aliexpress.com/item/1005007593889279.html).
Check the model number on the listing before you order. `JC3248W535EN` is the
3.5" 320×480 capacitive ESP32-S3 board in the row above; Guition sells several
boards with near-identical part numbers and only this one is supported here.

## Web Flash

No build tools, no IDE, no cloning anything — flash a board straight
from your browser:

**[https://squachwatch.com/](https://squachwatch.com/)**

Works in Firefox, Chrome, Edge, or Brave on desktop. Pick your board (2.8" CYD,
AWOK 2.4" or RL Phantom 2.4"), plug in, click Connect & Install, done.

**The Guition JC3248W535EN is not on the web flasher,** upstream's or this
fork's. Every manifest there declares `"chipFamily": "ESP32"`, and that board is
an ESP32-S3; ESP Web Tools reads the chip ID over USB and refuses. This is not a
manifest typo — the other boards' binaries are Xtensa LX6 code that would not
boot on an LX7 even if it did flash. JC3248W535EN owners build from source, one
command, below.

## Build

Three steps:

1. Install [PlatformIO](https://platformio.org/) (CLI or VS Code extension).
2. Clone the repo:
   ```sh
   git clone https://github.com/bsid3z/countergaze
   cd countergaze
   ```
   That is **this fork**, and it is what the JC3248W535EN steps below need —
   upstream has no `jc3248` environment, so cloning
   `skizzophrenic/SquachWatch-CYD` and then asking for that board fails.
   If you own a CYD and want the original, clone upstream instead; every
   other board builds identically from either.
3. Build and flash:
   ```sh
   pio run -t upload
   ```
   That builds the three CYD-family boards. **For the JC3248W535EN you must
   pass `-e jc3248`** — see the section right below.

The first build pulls the TFT_eSPI, XPT2046, and NimBLE-Arduino
libraries; after that it's incremental.

A full beginner-friendly walkthrough is in [docs/BUILD.md](docs/BUILD.md).

### Guition JC3248W535EN

**Prebuilt binaries, no toolchain:** [`firmware/`](firmware/) has the four
`.bin` files and the exact `esptool` command. Read the four gotchas in
[firmware/README.md](firmware/README.md) first — the bootloader goes at `0x0`
on an S3, not `0x1000`, and `--no-stub` is required.

To build it yourself, three commands:

```sh
git clone https://github.com/bsid3z/countergaze
cd countergaze
pio run -e jc3248 -t upload
```

`-e jc3248` is required — a bare `pio run -t upload` builds the CYD boards and
will not produce something this board can run.

Two things about this board specifically, both of which will otherwise cost you
an evening:

- **Press the RESET button after flashing.** esptool's automatic post-flash
  reset does not work reliably over this board's native USB, so until you reset
  it by hand the board keeps running the *previous* firmware. A flash that
  looks like it did nothing has almost always just not restarted yet.
- **If esptool cannot connect,** hold **BOOT**, tap **RESET**, release **BOOT**,
  and flash again. That puts it in the ROM bootloader, which always answers.

USB serial debugging is **off** by default on this board. The ESP32-S3's
USB Serial/JTAG unit resets the chip whenever a host opens the port, and with
nothing draining the CDC buffer the firmware blocks in `setup()` — which is why
an earlier build ran on a PC but hung on a power bank. For a console, build the
debug variant instead:

```sh
pio run -e jc3248-debug -t upload
pio device monitor -e jc3248-debug
```

## Usage

1. Plug the CYD into USB-C.
2. The splash runs for a second and a half, stamped with the build's own
   version (from `git describe`, so a working-tree build says so).
3. The main screen appears: your chosen background, Squachy, and live
   per-type counters. He says something reassuring every thirty seconds.
4. The three soft buttons at the bottom:
   - **`[ SCAN ]`** — return to the main (idle) screen.
   - **`[ LOG ]`** — open the rolling 200-entry detection log.
   - **`[ DESK ]`** — desk mode: the big clock, with Squachy under it.
   - On the LOG screen the third button is **`[ CLR ]`** — wipe the log and return.
5. When something is detected, the device **flashes a full-screen ALERT**:
   a header strip in the detection's own colour with the type in the
   Bangers face, a data plate with the vendor, the device's own name where
   it broadcasts one, its MAC and a signal meter, and a gauge showing what
   was found with the instrument grid over it. Tap anywhere to dismiss
   early, or it clears itself after 60 seconds.

If a microSD card is present, every detection is also appended to
`squachwatch-<day>.log` (CSV: `ts,type,rssi,mac,channel,vendor,ssid`).

### The clock

There is no GPS, and the board never joins a network to scan. But it does
join one for the update check at boot, and for UPDATE OVER WIFI, and the
clock rides along: one NTP round trip while the radio is up anyway, about a
second. The zone is yours to pick, and there are three ways: the web
flasher's **Set Time & Zone** button sends this computer's clock and zone
down the same cable right after flashing; the first time the clock is set
with no zone chosen, a card on the main screen asks, with the live time in
the zone it shows so you can see when it's right; and **TIME ZONE** on the
DESK MODE page changes it later. Daylight saving takes care of itself. Without
a saved network the clock can still be set over serial with a `TIME <epoch>`
line at 2,000,000 baud, and `ZONE US EASTERN` sets the zone the same way.
And every squad hello carries the sender's clock and zone, so a board with
neither takes them from the first member it hears: update one board by USB
and the rest of the squad know the time within a minute of meeting it.
Until the clock is set, timestamps count from boot. The board keeps a
note of the time in flash every ten minutes, and a cold boot with no clock
starts from that note: not the right time, since nobody knows how long the
power was off, but never earlier than the note, which keeps the day count
honest. Such a clock is used for the date only; the LOG times, the night
tag, the hour lines and the desk digits wait for a real answer.

Once it is set, the LOG shows the real time of each catch (or the date, for
one from another day); the alert card says **AT NIGHT** for anything caught
between eleven and five, and Squachy's line sharpens to match; he says hello
once a day with the date in it, knows whether it's Monday, lunch, the three
o'clock slump or two in the morning, and counts the days since the board
first knew the date: a week, a month, a hundred days, a year.

**BANTER** on the APPEARANCE page sets how much he talks when nothing is
happening: IMPORTANT (idle chatter off; he still speaks for a catch, a
message, a newer release and the daily hello), LESS, NORMAL or MORE. The
set pieces two Squachys act out follow the same setting.

### Desk mode

<p align="center">
  <img src="docs/desk-mode.gif" width="640"
       alt="Desk mode: the date and time in big digits over the fire scene with Squachy talking below; a catch appears as a small card; a squad message drops out from behind the clock with the sender's polaroid; the focus timer starts; the LOG shows real times; the time zone card asks once.">
</p>

**DESK MODE** in Settings turns the board into the thing beside the
keyboard: your background, the date and the time in big seven-segment
digits on a plate over it, Squachy underneath doing what he does, and a
focus timer. **FOCUS 25** starts twenty-five minutes: the scene clears, he
goes still and quiet, and when it runs out the light on the back goes
green, he tells you to stand up, and a five-minute break counts down on its
own. A tap on the running timer stops it. The desk keeps its own
background, picked with a tap at the left or right edge and remembered
separately from the main screen's. Detection keeps running behind all of
it: a catch shows as a small card by the buttons instead of the full
ALERT (tap it for the full card), and a squad message stands where Squachy
stands, as a polaroid of the sender's Squachy with his name in the margin,
the message on a note beside it and the time it came, until you tap it.
The power saver never dims this screen.

## The status light

The RGB LED on the back of the 2.8" CYD (on the front of the RL Phantom)
tells you what the screen is doing without the screen. A slow breathe in the
theme's colour when nothing is happening; three flashes and a hold in the
detection's own colour when something is, for as long as the alert card is
up; a double-blink for an unread message; a blip when a squad member walks
on; cyan while an update downloads and green or red for how it went. It goes
dark on the lock screen and through a wipe, so a duress restart looks like any
other restart from the back too.

**Settings → APPEARANCE → STATUS LIGHT**: the master switch, alerts and
messages on or off, idle breathe or solid or off, an idle colour that follows
the theme, the background, or one of nine fixed colours, brightness in five
steps, and a TEST row that plays the lot in six seconds. Boards whose LED pins
have not been checked (the AWOK and the 3.5") compile it out and say so on
that screen.

## SquachMesh

> **Work in progress.** It is in this release because it works — two boards
> find each other and each draws the other's Squachy — but it has had days of
> testing, not months. Both halves are **off** until you turn them on, and one
> of them costs you something; the device asks before it lets you near the
> switch.

<p align="center">
  <img src="docs/squachmesh.gif" width="640"
       alt="Two SquachWatches in range of each other. One Squachy walks in, they greet each other, and the pair stand around talking.">
</p>

Two SquachWatches in range of each other notice, and each one draws the
other's Squachy as a visitor. He walks in, they high five, they stand around
talking — now and then breaking into one of the thirty-odd emotes on their
own, a pie fight, a coin toss, a selfie, a dance-off, the same one on both
screens with the same result — and he goes home when the other board does.
His outfit, his shades and his name all travelled over the air in a
twenty-byte BLE advert. The name is one row, **NAME** under SQUACHMESH: a
curated one until somebody types one on the payphone, where **SHUFFLE**
steps through the curated list for anyone who would rather not type.
Whichever it is, the visitor wears it on a sticker on his chest.

It is deliberately not a network. No pairing, no connection, no
acknowledgement, no retry — a broadcast that says who is here, and anybody in
earshot may or may not catch it. A peer is recognised inside the scan callback
and returns before the signature tables ever see it, so two of these can never
set each other off.

**Settings → SQUACHMESH**, and it asks first. `DETECT` is receive-only: you
see other people's Squachys and broadcast nothing at all. `TRANSMIT` is the
half that makes you visible, and a full-screen warning stands in front of that
menu spelling out what goes out, how often, and what somebody with a scanner
can reconstruct from it — a fixed address that never changes is a trail of
where you have been. Nothing is transmitted until you have read that and
chosen YES.

That warning is not a formality. Broadcasting a stable identifier at strangers
is the exact behaviour this device exists to catch other people's hardware
doing. Offering it is defensible; switching it on quietly would not be.

### Messages

<p align="center">
  <img src="docs/squachmesh-messages.gif" width="640"
       alt="A visiting Squachy sends a typed message that lands in a red speech bubble; a ready-made reply is chosen, confirmed and sent, the visitor answers, and the phrase picker shows its big alphabet and word list.">
</p>

Two SquachWatches that share a five-word phrase can message each other: one
of 24 ready-made lines, or up to 48 characters typed on the payphone or the
QWERTY board. A message arrives as a **red** bubble with the sender's name in
it, so it is never mistaken for the Squachys' own chatter, and nothing is sent
until you have confirmed it.

**Settings → SQUACHMESH → MESSAGES**, then **PHRASE**: one of you ROLLs five
words and reads them out, the other ENTERs the same five. Setting a phrase
freezes the screen for about three seconds on purpose — it is 20,000 rounds of
PBKDF2, which every guess at your phrase has to pay too. A seven-card tutorial
runs the first time MESSAGES is switched on, and the **?** on the message
screen replays it; it never transmits anything.

Messages are AES-128-CCM, keyed from the phrase, with a nonce that cannot
repeat even across a crash, and every board checks its cipher against frames
made by an independent implementation at each boot. What stays visible is
that you sent something, and when: the contents are encrypted, the fact of a
message is not.

### Joining without typing

<p align="center">
  <img src="docs/squad-invite.gif" width="640"
       alt="Two boards side by side: one taps ADD TO SQUAD, the other's board asks and accepts, both show the same four digits, the phrase goes over, and the second board is in without typing anything.">
</p>

The typed phrase is the reliable way in and always will be. The convenient
way is **ADD**, beside INVITE and HUNT on the SQUAD screen (the **+N** next to
a visitor). Pick a board in range and tap it; their board asks them whether
they want in. Both screens then show the same four digits, which the two of
you compare out loud, and the phrase goes across sealed under a key that
exists for that one exchange and no other. The digits are derived from both
boards' keys, so a third board in the middle pretending to be each of you to
the other leaves the two screens disagreeing — say NO and nothing was sent.
The new member's board answers with a sealed hello the moment it has the
phrase, the inviter's shows **ADDED**, and both drop back to the main screen
on their own. If nothing comes back, the inviter's screen says so and offers
to show the phrase for typing.

Boards that have shown they hold your phrase read **MEMBER** on that screen,
and ADD only offers itself to strangers. Anyone with the phrase can invite
anyone; the phrase is the membership, and leaving somebody out means a new
phrase on every board.

### Your squad

**Settings → SQUACHMESH → SQUAD** is the roster: everybody who has ever been
heard holding your phrase, here or not, up to sixteen, kept across restarts.
Each member shows in the outfit from their latest advert, with how many
separate times you have met, those in range first. INVITE works when they
are here, AWAY says when they are not, and FORGET drops them after asking
once; they come back the next time they are heard with the phrase. A new
phrase clears the roster, because a new phrase is a new squad.

### Fox hunt

**HUNT** on either SQUAD screen aims HUNT MODE's signal gauge at that board.
It is the same meter the detector uses for a tag: no compass, so you turn
your body and walk toward where the needle does not fall. Two readings in a
row at arm's length and the gauge says **CAUGHT!**, Squachy bounces, and the
light on the back flashes green. The fox needs TRANSMIT on; the hunters need
DETECT on, which they have if they can see the SQUAD screen at all.

**SHOW PHRASE** on the PHRASE screen is on by default. Off, the five words
become dashes, the board never prints them, and the only way into the squad
from that board's side is ADD TO SQUAD, in person.

### Knowing there is an update

Two ways, neither of which installs anything. At boot, a board with a saved
WiFi network joins it for about a second, asks squachwatch.com for the latest
version of its own build, and lets go again, all before Bluetooth starts;
**UPDATE CHECK** on the SYSTEM page turns that off. And every board's hello
to its squad carries its version, so a board that hears a member running
something newer knows without touching WiFi. Either way Squachy says it once
on the main screen, the SYSTEM row reads UPDATE, and UPDATE FIRMWARE names
the version until you install it.

> **Not on the Guition JC3248W535EN.** The check asks squachwatch.com for
> `manifest-<build>.json` — the URL carries the build name — and that site
> cannot host a board upstream does not build. It is a guaranteed 404, so the
> boot check is compiled out rather than spending a WiFi join and a second of
> boot on it every start. The squad-hello route still works, and so does the
> manual **UPDATE OVER WIFI** screen, which now says *"the site has no update
> for this board"* instead of blaming your internet connection.
>
> To enable it, publish this fork's `web-flasher/` output somewhere and point
> the build at it with `-DOTA_WIFI_BASE='"http://your.host/"'`. It must be
> plain HTTP: `ota_wifi.cpp` uses a bare `WiFiClient` with no TLS, so an
> HTTPS-only host such as GitHub Pages will not work without giving it a
> secure client first.

**WIFI NETWORKS** on the SYSTEM page is where the board keeps the networks it
knows: up to six, with USE marking the one it tries first. ADD picks one from
a scan and takes the password on the board's keyboard; it is not checked by
joining, since joining means giving Bluetooth up until a restart, but at the
next boot check, and each row then says how that went: joined, wrong
password, or not found. At boot the board scans, joins the USE network if it
is there and otherwise the strongest saved one that is, so home and work both
just work. The update flow does the same, and only shows its own list when
none of the saved networks is in range. REMOVE takes one off the list.

### Smaller things

- **Arrows on NEARBY.** Each device shows a green up-arrow when it has come
  closer since its last reading and a red down-arrow when it has moved away;
  under four dB of change shows nothing, which is what a still device does.
- **First of its kind.** The first time this board ever catches a type, the
  card says so and Squachy marks the occasion when you get back to him.
- **FILL on the message screen.** Eight openings that end in a blank, MEET AT,
  I'M AT, BACK IN and the rest; pick one and the keyboard opens with it typed.
- **Read receipts.** When a squad member opens your message their board says
  so, and yours shows a READ toast with their name. A reader with TRANSMIT off
  can't send one, so you see sent and never read, which is the truth.
- **SNOOZE on an alert.** Quiets that one device until the board restarts. It
  is still scanned, counted and logged; only the alert stops. IGNORE is the
  same thing kept for good.
- **Banter about something.** Two Squachys now talk about the weather on
  screen, what was caught earlier, each other's outfits, how many times
  they've met, the squad's size, and the length of the day, one exchange in
  three, when there's something to say.

### Updating the squad

**Settings → SYSTEM → UPDATE FIRMWARE → UPDATE SQUAD** tells every board in
range with your phrase to install the version this one is running. Each of
them shows a thirty-second countdown with SKIP, joins WiFi, installs the
signed release from squachwatch.com, restarts, and reports back by name to
the board that asked. The sender can share its own saved network with the
nudge, sealed with the phrase; the receiving boards use it once and forget it.

A board listens because it holds your phrase, which is the same trust it
already gives you for messages and the invite; **REMOTE UPDATE** on its
SECURITY screen turns that off for anyone who wants it off. A locked board
ignores the whole thing regardless. So the order on release day is: update
one board by hand, then UPDATE SQUAD from it.

## Every outfit

Squachy has fourteen costumes. Most are earned by detection count; four are
hidden behind things nobody tells you about, on the background they belong
to. Two of them are in the animation at the top of this page.

<p align="center">
  <img src="docs/outfits.png" width="880"
       alt="All fourteen of Squachy's outfits, rendered by the firmware">
</p>

No fabricated marketing shots, which was the promise here before there was
anything to show. Every panel above was drawn by the firmware, one render
per costume, and the labels are read out of the source rather than typed
next to it — so a renamed or newly added outfit cannot end up captioned
wrongly. Regenerate with `python3 make_gallery.py` in `sim/`.

## Project layout

```
SquachWatch-CYD/
├── platformio.ini
├── README.md
├── LICENSE
├── docs/
│   ├── FAQ.md                    (what it does, hardware, legality)
│   ├── DESIGN.md                 (the contract — single source of truth)
│   ├── BUILD.md                  (friendly walkthrough)
│   ├── PINOUT.md                 (CYD pin map)
│   ├── DETECTIONS.md             (per-signature provenance)
│   └── SQUACHWARE-AESTHETIC.md   (CSS → RGB565 mapping)
├── include/
│   ├── state.h                   (DetectionType, Detection, Confidence)
│   ├── theme.h                   (palette, backgrounds, icons, chrome)
│   ├── signatures.h              (the tables and their lookups)
│   ├── detection.h
│   ├── remote_id.h               (ASTM F3411 decoder)
│   ├── clock.h                   (wall clock: NTP at the boot check, zones, the calendar)
│   ├── ignore_list.h             (per-device alert suppression)
│   ├── status_light.h            (the RGB LED and its rules)
│   ├── meshmsg.h                 (sealed frames: messages, emotes, nudges, invites)
│   ├── squachmesh.h              (the SquachMesh wire format -- read first)
│   ├── settings.h
│   ├── squachy.h                 (the mascot)
│   ├── bangers_font.h            (generated 1bpp display face)
│   ├── cyd_user_setup.h          (TFT_eSPI config for the CYD)
│   └── ui_*.h
├── src/
│   ├── main.cpp                  (setup/loop, state machine, touch)
│   ├── theme.cpp                 (backgrounds, per-type icons, chrome)
│   ├── squachy.cpp               (the mascot, his outfits and his lines)
│   ├── signatures.cpp
│   ├── detection.cpp             (WiFi promiscuous + NimBLE scan)
│   ├── remote_id.cpp
│   ├── clock.cpp
│   ├── ignore_list.cpp
│   ├── pet.cpp
│   ├── squachmesh.cpp            (SquachMesh encode/decode, no radio)
│   ├── meshtalk.cpp              (messages, the squad update, the invite)
│   ├── meshcrypto.cpp            (AES-CCM, PBKDF2, and X25519 for invites)
│   ├── status_light.cpp
│   ├── sd_log.cpp
│   └── ui_*.cpp
├── test/                         (host tests -- `make -C test`, no framework)
└── sim/                          (PC emulator — compiles src/ natively)
    ├── Makefile                  (`make` for the CLI, `make wasm` for the web build)
    ├── *.h                       (Arduino/TFT_eSPI/NVS shims)
    ├── make_readme_demo.py       (renders the animation at the top of this file)
    ├── make_demo.py              (the older background tour)
    ├── make_mesh_demo.py         (renders the SquachMesh clip above)
    ├── make_gallery.py           (renders the outfit sheet above)
    ├── make_social.py            (renders the repo's social preview card)
    └── web/                      (the browser build)
```

## Changes from upstream

Everything this fork alters, and why. Nothing else in the tree is touched:
no feature added or removed, no branding renamed, and no board upstream
supports behaves differently.

### New: the Guition JC3248W535EN

| Added | Why |
|---|---|
| `[env:jc3248]`, `[env:jc3248-debug]` | ESP32-S3 target. Upstream has no environment for this board, so a clone of upstream cannot build it at all. |
| `src/jc3248_panel.cpp`, `include/jc3248_panel.h` | Its panel is an AXS15231B on a **four-lane QSPI** bus. TFT_eSPI cannot drive that — not a missing driver, a bus it does not speak — so something had to own the transport. |
| `sim/TFT_eSPI.h` → `gfx/TFT_eSPI.h` | The emulator's portable rasterizer was already drawing every screen of this firmware; it just had no way out to real glass. Moving it makes one copy serve the emulator, the host tests **and** this board, so a drawing fix lands everywhere at once instead of drifting. |
| Battery percentage (`src/battery.cpp`) — **written, then disabled** | This is the only board with a battery input (IP5306, cell connector `P5`). The title bar gains a percentage left of the padlock and rotate icons. **It is compiled out**: reading GPIO5 — the pin a third-party config names as the battery divider — was confirmed on hardware to cut the board's battery power path entirely. See [docs/JC3248W535EN.md](docs/JC3248W535EN.md#do-not-read-gpio5). Other boards report no battery and draw nothing, so their bar is unchanged either way. |
| AXS15231B touch in `src/cap_touch.cpp` | Capacitive, I2C `0x3B`, a command/response protocol rather than a register map. The existing CST816 path could not read it. |
| `firmware/`, `manifest-jc3248.json`, picker entry, release-workflow steps | Owning the board means shipping it: prebuilt binaries for people without a toolchain, and a release that actually contains it. |

Full detail in **[docs/JC3248W535EN.md](docs/JC3248W535EN.md)**, including three
dead ends recorded so nobody repeats them.

### Bugs found and fixed — these affect every board

| Bug | Why it mattered |
|---|---|
| `fillRect` → `drawFastHLine` → a **virtual `drawPixel` per pixel** | A full-screen fill cost 225 ms on the S3. `fillSpan` writes the row instead. The emulator and every CYD get the same speedup. |
| `drawFastVLine` had the same shape | Fixed the same way, with `fillColumn`. Frame time on the JC3248W535EN went 407 ms → 137 ms overall. |
| **Neither had a test** | `shim_fidelity_test` covers glyph cells and ellipse radii and never calls a fill primitive, so CI was green on untested code. `test/fillprim_test.cpp` now draws every case twice — fast path vs `drawPixel` — and compares buffers: every edge, zero and negative lengths, viewports, 675 colours, both depths. Verified it fails when `fillColumn`'s clip is moved by one. |
| `randomSeed(analogRead(34))` | GPIO34 is not an ADC pin on an ESP32-S3: it logged an error, returned 0, and the board replayed the **same "random" sequence every boot**. Now `esp_random()`, the hardware TRNG — no pin, and a better seed than a floating input was on the boards where it worked. Nothing security-relevant used `random()`. |
| A 404 was reported as *"couldn't reach squachwatch.com"* | The update URL carries the build name, so any board whose build is not published there 404s — and the message sent people to debug a working router. New `NO_BUILD_FOR_BOARD` failure says what actually happened. |

### Bugs found in upstream's own docs

These predate this fork and are worth sending back to skizzophrenic.

| Bug | Why |
|---|---|
| `docs/FAQ.md` said **"MIT-licensed"** | `LICENSE` is GPL-3.0 and the README says so three times. Someone could have relied on the FAQ and relicensed in good faith. |
| `docs/FAQ.md` linked `docs/BUILD.md` *from inside* `docs/` | Resolves to `docs/docs/…`. Three dead links. |
| README said ALPR has **6** OUIs, and **"76 rows: 32 High"** | The table holds **7** and **77 / 33 High**. One ALPR prefix was added and none of the three numbers describing the table were updated. |

### Changed because an S3 is not an ESP32

| Change | Why |
|---|---|
| `setup()` no longer drives GPIO 21/27/32 high on this board | On a CYD those are candidate backlight pins and the spare ones are harmless. On an ESP32-S3, **GPIO26–32 are the internal flash and PSRAM lines** — two of those writes reconfigured the bus the CPU was executing from. Interrupt-watchdog boot loop, every boot, before `tft.init()`. GPIO21 is also QSPI D0 here. |
| `SdLog::begin()` returns early on `jc3248` | No usable card slot, and the fallback path calls `SPI.begin(18, 19, 23, 5)` — **GPIO19 is USB D−** on an S3. It would have taken out the USB console to mount a card that is not there. |
| `FastSprite`/`FramePush` gate on `SQW_REAL_TFT_ESPI`, not `ARDUINO_ARCH_ESP32` | Both reach into the real TFT_eSPI's internals. This build has the architecture but not the library. |
| USB CDC **off** by default; `jc3248-debug` turns it on | The S3's USB Serial/JTAG unit resets the chip whenever a host opens the port, and with CDC on and nothing draining the buffer the firmware blocks in `setup()` — it ran on a PC and hung on a power bank. |
| Boot-time update check skipped on `jc3248` | `manifest-jc3248.json` is a guaranteed 404 on upstream's site, which cannot host a board it does not build. Paying a WiFi join and a second of boot for that answer, every boot, is worse than not asking. The manual UPDATE OVER WIFI screen still runs. |
| `createSprite()` checks the largest free block first | `main.cpp` has a tested no-frame-buffer path. The PSRAM allocator returns null on exhaustion, and with exceptions off `std::vector` writes through it and panics — so that path could never run. |
| One anonymous struct in `main.cpp` given a name | GCC 8 at C++17 will not assign `{}` to an unnamed type, and the rasterizer needs C++17. Identical in every other build. |
| `pushFrame()` logs a dropped frame | Refusing a wrongly-shaped buffer is right; doing it silently looks like a hang on the glass. |
| CI builds `jc3248`; `branches: [master, main]` | The workflow watched only upstream's branch name, so on this fork it had **zero runs** — indistinguishable from passing. |
| Clone URLs point here | Both walkthroughs said to clone upstream, then told you to build `-e jc3248`, which upstream has no environment for. |

### Porting another ESP32-S3 board is now mostly done

Worth stating plainly, because it was not obvious going in: **almost none of
this port is about the JC3248W535EN.** The board-specific part is one file.
Everything else was making an ESP32-S3 work at all, and that is reusable.

**Already generic — you get these for free:**

- `gfx/TFT_eSPI.h` as a *real driver* under `SQW_HW_PANEL`, not a preview.
  Any panel TFT_eSPI cannot drive can be reached this way: the whole UI draws
  into an RGB565 buffer and hands it to one push function.
- The PSRAM allocator and the `createSprite()` free-block check — any S3 with
  external RAM needs both, since a full frame buffer does not fit in SRAM.
- The **GPIO26–32 guard**. Upstream drives 21/27/32 high to find a backlight;
  on any S3 those are the internal flash and PSRAM lines. This is the bug that
  boot-looped the board before `tft.init()`, and it is fixed for all S3 builds,
  not just this one.
- `FastSprite`/`FramePush` gating on `SQW_REAL_TFT_ESPI` rather than on the
  architecture, so they stay out of any build without the real library.
- USB CDC off by default, and the reason written down — every S3 with native
  USB has the reset-on-open and the blocking-buffer problem.
- The SD guard, the C++17 fix, the flash offsets (bootloader at `0x0`, not
  `0x1000`), `--no-stub`, and CI that builds an S3 target.

**What a new S3 board still needs — roughly a day, not a project:**

1. A transport like `src/jc3248_panel.cpp`: bring the panel up, and push one
   RGB565 buffer to it. ~150 lines.
2. A pin header like `include/jc3248_user_setup.h`.
3. An `[env:...]` modelled on `[env:jc3248]`.
4. A touch branch in `src/cap_touch.cpp`, if its controller is new.

If the panel is ordinary SPI and TFT_eSPI supports it, you need even less —
drop `SQW_HW_PANEL` and the normal TFT_eSPI path works, with only the S3
hardware guards above mattering.

The one thing that does **not** generalise is the dead ends in
[docs/JC3248W535EN.md](docs/JC3248W535EN.md): no partial window writes and
MADCTL ignored are facts about the AXS15231B controller, not about the S3.

### Deliberately NOT changed

| Left alone | Why |
|---|---|
| `"SquachWatch/msg/v1"` key-derivation salt | Renaming it breaks mesh interop with every existing board and fails `MeshCrypto::selfTest` at boot. It contains the project's name; it is not a name. |
| `"squachwatch"` NVS namespace | Renaming orphans saved settings on existing devices. |
| `squachwatch-<day>.log` filename | Renaming orphans existing SD logs. |
| All branding — SquachWatch, Squachy, SquachMesh, SquachWare | Not ours. The fork adds a board; it does not rebrand someone else's project. |
| Upstream's commit history | 348 commits, unmodified. The git log is the strongest attribution there is. |
| Links to `squachwatch.com` and `talkingsasquach.com`, and the `funding_url` | Those point at the original author on purpose. |

## License

**GNU General Public License v3.0 (GPL-3.0).** See [LICENSE](LICENSE).

Copyright remains with the original authors. This fork is redistributed under
the same licence, as GPL-3.0 requires.

## Credits

- **SquachWatch itself — all of it — is by
  [skizzophrenic](https://github.com/skizzophrenic) (Talking Sasquach).**
  The firmware, the detection research, Squachy, SquachMesh, the SquachWare
  look, the emulator and the web flasher are his work. This fork adds one
  board and changes nothing else.
- Flock Safety OUI research: [@NitekryDPaul](https://x.com/NitekryDPaul),
  DeFlockJoplin, [`colonelpanichacks/flock-you`](https://github.com/colonelpanichacks/flock-you)
  (MIT).
- Axon / skimmer / SSID prefix data: compiled with assistance from
  Gemini (Google), expanded against public sources.
- AirTag manufacturer-data format: public Apple FindMy spec.
- AWOK 2.4" board port (ESP32-Marauder V6.1 hardware): **bkbroiler**,
  who did the actual pin-mapping and shared-bus touch-calibration work
  that made this board possible.

## Status

**Shipping.** Releases are cut by pushing a `v*.*.*` tag; the flasher above
is rebuilt and redeployed by the same CI run, so the web flasher always
matches the newest release.

Detection is reliable for the high-priority targets (Flock, Axon, skimmer,
camera glasses). Remote ID and iBeacon are exact-format matches. Raven,
generic ALPR and the Google tracker network are best-effort — see
[docs/DETECTIONS.md](docs/DETECTIONS.md) for per-signature provenance and
the confidence each one earns.

Verified on real hardware. There is also a PC emulator in `sim/` that
compiles the actual `src/` against shims, and a host test suite in `test/`
(`make -C test`) covering the decoders, the signature tables and the
emulator's own fidelity to the display library.
