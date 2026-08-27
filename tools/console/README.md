# Motionhexa Console

A local management tool for this project: a small Node server (no npm install
needed — zero dependencies) plus a browser UI. It reads and writes the real
project files on disk directly, and shells out to PlatformIO to build/flash.
It only runs on your machine; nothing here talks to Claude or the internet.

## Run it

**Easiest:** double-click `Start Console.command` in this folder. It opens a
Terminal window and your browser automatically.

**Or from a terminal:**
```
node tools/console/server.js
```
Then open http://localhost:2710 in your browser. Leave the terminal window
open while you use it — closing it (or Ctrl+C) stops the server.

### What you need installed first

- **Node.js** — not preinstalled on macOS. Get it from [nodejs.org](https://nodejs.org)
  (the LTS version). The double-click launcher will tell you plainly if it's
  missing instead of failing silently.
- **PlatformIO's `pio` CLI** — only needed for the Build/Deploy buttons, not
  for editing programs/config/fonts. Install with `pipx install platformio`
  (or see [platformio.org](https://platformio.org)).
- The project's git submodules need to be checked out —
  `git submodule update --init --recursive` from the project root — same
  requirement as building this project any other way.

Nothing else is required. Cloning this repo fresh with
`git clone --recurse-submodules <url>` handles the last point automatically.

## The three tabs

Build and Deploy are in the header on every tab, so you don't need to be on
any particular page to compile or flash.

### Programs (`/`)

- Drag the **handle** (⠿, left of the thumbnail) to reorder. Check/uncheck to
  enable or disable a program in the on-device rotation — this edits the
  `patternManager.registerPattern<...>()` calls in `src/main.cpp` directly.
  Disabling doesn't delete the code, just skips it. Unchecking a program
  drops it to the bottom of the list; rechecking it snaps back to its
  original spot — the underlying order never actually changes on toggle,
  only the enabled flag, so nothing needs to "remember" a position.
- The **disclosure triangle** on a row opens that program's own settings
  (e.g. TriBounce's speed, PixelDust/PixelSand's particle count, audio
  sensitivity on the programs that react to sound). Programs with nothing
  exposed yet show a dimmed, disabled triangle.
- **Thumbnails** are real per-program data, not generic placeholders: each
  one is a swatch of that program's own actual colors — its literal
  `CRGB`/`CHSV` constants, its own gradient palette stops, or (for the
  handful of programs that just draw from the shared random palette
  rotation with nothing of their own) a real sample from that same shared
  palette pool. See `@thumbnail(...)` below to add or change one.
- **Save changes** writes your edits to the actual source files. Nothing
  takes effect on the device until you Build or Deploy.

### System (`/system.html`)

- **Device Name** — a friendly string reported over USB serial when the
  device is asked to identify itself (handy if you own more than one hexa).
- **Brightness** — capped at the fixed hardware-safe ceiling; that ceiling
  itself isn't user-adjustable (it exists to keep the board from
  overheating).
- **Charge Indicator** — pick between two visual styles for how the device
  shows charging progress while plugged in (Ring: fills proportional to
  charge from the USB port; Pulse: the whole outer edge breathes at a
  brightness set by charge level). Adding a third style means writing the
  actual animation in `ChargingPattern` (`src/patterns.h`) and adding its
  name to the `@tunable_enum` options list — see below.

### Fonts & Elements (`/forge.html`)

The full Hexa Object Forge pixel editor for the font/element bitmaps, saving
straight to `src/patterns.h` — no more copying giant text blocks through
chat. Paint, hit **Save to patterns.h**, done. The Export/Import tabs are
still there as a manual fallback (e.g. to hand someone a text copy) but
aren't needed for normal use anymore.

## Adding a new setting

Four tag kinds, all just trailing comments on the constant's own line — the
scanner is generic, no server code changes needed for a new tag of a kind
that already exists. It currently reads `main.cpp`, `power.h`, and
`patterns.h` (see `TUNABLE_FILES` in `server.js` to add more files).

**Slider**, for a numeric constant — shows in the general Config panel,
or on a specific program's settings if you add its class name as a 4th
argument:
```cpp
const uint8_t kSomeValue = 42; // @tunable("Some Value", 0, 100)
const int kTriBounceSpeed = 70; // @tunable("Speed", 10, 200, "TriBounce")
```

**Text field**, for a `const char *` string constant — System tab only:
```cpp
const char *kDeviceName = "motionhexa"; // @tunable_text("Device Name")
```

**Labeled choice**, for an `int` whose value is really an index into a
fixed list of named options (branch on it in code, same as
`kChargeIndicatorStyle` in `patterns.h`):
```cpp
const int kChargeIndicatorStyle = 0; // @tunable_enum("Charge Indicator", "Ring", "Pulse")
```

**Thumbnail**, on a pattern's own `class Name : ...  {` line — 1 or more
real hex colors sourced from that pattern's own code (see the existing 21
tags in `patterns.h` for examples of what counts as "real": literal
`CRGB`/`CHSV` values, actual gradient palette stops, or — only when nothing
pattern-specific exists — a genuine sample from the shared palette pool the
pattern draws from):
```cpp
class TriBounce : public BouncyPixels { // @thumbnail("#FF0000", "#80FF00", "#00FFFF", "#8000FF")
```

If the value you want to tag is an inline literal (e.g. a constructor
argument) rather than a named `const`, pull it out into a named constant
first — see the TriBounce/PixelDust/PixelSand examples already in
`patterns.h` for the pattern to copy.

## Safety notes

- Every write is a plain local file edit — use `git diff` / `git checkout --
  <file>` same as any other change if something looks wrong. This project
  wasn't a git repo before this tool was added; it is now, specifically so
  console-driven edits are easy to inspect and revert.
- The Programs panel only reorders/toggles patterns that are already present
  in `main.cpp`'s registration block — it refuses to write if the set of
  pattern names doesn't match exactly (protects against a stale browser tab
  clobbering a change made another way).
- The port is 2710 (271 LEDs ×10), not 271 — macOS blocks regular apps from
  binding to ports below 1024 without admin rights, so a literal 271 would
  need `sudo` and break the "just double-click it" goal. Override with
  `PORT=<number> node server.js` if you ever need a different one.
