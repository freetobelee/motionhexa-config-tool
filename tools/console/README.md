# Motionhexa Console

A local management tool for this project: a small Node server (no npm install
needed — zero dependencies) plus a browser UI. It reads and writes the real
project files on disk directly, and shells out to PlatformIO to build/flash.
It only runs on your machine; nothing here talks to Claude or the internet.

## Run it

```
node tools/console/server.js
```

Then open http://localhost:4173 in your browser. Leave the terminal window
open while you use it — closing it (or Ctrl+C) stops the server.

## What it does

- **Effects** — check/uncheck to enable or disable a pattern in the on-device
  rotation, and reorder with the ▲/▼ buttons. This edits the
  `patternManager.registerPattern<...>()` calls in `src/main.cpp` directly.
  Disabling a pattern here doesn't delete its code, just skips it (same as
  the `// X is left defined but unregistered for now` comments already in
  the file for a few patterns).
- **Config** — any constant tagged `@tunable("Label", min, max)` in a
  comment shows up here as a slider. Two are wired up already
  (`kDefaultBrightness`, `autoBrightness->maxBrightness` in `main.cpp`). To
  expose another constant, just add the tag to its line, e.g.:
  ```cpp
  const uint8_t kSomeValue = 42; // @tunable("Some Value", 0, 100)
  ```
  No server code changes needed — the scanner picks it up automatically. It
  currently scans `main.cpp`, `power.h`, and `patterns.h` (see
  `TUNABLE_FILES` in `server.js` to add more files).
- **Save changes** — writes your edits to the actual source files. Nothing
  takes effect on the device until you Build or Deploy.
- **Build only** — runs `pio run -e v5`, streaming live output below.
- **Build & Deploy to device** — runs `pio run -e v5 -t upload` (asks you to
  confirm first, since it flashes the physical device over USB).

## Safety notes

- Every write is a plain local file edit — use `git diff` / `git checkout --
  <file>` same as any other change if something looks wrong. This project
  wasn't a git repo before this tool was added; it is now, specifically so
  console-driven edits are easy to inspect and revert.
- The Effects panel only reorders/toggles patterns that are already present
  in `main.cpp`'s registration block — it refuses to write if the set of
  pattern names doesn't match exactly (protects against a stale browser tab
  clobbering a change made another way).
- The `lib/dustlib` and `lib/BQ27427_Arduino_Library` submodules need to be
  checked out (`git submodule update --init --recursive`) for Build/Deploy
  to succeed — same requirement as building this project any other way.
