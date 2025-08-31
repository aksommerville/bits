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
- `gcfg`: DEPRECATED Manage global config files, esp input devices. I want to get off per-program input config.
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

- [x] Revise gcfg+inmgr... Use the same button symbols, including saving the config file, other things added in Romassist.
- - Make it just one unit.
- [x] Integrate existing games with inmgr
- - ...ok, I've taken that about as far as I care to. The ones remaining look complicated. We've got Shovel, Romassist, and the last two Eggs, so that's most games.
- - [x] shovel +inmgr
- - - [x] opener
- - - [x] ninelives
- - [x] ra3
- - [x] egg2 +inmgr
- - [x] egg1
- - [x] pokorc
- - [x] lilsitter
- - [x] ivand
- - [ ] chetyorska... not bothering; it's really meant for MIDI only.
- - [ ] ctm... going to be tricky
- - [x] ecom
- - [ ] fullmoon4... tricky
- - [ ] pebble
- - [ ] plundersquad... tricky
- - [ ] sitter2009... oh dear is this tricky
- - [ ] tooheavy... PUNT. I'm strongly leaning toward rewriting this, once egg2 is stable. Currently runs on egg-202405, where it runs at all. Don't touch it, don't breathe on it.
- - [ ] ttaq... Lots of quirky abstractions here already, it's going to be painful.
