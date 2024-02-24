/* wamr.h
 * Adapter to wasm-micro-runtime.
 * API is deliberately similar to 'qjs.h'.
 */
 
#ifndef WAMR_H
#define WAMR_H

#include <stdint.h>

struct wamr;

void wamr_del(struct wamr *wamr);

struct wamr *wamr_new();

/* Type NativeSymbol, from wasm-micro-runtime.
 */
int wamr_set_exports(struct wamr *wamr,const void *symbolv,int symbolc);

int wamr_add_module(struct wamr *wamr,int modid,const void *src,int srcc,const char *refname);
int wamr_link_function(struct wamr *wamr,int modid,int fnid,const char *name,int namec);

/* Return value is written into (argv[0]).
 */
int wamr_call(struct wamr *wamr,int modid,int fnid,uint32_t *argv,int argc);

/* Return a pointer in our process space, to wasm pointer (waddr) in a given module.
 * Optionally assert that at least (reqc) bytes are available beyond it.
 */
void *wamr_validate_pointer(struct wamr *wamr,int modid,uint32_t waddr,int reqc);

#endif
