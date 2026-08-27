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
  for editing effects/config/fonts. Install with `pipx install platformio`
  (or see [platformio.org](https://platformio.org)).
- The project's git submodules need to be checked out —
  `git submodule update --init --recursive` from the project root — same
  requirement as building this project any other way.

Nothing else is required. Cloning this repo fresh with
`git clone --recurse-submodules <url>` handles the last point automatically.

## What it does

### Effects & Config (`/`)

- **Effects** — drag rows to reorder, check/uncheck to enable or disable a
  pattern in the on-device rotation. This edits the
  `patternManager.registerPattern<...>()` calls in `src/main.cpp` directly.
  Disabling a pattern doesn't delete its code, just skips it. Unchecking an
  effect drops it to the bottom of the list; rechecking it snaps back to its
  original spot — the underlying order never actually changes on toggle, only
  the enabled flag, so nothing needs to "remember" a position.
- **Per-effect settings** — the ⚙ button on a row opens that effect's own
  tunables (e.g. TriBounce's speed, PixelDust/PixelSand's particle count,
  audio sensitivity on the patterns that react to sound). Effects with
  nothing exposed yet show a disabled ⚙.
- **Thumbnails** — each effect gets a small abstract generated icon (colors
  and shape derived from its name) so the list is easier to scan. These are
  decorative, not a literal preview of the animation — there's no way to
  render what a pattern actually looks like without running it on the device.
- **Config** — general settings not tied to one effect (currently just
  Default Brightness, capped at the fixed hardware-safe ceiling).
- **Save changes** — writes your edits to the actual source files. Nothing
  takes effect on the device until you Build or Deploy.
- **Build only** — runs `pio run -e v5`, streaming live output below.
- **Build & Deploy to device** — runs `pio run -e v5 -t upload` (asks you to
  confirm first, since it flashes the physical device over USB).

### Fonts & Elements (`/forge.html`)

The full Hexa Object Forge pixel editor for the font/element bitmaps, now
saving straight to `src/patterns.h` — no more copying giant text blocks
through chat. Paint, hit **Save to patterns.h**, done. The Export/Import tabs
are still there as a manual fallback (e.g. to hand someone a text copy) but
aren't needed for normal use anymore.

## Adding a new tunable

Tag any numeric constant with a trailing comment:
```cpp
const uint8_t kSomeValue = 42; // @tunable("Some Value", 0, 100)
```
It shows up in the general Config panel automatically — no server code
changes needed. To tie it to a specific effect's settings page instead of the
general panel, add a 4th argument matching that pattern's class name:
```cpp
const int kTriBounceSpeed = 70; // @tunable("Speed", 10, 200, "TriBounce")
```
The scanner currently covers `main.cpp`, `power.h`, and `patterns.h` (see
`TUNABLE_FILES` in `server.js` to add more files). If the constant is an
inline literal (e.g. a constructor argument) rather than a named `const`,
pull it out into a named constant first — see the TriBounce/PixelDust/
PixelSand examples already in `patterns.h` for the pattern to copy.

## Safety notes

- Every write is a plain local file edit — use `git diff` / `git checkout --
  <file>` same as any other change if something looks wrong. This project
  wasn't a git repo before this tool was added; it is now, specifically so
  console-driven edits are easy to inspect and revert.
- The Effects panel only reorders/toggles patterns that are already present
  in `main.cpp`'s registration block — it refuses to write if the set of
  pattern names doesn't match exactly (protects against a stale browser tab
  clobbering a change made another way).
- The port is 2710 (271 LEDs ×10), not 271 — macOS blocks regular apps from
  binding to ports below 1024 without admin rights, so a literal 271 would
  need `sudo` and break the "just double-click it" goal. Override with
  `PORT=<number> node server.js` if you ever need a different one.
