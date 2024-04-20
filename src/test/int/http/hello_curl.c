/* If we brought in libcurl instead of our 'http' unit, what would that look like?
 * $ sudo apt install libcurl4-openssl-dev
 * Link: -lcurl
 * Other options: -nss- -gnutls-
 *
 * TODO:
 *  [x] HTTPS to aksommerville.com
 *  [x] Can we automatically follow redirects? (en.wikipedia.org redirects)
 *     YES: CURLOPT_FOLLOWLOCATION=1
 *  [ ] What happens if invalid certificate?
 *  [x] What happens on transport errors? Malformed URL might be enough to test?
 *     Request terminates sanely. Failure is not known at the time of adding.
 *     msg->data.result is a CURLcode describing the error.
 *  [x] Provide request method.
 *     Looks like we can't do arbitrary methods?
 *     CURLOPT_POST LONG
 *     CURLOPT_PUT LONG
 *     CURLOPT_HTTPGET LONG (redundant)
 *     [x] How does one send DELETE or PATCH?
 *     [x] I'd much prefer to specify freeform, is that possible?
 *       YES: curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
 *  [x] Provide request body.
 *     CURLOPT_READFUNCTION, same shape as WRITEFUNCTION
 *  [x] Read response headers and status.
 *     NB msg->data.result is zero on success, not the HTTP status.
 *     curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&status)==CURLE_OK
 *     Can set CURLOPT_HEADERDATA to receive Status-Line and headers via cb_data (arg is userp; and there's a "\r\n" before the body).
 *     Better yet: CURLOPT_HEADERFUNCTION lets you use a separate callback for the headers.
 *  [x] Can we tell libcurl to only allow http and https? If not, we can filter beforehand. *Must* forbid "file:///"
 *    YES: CURLOPT_PROTOCOLS,CURLPROTO_HTTP|CURLPROTO_HTTPS)) // FTP,FTPS,SCP,SFTP,FILE,SMB,SMBS
 *    Also CURLOPT_REDIR_PROTOCOLS
 *  [x] Confirm "file:///" could be used to read the local fs. ...confirmed. Be careful with CURLOPT_PROTOCOLS!
 *  [ ] WebSocket
 *  [ ] Do requests ever time out? And do we want them to? And other safety limits?
 *    CURLOPT(CURLOPT_TIMEOUT_MS, CURLOPTTYPE_LONG, 155),
 *    CURLOPT(CURLOPT_CONNECTTIMEOUT_MS, CURLOPTTYPE_LONG, 156),
 *    CURLOPT(CURLOPT_MAXFILESIZE, CURLOPTTYPE_LONG, 114),
 *  [x] Can the library manage cookies for us?
 *    CURLOPT(CURLOPT_COOKIEJAR, CURLOPTTYPE_STRINGPOINT, 82),
 *    ...but i think we don't want it to
 *  [ ] Client authentication? I'm not sure what we want out of that.
 *    CURLOPT_SSLCERT
 *
 * Conclusion:
 * We should use this, and abandon 'http'. Or, it's reasonable to keep http for cheap dev servers, it's really good at that.
 * There is one major concern, that curl_ws_send() blocks until completion and we don't have an async option.
 * But I'd rather live with *that* as the bug, as opposed to "no TLS".
 */
 
int hello_curl_dummy=0;
#if 0

#include "test/test.h"
#include <unistd.h>
#include <curl/curl.h>

static size_t hello_curl_cb_data(char *data,size_t n,size_t l,void *userp) {
  if ((n<0)||(l<0)||(n>INT_MAX)||(l>INT_MAX)||(l&&(n>INT_MAX/l))) return -1;
  int c=n*l;
  int printc=c;
  if (printc>100) printc=100;
  while (printc&&((unsigned char)data[printc-1]<=0x20)) printc--;
  fprintf(stderr,"%s: %d b userp=%p '%.*s'\n",__func__,c,userp,printc,data);
  return c;
}

static size_t hello_curl_cb_header(char *data,size_t n,size_t l,void *userp) {
  if ((n<0)||(l<0)||(n>INT_MAX)||(l>INT_MAX)||(l&&(n>INT_MAX/l))) return -1;
  int c=n*l;
  int printc=c;
  if (printc>100) printc=100;
  while (printc&&((unsigned char)data[printc-1]<=0x20)) printc--;
  fprintf(stderr,"%s: %d b userp=%p '%.*s'\n",__func__,c,userp,printc,data);
  return c;
}

  /* Data passed to the CURLOPT_PROGRESSFUNCTION and CURLOPT_XFERINFOFUNCTION
     callbacks */
  //CURLOPT(CURLOPT_XFERINFODATA, CURLOPTTYPE_CBPOINT, 57),

