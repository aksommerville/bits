#include "test/test.h"
#include "opt/curlwrap/curlwrap.h"
#include <unistd.h>
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

/* HTTP GET and POST.
 * These will call out to websites on the real internet, and also to our dev server.
 */
 
struct httpget_context {
  int finishc;
  int expectc;
};
 
static void httpget_cb_http_rsp(struct curlwrap *cw,int reqid,int status,int length) {
  struct httpget_context *ctx=curlwrap_get_userdata(cw);
  char *src=0;
  int srcc=curlwrap_http_get_body(&src,cw,reqid);
  if (srcc<0) srcc=0;
  else if (srcc>100) srcc=100;
  int i=srcc; while (i-->0) if ((src[i]<0x20)||(src[i]>0x7e)) src[i]=' ';
  const char *type=0;
  int typec=curlwrap_http_get_header(&type,cw,reqid,"content-type",-1);
  if (typec<0) typec=0;
  fprintf(stderr,"%s %d type='%.*s' len=%d body='%.*s...'\n",__func__,status,typec,type,length,srcc,src);
  ctx->finishc++;
}
 
XXX_ITEST(curlwrap_http,curlwrap) {
  fprintf(stderr,"%s...\n",__func__);
  
  struct httpget_context ctx={
    .finishc=0,
    .expectc=0,
  };
  struct curlwrap_delegate delegate={
    .userdata=&ctx,
    .cb_http_rsp=httpget_cb_http_rsp,
  };
  struct curlwrap *cw=curlwrap_new(&delegate);
  ASSERT(cw)
  
  #define REQ(method,url,body,bodyc) { \
    int reqid=curlwrap_http_request(cw,method,url,body,bodyc); \
    ASSERT_CALL(reqid,"curlwrap_http_request(%s,%s,%d)",method,url,bodyc) \
    ctx.expectc++; \
  }
  REQ("GET","http://aksommerville.com",0,0)
  REQ("GET","https://en.wikipedia.org/",0,0)
  REQ("POST","http://localhost:8080/echo","This is the client's message.",29)
  #undef REQ
  
  int repc=0;
  for (;;) {
    if (++repc>=100) {
      FAIL("Failed to complete after 100 cycles")
    }
    ASSERT_CALL(curlwrap_update(cw))
    if (ctx.finishc>=ctx.expectc) {
      fprintf(stderr,"%d requests complete, after %d cycles\n",ctx.finishc,repc);
      break;
    }
    usleep(10000);
  }
  
  curlwrap_del(cw);
  fprintf(stderr,"%s: Normal exit\n",__func__);
  return 0;
}

/* WebSocket.
 * Requires Egg's dev server.
 */
 
#define WSLIMIT 8
 
struct ws_context {
  int pendingv[WSLIMIT];
  int pendingc;
  int livev[WSLIMIT];
  int livec;
};

static int wslist_add(int *v,int c,int n) {
  int i=c; while (i-->0) {
    if (v[i]==n) return c;
  }
  if (c>=WSLIMIT) return c;
  v[c++]=n;
  return c;
}

static int wslist_remove(int *v,int c,int n) {
  int i=c; while (i-->0) {
    if (v[i]==n) {
      c--;
      memmove(v+i,v+i+1,sizeof(int)*(c-i));
      return c;
    }
  }
  return c;
}

static void cb_ws_connect(struct curlwrap *cw,int wsid) {
  struct ws_context *ctx=curlwrap_get_userdata(cw);
  fprintf(stderr,"%s wsid=%d\n",__func__,wsid);
  ctx->pendingc=wslist_remove(ctx->pendingv,ctx->pendingc,wsid);
  ctx->livec=wslist_add(ctx->livev,ctx->livec,wsid);
  int err=curlwrap_ws_send(cw,wsid,1,"Hi I'm the client.",18);
  if (err<0) fprintf(stderr,"*** curlwrap_ws_send failed ***\n");
  else fprintf(stderr,"Sent an 18-byte hello to the server.\n");
}

static void cb_ws_disconnect(struct curlwrap *cw,int wsid) {
  struct ws_context *ctx=curlwrap_get_userdata(cw);
  fprintf(stderr,"%s wsid=%d\n",__func__,wsid);
  ctx->pendingc=wslist_remove(ctx->pendingv,ctx->pendingc,wsid);
  ctx->livec=wslist_remove(ctx->livev,ctx->livec,wsid);
}

static void cb_ws_message(struct curlwrap *cw,int wsid,int msgid,int length) {
  struct ws_context *ctx=curlwrap_get_userdata(cw);
  const char *src=0;
  int srcc=curlwrap_ws_get_message(&src,cw,wsid,msgid);
  int loggable=1;
  if ((srcc<0)||(srcc>100)) loggable=0;
  else {
    int i=srcc; while (i-->0) if ((src[i]<0x20)||(src[i]>0x7f)) { loggable=0; break; }
  }
  if (loggable) {
    fprintf(stderr,"%s wsid=%d msgid=%d length=%d '%.*s'\n",__func__,wsid,msgid,length,srcc,src);
  } else {
    fprintf(stderr,"%s wsid=%d msgid=%d length=%d srcc=%d\n",__func__,wsid,msgid,length,srcc);
  }
}
 
XXX_ITEST(curlwrap_websocket,curlwrap) {
  fprintf(stderr,"%s...\n",__func__);
  
  signal(SIGINT,rcvsig);
  
  struct ws_context ctx={
    .pendingc=0,
    .livec=0,
  };
  struct curlwrap_delegate delegate={
    .userdata=&ctx,
    .cb_ws_connect=cb_ws_connect,
    .cb_ws_disconnect=cb_ws_disconnect,
    .cb_ws_message=cb_ws_message,
  };
  struct curlwrap *cw=curlwrap_new(&delegate);
  ASSERT(cw)
  
  #define REQ(url) { \
    int wsid=curlwrap_ws_connect(cw,url); \
    ASSERT_CALL(wsid,"curlwrap_ws_connect(%s)",url) \
    ctx.pendingv[ctx.pendingc++]=wsid; \
    fprintf(stderr,"Established wsid=%d to %s\n",wsid,url); \
  }
  REQ("ws://localhost:8080/ws")
  #undef REQ
  
  int rmclock=3000;
  while (!sigc) {
    ASSERT_CALL(curlwrap_update(cw))
    if (!ctx.pendingc&&!ctx.livec) {
      fprintf(stderr,"All sockets closed.\n");
      break;
    }
    usleep(10000);
    if (rmclock--<=0) {
      rmclock=3000;
      if (ctx.pendingc>0) {
        ctx.pendingc--;
        int wsid=ctx.pendingv[ctx.pendingc];
        fprintf(stderr,"Closing pending connection %d due to rmclock.\n",wsid);
        curlwrap_ws_disconnect(cw,wsid);
      } else if (ctx.livec>0) {
        ctx.livec--;
        int wsid=ctx.livev[ctx.livec];
        fprintf(stderr,"Closing live connection %d due to rmclock.\n",wsid);
        curlwrap_ws_disconnect(cw,wsid);
      }
    }
  }
  
  curlwrap_del(cw);
  fprintf(stderr,"%s: Normal exit\n",__func__);
  return 0;
}
