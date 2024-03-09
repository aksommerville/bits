#include "http_internal.h"

/* Delete.
 */
 
void http_websocket_del(struct http_websocket *ws) {
  if (!ws) return;
  free(ws);
}

/* New.
 */
 
struct http_websocket *http_websocket_new(struct http_context *ctx) {
  struct http_websocket *ws=calloc(1,sizeof(struct http_websocket));
  if (!ws) return 0;
  ws->ctx=ctx;
  return ws;
}

/* Disconnect.
 */
 
void http_websocket_disconnect(struct http_websocket *ws) {
  if (!ws) return;
  fprintf(stderr,"%s TODO\n",__func__);
}

/* Send.
 */

int http_websocket_send(struct http_websocket *ws,int opcode,const void *v,int c) {
  fprintf(stderr,"%s TODO c=%d\n",__func__,c);
  return -1;
}

/* Trivial accessors.
 */

void *http_websocket_get_userdata(const struct http_websocket *ws) {
  return ws->userdata;
}

void http_websocket_set_userdata(struct http_websocket *ws,void *userdata) {
  ws->userdata=userdata;
}

void http_websocket_set_callback(struct http_websocket *ws,int (*cb)(struct http_websocket *ws,int opcode,const void *v,int c)) {
  ws->cb=cb;
}
