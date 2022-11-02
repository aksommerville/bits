/* midi.h
 * Helpers to read MIDI files and streams.
 * Tables of defines, from the spec.
 */

#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>

/* Event and stream reader.
 * Neither requires cleanup.
 * Both require input to remain constant while generated events are in play.
 ****************************************************************************/

struct midi_event {
  uint8_t chid; // 0..15
  uint8_t opcode; // with (chid) zeroed
  uint8_t a,b; // data bytes
  const void *v; // borrowed data for sysex and meta
  int c;
};

struct midi_stream_reader {
  uint8_t status;
  const void *v;
  int c;
  uint8_t buf[4];
  uint8_t bufc;
};

/* Call "more" with an arbitrary chunk of input off the wire.
 * Then "next" until it returns zero.
 * These are for I/O streams, not files.
 */
int midi_stream_reader_more(struct midi_stream_reader *reader,const void *src,int srcc);
int midi_stream_reader_next(struct midi_event *event,struct midi_stream_reader *reader);

/* File reader.
 *****************************************************************************/

struct midi_file {
  int refc;
  uint8_t *v;
  int c;
  int ownv;
  int output_rate;

  uint16_t format;
  uint16_t track_count;
  uint16_t division;

  struct midi_track {
    const uint8_t *v;
    int c;
    int term,termcapa;
    int p,pcapa;
    int delay,delaycapa; // <0 if not read yet
    uint8_t status,statuscapa;
  } *trackv;
  int trackc,tracka;
};

void midi_file_del(struct midi_file *file);
int midi_file_ref(struct midi_file *file);

struct midi_file *midi_file_new_copy(const void *src,int srcc);
struct midi_file *midi_file_new_borrow(const void *src,int srcc);
struct midi_file *midi_file_new_handoff(void *src,int srcc);

/* Tell us your driver's output rate and we'll take care of timing.
 * Defaults to 44100.
 */
int midi_file_set_output_rate(struct midi_file *file,int hz);

/* During playback, if you get some private Sysex or Meta event for the loop point, call this.
 * We will arrange to return to the current position (just *after* the most recently report event) at EOF.
 * Default is to fail at EOF.
 * When looping enabled, we always report a 1-frame delay just before the loop.
 */
int midi_file_set_loop_point(struct midi_file *file);

/* If an event is ready to fire, populates (event) and returns zero.
 * No event ready, returns frame count to next event.
 * <0 at EOF or error.
 * We can optionally report the track index the event was read from, though you shouldn't need it.
 */
int midi_file_next(struct midi_event *event,struct midi_file *file,int *trackp);

/* Advance our clock by so many frames.
 * This must not be more than the last thing returned by midi_file_next().
 */
int midi_file_advance(struct midi_file *file,int framec);

/* Symbols.
 ****************************************************************************/

//TODO Digest and copy from MIDI specs.

#endif
