/* synth_minor.h
 * A trivial synthesizer.
 * Requires our unit format/midi.
 */
 
#ifndef SYNTH_MINOR_H
#define SYNTH_MINOR_H

#include <stdint.h>

struct synth_minor;
struct midi_event;

void synth_minor_del(struct synth_minor *synth);
struct synth_minor *synth_minor_new(int rate,int chanc);

void synth_minor_update(int16_t *v,int c,struct synth_minor *synth);

void synth_minor_event(struct synth_minor *synth,const struct midi_event *event);

#endif