IGNORETEST(hello_curl) {
  fprintf(stderr,"%s...\n",__func__);
  
  // curl_global_sslset()?
  CURLcode err=curl_global_init(0);
  fprintf(stderr,"curl_global_init: %d\n",err);
  
  CURLM *curlm=curl_multi_init();
  ASSERT(curlm,"curl_multi_init")
  
  // Make the request.
  const char *url="https://aksommerville.com/";
  CURL *easy=curl_easy_init();
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_SSL_VERIFYHOST,2))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_SSL_VERIFYPEER,1))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_PROTOCOLS,CURLPROTO_HTTP|CURLPROTO_HTTPS)) // FTP,FTPS,SCP,SFTP,FILE,SMB,SMBS
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_REDIR_PROTOCOLS,CURLPROTO_HTTP|CURLPROTO_HTTPS))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_FOLLOWLOCATION,1))
  //ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_HEADERDATA,1))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_HEADERFUNCTION,hello_curl_cb_header))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_WRITEFUNCTION,hello_curl_cb_data))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_URL,url))
  curl_multi_add_handle(curlm,easy);//TODO Is this a handoff? Do we need to cleanup (easy) at some point?
  fprintf(stderr,"GET %s...\n",url);
  
  /* This is fine. *
  int ttlsec=3;
  fprintf(stderr,"Polling for up to %d seconds...\n",ttlsec);
  int ttl=ttlsec*10;
  while (ttl-->0) {
    int ret=0;
    CURLMcode perr=curl_multi_poll(curlm,0,0,100,&ret);
    if (perr||ret) fprintf(stderr,"curl_multi_poll perr=%d ret=%d\n",perr,ret);
  }
  /**/
  /* But I think this is neater. */
  fprintf(stderr,"Polling until curl goes idle...\n");
  for (;;) {
    int running_handles=0;
    CURLMcode perr=curl_multi_perform(curlm,&running_handles);
    
    CURLMsg *msg;
    int msgs_left=0;
    while (msg=curl_multi_info_read(curlm,&msgs_left)) {
      if (msg->msg==CURLMSG_DONE) {
        fprintf(stderr,"CURLMSG_DONE: Removing transfer. result=%d\n",msg->data.result);
        easy=msg->easy_handle;
        long status=0;
        if (curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&status)==CURLE_OK) {
          fprintf(stderr,"Status %d\n",(int)status);
        } else {
          fprintf(stderr,"Status not available.\n");
        }
        curl_multi_remove_handle(curlm,easy);
        curl_easy_cleanup(easy);
      } else {
        fprintf(stderr,"Unexpected CURLMSG: %d\n",msg->msg);
      }
    }
    
    if (perr||!running_handles) break;
    usleep(2000);
  }
  /**/
  
  curl_multi_cleanup(curlm);
  curl_global_cleanup();
  fprintf(stderr,"End of test.\n");
  return 0;
}

/*********************************** WebSocket *********************************
  Per libcurl docs, this is experimental and subject to change.
  They explicitly warn not to use in production code, but I'm doing it anyway.
  Server: ~/proj/egg$ make serve
  
  I'm getting CURLE_UNSUPPORTED_PROTOCOL (1) with no activity.
  
  Docs: 
    The WebSocket API was introduced as experimental in 7.86.0 and is still experimental today.
    It is only built-in if explicitly opted in at build time. We discourage use of the WebSocket API 
    in production because of its experimental state. We might change API, ABI and behavior before this "goes live".
  curl_version: libcurl/7.81.0 OpenSSL/3.0.2 zlib/1.2.11 brotli/1.0.9 zstd/1.4.8 libidn2/2.3.2 libpsl/0.21.0 (+libidn2/2.3.2) libssh/0.9.6/openssl/zlib nghttp2/1.43.0 librtmp/2.3 OpenLDAP/2.5.17
  
  Building libcurl from source instead.
  https://github.com/curl/curl
  cmake -DENABLE_WEBSOCKETS=ON -DBUILD_STATIC_LIBS=ON ..
  curl_version: libcurl/8.8.0-DEV OpenSSL/3.0.2 zlib/1.2.11
  
  *** curl_ws_send() is synchronous, and there doesn't seem to be an async alternative ***
  Curl team seems to be cognizant of the issue, of course they are, but not clear if it will be addressed: https://github.com/curl/curl/issues/12587
*/

