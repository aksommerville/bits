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
- `hostio`: Wraps I/O drivers with abstract types.
- `simplifio`: Same idea but no generic driver types. More efficient and easier to understand, with some flexibility lost.
- `midi`: Read streams and files, and a bunch of helpful constants.
- `qjs`: QuickJS adapter.
- `wamr`: wasm-micro-runtime adapter.
- `http`: HTTP+WebSocket client and server, for dev tooling. Don't use on the public internet.


## TODO

- [x] wav
- [ ] Drivers with private interfaces
- - [ ] bcm
- - [ ] ossmidi
- - [ ] macioc
- - [ ] macwm
- - [ ] macaudio
- - [ ] machid
- - [ ] mswm
- - [ ] msaudio
- - [ ] mshid
