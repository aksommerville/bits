# AK Sommerville's Bits

Reusable code I can yoink from for future projects.

Trying to keep everything portable. Ideally each directory anywhere under `src` can compile alone.

Any unit with additional concerns (libraries etc) should have a `README.md`.

## Overview

- src/serial: Encoder and decoder. JSON, text, hashing.
- src/format: File formats.
- - png
- - midi
- src/platform: Hardware interfaces peculiar to a platform.
- - hw: Generic interface layer. Manages driver selection, makes the drivers look alike, etc.
- - macos
- - - macioc: Requires plist and nib, provides AppDelegate and main.
- - - macwm: Single window with or without GX.
- - - macaudio: PCM out
- - - machid: HID in
- - linux
- - - x11
- - - drm
- - - alsa
- - - pulse
- - - evdev
- - genioc: Generic IoC interface for platforms that don't actually need one.

## TODO

- [x] platform/macos/macwm/AKOpenGLView
- [ ] platform/macos/macwm/AKMetalView
- [ ] MacOS MIDI-In
- [ ] macioc: I added arg() to genioc, should there be a corrollary in macioc?
- [ ] hw glue for linux io units
