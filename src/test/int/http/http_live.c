#include "test/test.h"
#include "http/http.h"
#include "serial/serial.h"
#include <signal.h>

/* Signal handler.
 */
 
static volatile int sigc=0;

static void rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Client HTTP request.
 */
 
static int http_client_request_complete=0;

static int http_client_request_cb_header(const char *k,int kc,const char *v,int vc,void *userdata) {
  fprintf(stderr,"  %.*s: %.*s\n",kc,k,vc,v);
  return 0;
}

static int http_client_request_cb(struct http_xfer *req,struct http_xfer *rsp) {

  struct sr_encoder *body=http_xfer_get_body(rsp);
  int status=http_xfer_get_status(rsp);
  const char *msg=0;
  int msgc=http_xfer_get_message(&msg,rsp);
  if (msgc<0) msgc=0;
  fprintf(stderr,"Got response. status=%d message='%.*s' bodyc=%d\n",status,msgc,msg,body->c);
  
  http_xfer_for_each_header(rsp,http_client_request_cb_header,0);
  
  if (body->c>1024) {
    fprintf(stderr,"%.1024s\n...truncated\n",(char*)body->v);
  } else {
    fprintf(stderr,"%.*s\n",body->c,(char*)body->v);
  }

  http_client_request_complete=1;
  return 0;
}
 
XXX_ITEST(http_client_request) {
  signal(SIGINT,rcvsig);
  
  struct http_context *ctx=http_context_new(0);
  ASSERT(ctx)
  
  const char *url="http://aksommerville.com/";//"http://localhost:9080/";
  fprintf(stderr,"Sending request for %s...\n",url);
  struct http_xfer *req=http_request(ctx,"GET",url,-1,http_client_request_cb,0);
  ASSERT(req)
  
  fprintf(stderr,"Waiting for response...\n");
  while (!http_client_request_complete&&!sigc) {
    ASSERT_CALL(http_update(ctx,100))
  }
  
  ASSERT(http_client_request_complete)
  fprintf(stderr,"Normal exit.\n");
  http_context_del(ctx);
  return 0;
}

/* HTTP server.
 * A quick test to validate deferred service:
 *  - GET /defer : Waits for content.
 *  - POST /content : Sends body to all currently deferred requests.
 * Any other GET returns some text synchronously.
 */
 
#define DEFER_LIMIT 16
static struct http_xfer *deferv[DEFER_LIMIT];
static int deferc=0;

static int http_server_cb_get_defer(struct http_xfer *req,struct http_xfer *rsp) {
  if (deferc>=DEFER_LIMIT) return -1;
  deferv[deferc++]=rsp;
  return 0;
}

static int http_server_cb_post_content(struct http_xfer *req,struct http_xfer *rsp) {
  struct sr_encoder *content=http_xfer_get_body(req);
  while (deferc>0) {
    deferc--;
    if (sr_encode_raw(http_xfer_get_body(deferv[deferc]),content->v,content->c)<0) return -1;
    if (http_xfer_set_status(deferv[deferc],200,"OK Albeit A Bit Late")<0) return -1;
  }
  if (http_xfer_set_status(rsp,201,"Delivered")<0) return -1;
  return 0;
}
 
static int http_server_cb_get_default(struct http_xfer *req,struct http_xfer *rsp) {
  struct sr_encoder *body=http_xfer_get_body(rsp);
  if (sr_encode_raw(body,"Hello I am a web server.\n",-1)<0) return -1;
  if (http_xfer_set_header(rsp,"Content-Type",12,"text/plain",10)<0) return -1;
  if (http_xfer_set_status(rsp,200,"OK")<0) return -1;
  return 0;
}

static int http_server_cb_serve(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  return http_dispatch(req,rsp,
    HTTP_METHOD_GET,"/defer",http_server_cb_get_defer,
    HTTP_METHOD_POST,"/content",http_server_cb_post_content,
    HTTP_METHOD_GET,"",http_server_cb_get_default
  );
}
 
XXX_ITEST(http_server) {
  signal(SIGINT,rcvsig);
  
  struct http_context_delegate delegate={
    .cb_serve=http_server_cb_serve,
  };
  struct http_context *ctx=http_context_new(&delegate);
  ASSERT(ctx)
  
  int port=8080;
  ASSERT_CALL(http_listen(ctx,1,port))
  
  fprintf(stderr,"Running on port %d. SIGINT to quit.\n",port);
  while (!sigc) {
    ASSERT_CALL(http_update(ctx,1000))
  }
  
  fprintf(stderr,"Normal exit.\n");
  http_context_del(ctx);
  return 0;
}