static size_t curl_websocket_cb_data(char *data,size_t n,size_t l,void *userp) {
  CURL *easy=userp;
  int c=l;
  if ((l<0)||(l>INT_MAX)||(n!=1)) c=0;
  if (c>100) c=100;
  while (c&&((unsigned char)data[c-1]<=0x20)) c--;
  // There is curl_ws_meta() if we want to get the opcode.
  const struct curl_ws_frame *frame=curl_ws_meta(easy);
  int opcode=0;
  if (frame) {
    if (frame->flags&CURLWS_BINARY) opcode=2; else opcode=1;
  }
  fprintf(stderr,"%s data=%p n=%d l=%d userp=%p opcode=%d '%.*s'\n",__func__,data,(int)n,(int)l,userp,opcode,c,data);
  
  // And let's send a reply each time.
  size_t sent=0;
  int err=curl_ws_send(easy,"Hi I'm libcurl.",15,&sent,0,CURLWS_TEXT);
  fprintf(stderr,"curl_ws_send: err=%d sent=%d\n",err,(int)sent);
  
  return l;
}

static size_t curl_websocket_cb_read(char *data,size_t n,size_t l,void *userp) {
  fprintf(stderr,"%s data=%p n=%d l=%d userp=%p\n",__func__,data,(int)n,(int)l,userp);
  return 0;
}

IGNORETEST(curl_websocket) {
  fprintf(stderr,"%s...\n",__func__);
  fprintf(stderr,"curl_version: %s\n",curl_version());
  ASSERT_INTS(0,curl_global_init(0))
  CURLM *multi=curl_multi_init();
  ASSERT(multi,"curl_multi_init")
  
  CURL *easy=curl_easy_init();
  ASSERT(easy)
  curl_easy_setopt(easy,CURLOPT_URL,"ws://localhost:8080/ws");
  curl_easy_setopt(easy,CURLOPT_WRITEFUNCTION,curl_websocket_cb_data);
  curl_easy_setopt(easy,CURLOPT_READFUNCTION,curl_websocket_cb_read);
  //curl_easy_setopt(easy,CURLOPT_HEADERDATA,1);
  curl_easy_setopt(easy,CURLOPT_WRITEDATA,easy);
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_SSL_VERIFYHOST,2))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_SSL_VERIFYPEER,1))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_PROTOCOLS,CURLPROTO_HTTP|CURLPROTO_HTTPS|CURLPROTO_ALL)) // FTP,FTPS,SCP,SFTP,FILE,SMB,SMBS
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_REDIR_PROTOCOLS,CURLPROTO_HTTP|CURLPROTO_HTTPS))
  ASSERT_INTS(0,curl_easy_setopt(easy,CURLOPT_FOLLOWLOCATION,1))
  curl_multi_add_handle(multi,easy);
  fprintf(stderr,"WS request installed easy=%p\n",easy);
  
  int ttl=10000;
  for (;;) {
    if (ttl--<=0) {
      fprintf(stderr,"Terminating due to TTL\n");
      break;
    }
    int running_handles=0;
    CURLMcode perr=curl_multi_perform(multi,&running_handles);
    
    CURLMsg *msg;
    int msgs_left=0;
    while (msg=curl_multi_info_read(multi,&msgs_left)) {
      if (msg->msg==CURLMSG_DONE) {
        fprintf(stderr,"CURLMSG_DONE: Removing transfer. result=%d\n",msg->data.result);
        easy=msg->easy_handle;
        long status=0;
        if (curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&status)==CURLE_OK) {
          fprintf(stderr,"Status %d\n",(int)status);
        } else {
          fprintf(stderr,"Status not available.\n");
        }
        curl_multi_remove_handle(multi,easy);
        curl_easy_cleanup(easy);
      } else {
        fprintf(stderr,"Unexpected CURLMSG: %d\n",msg->msg);
      }
    }
    
    if (perr||!running_handles) break;
    usleep(2000);
  }
  
  curl_multi_cleanup(multi);
  curl_global_cleanup();
  return 0;
}

#endif
