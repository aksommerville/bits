/* http.h
 */
 
#ifndef HTTP_H
#define HTTP_H

struct http_context;
struct http_servlet;
struct http_xfer;
struct http_socket;
struct sr_encoder;

//TODO WebSocket

/* Global context.
 * Create one context for the program's life.
 * Both client and server can use it.
 *******************************************************************/

void http_context_del(struct http_context *ctx);
struct http_context *http_context_new();

/* Normally you'll call http_context_update, with an optional timeout in milliseconds.
 * That performs its own poll against the context's open files.
 * If you want to share a poll with other units, use populate_pollfd and update_file instead.
 */
int http_context_update(struct http_context *ctx,int toms);
int http_context_populate_pollfd(struct pollfd *dst,int dsta,const struct http_context *ctx);
int http_context_update_file(struct http_context *ctx,int fd);

/* Add a server socket on loopback (local_only) or any interface.
 * We don't normally distinguish connections by port.
 * TCP only.
 */
int http_context_listen(struct http_context *ctx,int local_only,int port);
void http_context_unlisten(struct http_context *ctx,int port);

/* Attach a new servlet, returns WEAK reference.
 * Normally you only check null/not-null response. But can use it to configure further.
 */
struct http_servlet *http_context_add_servlet(
  struct http_context *ctx,
  const char *method, // null for any
  const char *path, // '*' matches anything but slash; '**' matches anything
  int (*serve)(struct http_xfer *req,struct http_xfer *rsp,struct http_servlet *servlet)
);

struct http_servlet *http_context_add_websocket_servlet(
  struct http_context *ctx,
  const char *path,
  void (*cb_connect)(struct http_socket *sock,struct http_servlet *servlet),
  void (*cb_disconnect)(struct http_socket *sock,struct http_servlet *servlet),
  void (*cb_message)(struct http_socket *sock,int opcode,const void *v,int c,struct http_servlet *servlet)
);

/* Client requests.
 ********************************************************************/

/* Attach a new request, returns WEAK reference.
 * With no further configuration, it will go out as GET with no extra headers.
 * We resolve the remote host and connect during this call, but do not send anything.
 * So you still have an opportunity to modify the request before it encodes.
 * If you need additional context, use (req->userdata).
 */
struct http_xfer *http_context_add_request(
  struct http_context *ctx,
  const char *url,int urlc,
  int (*cb)(struct http_xfer *req,struct http_xfer *rsp),
);

void http_context_drop_request(struct http_context *ctx,struct http_xfer *req);

struct http_socket *http_context_connect_websocket(
  const char *url,int urlc,
  void (*cb_connect)(struct http_socket *sock),
  void (*cb_disconnect)(struct http_socket *sock),
  void (*cb_message)(struct http_socket *sock,int opcode,const void *v,int c)
);

/* Sockets, ie WebSocket.
 *****************************************************************/
 
void http_socket_close(struct http_socket *sock);

// Queues for delivery. No I/O at this point.
int http_socket_send(struct http_socket *sock,int opcode,const void *v,int c);

void http_socket_set_userdata(struct http_socket *sock,void *userdata);
void *http_socket_get_userdata(const struct http_socket *sock);

/* Transfers.
 *****************************************************************/

/* Normally, we expect servlets to complete their responses synchronously.
 * Even if you have to read a file or whatever, just block on it, it's fine.
 * But some things, like calls out to some other HTTP server, really do need to wait.
 * In that case, you must 'defer' during the 'serve' callback, 
 * and later by mechanisms not defined here, 'ready' to queue it for delivery.
 */
int http_context_defer_response(struct http_context *ctx,struct http_xfer *rsp);
int http_context_ready_response(struct http_context *ctx,struct http_xfer *rsp);
 
int http_xfer_get_topline(void *dstpp,const struct http_xfer *xfer);
int http_xfer_set_topline(struct http_xfer *xfer,const char *src,int srcc);

int http_xfer_for_each_header(const struct http_xfer *xfer,int (*cb)(const char *k,int kc,const char *v,int vc,void *userdata),void *userdata);
int http_xfer_add_header(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc); // appends blindly

/* Triggers for every query parameter, and depending on Content-Type, may enter body too.
 * For efficiency's sake, neither key nor value get decoded first.
 * Check whether you want the key, and if so decode the value yourself.
 * I recommend don't decode keys at all -- if they send "us%65rname" instead of "username", we can plausibly plead ignorance.
 */
int http_xfer_for_each_param(
  const struct http_xfer *xfer,
  int (*cb)(const char *k,int kc,const char *v,int vc,void *userdata),
  void *userdata
);

struct sr_encoder *http_xfer_get_body(struct http_xfer *xfer);

/* Convenient xfer accessors, built on the primitives above.
 * Note that if you take multiple query parameters, it's way more efficient to 'for_each' them, and decode yourself.
 */
int http_xfer_get_request_method(void *dstpp,const struct http_xfer *xfer);
int http_xfer_get_request_path(void *dstpp,const struct http_xfer *xfer);
int http_xfer_get_request_query(void *dstpp,const struct http_xfer *xfer);
int http_xfer_get_response_status(const struct http_xfer *xfer); // => status
int http_xfer_get_response_message(void *dstpp,const struct http_xfer *xfer);
int http_xfer_get_header(void *dstpp,const struct http_xfer *xfer,const char *k,int kc); // first match only
int http_xfer_get_header_int(int *v,const struct http_xfer *xfer,const char *k,int kc);
int http_xfer_set_header(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc); // replace or append
int http_xfer_get_param(void *dstpp,const struct http_xfer *xfer,const char *k,int kc); // decodes into a private buffer, will get overwritten
int http_xfer_get_param_int(int *v,const struct http_xfer *xfer,const char *k,int kc);

void http_xfer_set_userdata(struct http_xfer *xfer,void *userdata);
void *http_xfer_get_userdata(const struct http_xfer *xfer);

#endif
