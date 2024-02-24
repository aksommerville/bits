/* qjs.h
 * Adapter to QuickJS.
 * API is deliberately similar to 'wamr.h'.
 */
 
#ifndef QJS_H
#define QJS_H

#include <stdint.h>

struct qjs;

void qjs_del(struct qjs *qjs);

struct qjs *qjs_new();

int qjs_set_exports(struct qjs *qjs,int TODO);

int qjs_add_module(struct qjs *qjs,int modid,const void *v,int c,const char *refname);
int qjs_link_function(struct qjs *qjs,int modid,int fnid,const char *name,int namec);

/* Return value goes in (argv[0]).
 */
int qjs_call(struct qjs *qjs,int modid,int fnd,uint32_t *argv,int argc);

#endif
