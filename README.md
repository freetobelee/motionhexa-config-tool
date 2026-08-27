# Motionhexa Config Tool

A local, zero-install browser tool for editing a [motionhexa](https://www.tindie.com/products/starduststorm/the-hexagon/) — a motion-reactive, battery-powered hexagonal LED sculpture — without touching C++. Manage which patterns run, paint custom pixel fonts, tune device settings, and build & flash the firmware, all from one page in your browser.

This repo is the Config Tool first, and a firmware fork second: it bundles the full motionhexa firmware (originally by [starduststorm](https://github.com/starduststorm/motionhexa)) plus a couple dozen new LED patterns, mostly built to give the tool something real to edit and show off.

> This repository's descriptions and day-to-day upkeep are managed collaboratively with [Claude](https://claude.com) (Anthropic).

<img src="doc/assets/hexa_front.jpg" alt="photo of motionhexa pixels panel" height="320">
<img src="doc/assets/hexa_back_assembled.jpg" alt="photo of back of assembled motionhexa" height="320">

## What it does

- **Programs** — enable/disable and drag-to-reorder the pattern rotation, with a live thumbnail per pattern and a settings panel for anything it exposes (speed, particle count, audio sensitivity, etc.)
- **Fonts & Elements** — a full pixel editor for the hex-grid font and icon set used by on-device text, at three sizes (Small/Medium/Large)
- **System** — device name, default brightness, and charge-indicator style
- **Build & Deploy** — compiles with PlatformIO and flashes the connected device, streamed live, from the header of every page

Every edit writes straight to the real source files (`src/main.cpp`, `src/patterns.h`) — there's no separate config format to keep in sync. New settings just need a trailing tag comment (`@tunable`, `@tunable_text`, `@tunable_enum`, `@thumbnail`) on the constant they control; see [Adding a setting](#adding-a-setting) below.

## Quick start

Requires [Node.js](https://nodejs.org) (no npm install — the server has zero dependencies) and, for Build/Deploy, [PlatformIO](https://platformio.org)'s `pio` CLI.

```
git clone --recurse-submodules https://github.com/freetobelee/motionhexa.git
cd motionhexa
node console/server.js
```

Then open `http://localhost:2710`. On macOS you can also just double-click `console/Start Console.command`.

## Adding a setting

Four tag kinds, all trailing comments on the constant's own line — the scanner is generic, so a new tag of an existing kind needs no server code changes. It currently reads `main.cpp`, `power.h`, and `patterns.h`.

```cpp
// Slider — general Config panel, or a specific program's settings with a 4th arg
const int kTriBounceSpeed = 70; // @tunable("Speed", 10, 200, "TriBounce")

// Text field — System tab only
const char *kDeviceName = "motionhexa"; // @tunable_text("Device Name")

// Labeled choice — an int that's really an index into named options
const int kChargeIndicatorStyle = 0; // @tunable_enum("Charge Indicator", "Gradient Fill", "Percentage", "Pie Fill", "Original")

// Thumbnail — on a pattern's own class line, real colors from its own code
class TriBounce : public BouncyPixels { // @thumbnail("#FF0000", "#80FF00", "#00FFFF", "#8000FF")
```

If the value you want to tag is an inline literal (a constructor argument, say) rather than a named constant, pull it out into one first.

## Writing a new pattern

Subclass `Pattern` in `src/patterns.h`:

```cpp
class MyAwesomePattern : public Pattern {
public:
  void update() {
    for (int px = 0; px < ctx.leds.size(); ++px) {
      ctx.leds[px] = CHSV(10 * px - millis() / 10, 0xFF, 0xFF);
    }
  }
  const char *description() { return "MyAwesomePattern"; }
};
```

Then register it in `src/main.cpp`'s `setup()`:

```cpp
patternManager.registerPattern<MyAwesomePattern>();
```

It'll show up in the Config Tool's Programs list automatically. Registration order is playback order.

## Building the firmware

Built with [PlatformIO](https://platformio.org) — build the `v5` environment, which targets the MakerFaire2025 hardware revision.

```
git submodule update --init --recursive
pio run -e v5
```

Dependencies ([FastLED], [SparkFun_ICM-20948], [edrean/BQ27427 Battery Fuel Gauge], [dustlib]) are fetched automatically on first build.

### Recovering a device that won't boot

If the hexa stops responding and won't re-flash normally, put it into RP2040 USBBOOT mode:

1. Carefully open the case with a flat, blunt tool, avoiding the battery.
2. Find the USBBOOT button on the hexacontroller board:
   <img src="doc/assets/usb_boot.jpg" alt="location of USBBOOT button" height="180">
3. Hold it while plugging into USB — a "RPI-RP2" mass-storage drive should appear.
4. Either re-flash with PlatformIO, or copy [the stable release binary](bin/hexa-v5-firmware@ed502d04.uf2) onto the drive; it reboots automatically once the copy finishes.

## The hardware

RP2040 dual-core ARM, ICM-20948 motion sensor, LMD4030 PDM microphone, BQ27421 battery monitor, SK9822-EC20 pixels, USB-C rechargeable via an LP28013HQVF lipo charger. PCB sources are in `hexacontroller/` (logic/power/motion) and `hexa/` (the 271-pixel hex lattice).

A physical button (squeeze through the case) controls it directly: single click for next pattern, double click for previous, a 1-second hold to power on/off, a 10-second hold for a soft reset.

<details>
<summary>Disabled hardware &amp; known errata</summary>

- An ALS-PT19-315 ambient light sensor for autobrightness is disabled — its response time is too slow to subtract out light from nearby pixels during patterns. Might still work for a one-shot brightness read at startup.
- An MT3608-based 5V boost circuit is disabled — the pixels show no color-loss issues running directly on lipo voltage.
- **v5 errata:** the charging ring can remain lit after unplugging due to unexpected voltage on VBUS. Mitigated in `1e0326c` by fully powering off when the device is manually turned off.

</details>

## Roadmap / project direction

Config Tool:
- Add contextual instructions to the programs that need them (Clock, Solar System)
- Remove the "Saved" indicator text in the Fonts & Elements tool
- Make thumbnails real visual representations of what each program actually looks like, not just a color swatch
- Double-check the audio sensitivity setting actually works on the droplet programs
- Add a way to define which physical face is "the bottom" per program, for the programs that need it (Clock, Solar System, Timer, Glyph Viewer)

Firmware:
- The hex-font glyph library (used by the Fonts & Elements tool and programs like the font test) is still incomplete — a number of characters are still generic auto-generated defaults rather than hand-designed

[FastLED]: https://github.com/FastLED/FastLED
[SparkFun_ICM-20948]: https://github.com/sparkfun/SparkFun_ICM-20948_ArduinoLibrary
[edrean/BQ27427 Battery Fuel Gauge]: https://github.com/edreanernst/BQ27427_Arduino_Library
[dustlib]: https://github.com/starduststorm/dustlib
