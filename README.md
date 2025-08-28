# Bits

Things we can copy, try to keep them self-contained.

- `fs`: Conveniences for files and directories.
- `serial`: Utilities for encoding and decoding general data. esp JSON.
- Image formats
- - `bmp`
- - `gif`
- - `ico`
- - `jpeg`: libjpeg adapter.
- - `png`
- - `qoi`
- - `rlead`: My own invention, for black-and-white images.
- - `rawimg`: Wraps the others, and also a generic format of its own.
- I/O drivers
- - `alsafd`
- - `asound`: libasound adapter.
- - `pulse`
- - `drmfb`
- - `drmgx`
- - `glx`
- - `x11fb`
- - `evdev`
- - `ossmidi`: MIDI-In for Linux, doesn't actually use OSS.
- `hostio`: Wraps I/O drivers with abstract types.
- `simplifio`: Same idea but no generic driver types. More efficient and easier to understand, with some flexibility lost.
- `midi`: Read streams and files, and a bunch of helpful constants.
- `wav`: Encode and decode WAV files.
- `qjs`: QuickJS adapter.
- `wamr`: wasm-micro-runtime adapter.
- `http`: HTTP+WebSocket client and server, for dev tooling. Don't use on the public internet.
- `curlwrap`: Wrapper around libcurl for HTTP and (experimental) WebSocket. Safe for real-world use.
- `gcfg`: Manage global config files, esp input devices. I want to get off per-program input config.
- `inmgr`: Generic input manager, savvy to gcfg, producing multiple 16-bit player states.

## TODO

- [x] wav
- [x] Drivers with private interfaces
- - [x] bcm
- - [x] ossmidi
- - [x] macioc
- - [x] macwm
- - [x] macaudio
- - [x] machid
- - [x] mswm
- - [x] msaudio
- - [x] mshid
- [ ] Windows: No poll(). 'http' unit depends on it.
- [ ] Windows: Failing to link.
- [ ] Windows: qjs, wamr, jpeg
- [ ] MacOS: qjs, wamr, jpeg
- [ ] MacOS: Incorporate IoC in hostio? How is this going to work?

- [ ] Integrate existing games with gcfg (and inmgr if we don't already have that managed):
- - [x] shovel +inmgr
- - - [ ] opener
- - - [ ] ninelives
- - [ ] ra3
- - [ ] egg2 +inmgr
- - [ ] egg1
- - [ ] pokorc
- - [ ] lilsitter
- - [ ] ivand
- - [ ] chetyorska
- - [ ] ctm
- - [ ] ecom
- - [ ] fullmoon4
- - [ ] pebble
- - [ ] plundersquad
- - [ ] sitter2009
- - [ ] tooheavy
- - [ ] ttaq
