#include "test/test.h"
#include "opt/http/http.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"
#include <signal.h>
#include <stdint.h>

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

/* WebSocket server.
 */
 
static int ws_server_cb_default(struct http_xfer *req,struct http_xfer *rsp) {
  char method[16];
  int methodc=http_xfer_get_method(method,sizeof(method),req);
  if ((methodc<1)||(methodc>=sizeof(method))) return -1;
  const char *path=0;
  int pathc=http_xfer_get_path(&path,req);
  if (pathc<0) return -1;
  fprintf(stderr,"%s %.*s %.*s\n",__func__,methodc,method,pathc,path);
  return -1;
}

static int ws_server_cb_get_html(struct http_xfer *req,struct http_xfer *rsp) {
  void *html=0;
  int htmlc=file_read(&html,"src/test/int/http/ws_client.html");
  if (htmlc<0) return -1;
  sr_encode_raw(http_xfer_get_body(rsp),html,htmlc);
  free(html);
  http_xfer_set_header(rsp,"Content-Type",12,"text/html",9);
  http_xfer_set_header(rsp,"Access-Control-Allow-Origin",-1,"*",1);
  http_xfer_set_status(rsp,200,"OK");
  return 0;
}

static int ws_server_cb_message(struct http_websocket *ws,int opcode,const void *v,int c) {
  if (opcode<0) {
    return 0;
  }
  int istext=1,i=c;
  while (i-->0) {
    uint8_t ch=((uint8_t*)v)[i];
    if ((ch<0x20)||(ch>0x7e)) {
      istext=0;
      break;
    }
  }
  if (istext) {
    fprintf(stderr,"%s[%d]: '%.*s'\n",__func__,opcode,c,(char*)v);
  } else {
    fprintf(stderr,"%s[%d]: %d bytes\n",__func__,opcode,c);
  }
  http_websocket_send(ws,1,"Server acknowledges.",20);
  return 0;
}

static int ws_server_cb_ws(struct http_xfer *req,struct http_xfer *rsp) {
  struct http_websocket *ws=http_websocket_check_upgrade(req,rsp);
  if (!ws) return -1;
  fprintf(stderr,"UPGRADED TO WEBSOCKET\n");
  http_websocket_set_callback(ws,ws_server_cb_message);
  return 0;
}
 
static int ws_server_cb_serve(struct http_xfer *req,struct http_xfer *rsp,void *userdata) {
  return http_dispatch(req,rsp,
    HTTP_METHOD_GET,"/ws_client.html",ws_server_cb_get_html,
    HTTP_METHOD_GET,"/ws",ws_server_cb_ws,
    0,"",ws_server_cb_default
  );
  return 0;
}

static int ws_server_send_random_message(struct http_context *ctx) {
  int p=0;
  for (;;p++) {
    struct http_websocket *websocket=http_context_get_websocket_by_index(ctx,p);
    if (!websocket) break;
    const char *msg="";
    switch (rand()%5) {
      case 0: msg="Hello this is a random message."; break;
      case 1: msg="This message is also random."; break;
      case 2: msg="This is not as random as the other messages."; break;
      case 3: msg="Congratulations, you found the hidden message."; break;
      case 4: msg="Hi."; break;
    }
    int msgc=0; while (msg[msgc]) msgc++;
    http_websocket_send(websocket,1,msg,msgc);
  }
  return 0;
}
 
XXX_ITEST(ws_server) {
  signal(SIGINT,rcvsig);
  
  struct http_context_delegate delegate={
    .cb_serve=ws_server_cb_serve,
  };
  struct http_context *ctx=http_context_new(&delegate);
  ASSERT(ctx)
  
  int port=8081;
  ASSERT_CALL(http_listen(ctx,1,port))
  
  fprintf(stderr,"Running on port %d. SIGINT to quit.\n",port);
  int msgclock=0;
  while (!sigc) {
    ASSERT_CALL(http_update(ctx,1000))
    if (++msgclock>=10) {
      msgclock=0;
      ws_server_send_random_message(ctx);
    }
  }
  
  fprintf(stderr,"Normal exit.\n");
  http_context_del(ctx);
  return 0;
}

/* WebSocket client.
 * Run this while the WebSocket server demo above is running.
 */
 
static struct http_websocket *myws=0;
 
static int cb_ws_client_message(struct http_websocket *ws,int opcode,const void *v,int c) {
  if ((opcode<0)&&(ws==myws)) {
    fprintf(stderr,"WebSocket disconnected.\n");
    myws=0;
  } else {
    fprintf(stderr,"IN: %.*s\n",c,(char*)v);
  }
  return 0;
}
 
XXX_ITEST(ws_client) {
  signal(SIGINT,rcvsig);
  
  double beacon_frequency=1.0;
  double recent_time=http_now();
  int beaconc=0;
  
  struct http_context *ctx=http_context_new(0);
  ASSERT(ctx)
  
  ASSERT(myws=http_websocket_connect(ctx,"http://localhost:8080/ws",-1,cb_ws_client_message,0))
  
  fprintf(stderr,"Will send a WebSocket packet every %.03f s until SIGINT...\n",beacon_frequency);
  while (!sigc&&myws) {
    ASSERT_CALL(http_update(ctx,500))
    if (!myws) break;
    double now=http_now();
    double elapsed=now-recent_time;
    if (elapsed>=beacon_frequency) {
      recent_time=now;
      char msg[128];
      int msgc=snprintf(msg,sizeof(msg),"This is message #%d from the client.",++beaconc);
      fprintf(stderr,"OUT: %.*s\n",msgc,msg);
      ASSERT_CALL(http_websocket_send(myws,1,msg,msgc))
    }
  }
  
  fprintf(stderr,"Normal exit.\n");
  http_context_del(ctx);
  return 0;
}
