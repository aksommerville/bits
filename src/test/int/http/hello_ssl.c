/* Trying out OpenSSL, with a mind to incorporate in our 'http' unit.
 * That integration would be complex at best.
 * I'm not able to connect to aksommerville.com, and that's extremely troubling.
 * Going to try replacing 'http' with libcurl instead, see if that suits our needs better.
 */

int hello_ssl_dummy=0;
#if 0

#include "test/test.h"
#include <unistd.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

IGNORETEST(hello_ssl) {
  fprintf(stderr,"%s...\n",__func__);
  
  // Following this example: https://fm4dd.com/openssl/sslconnect.htm
  
  OpenSSL_add_all_algorithms();
  //ERR_load_BIO_strings(); // deprecated at 3.0, no alternative suggested
  ERR_load_crypto_strings();
  SSL_load_error_strings();
  
  ASSERT_CALL(SSL_library_init())
  
  const SSL_METHOD *method=SSLv23_client_method();
  //const SSL_METHOD *method=DTLS_client_method();
  SSL_CTX *ctx=SSL_CTX_new(method);
  ASSERT(ctx);
  
  SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
  
  SSL *ssl = SSL_new(ctx);
  ASSERT(ssl)
  
  int fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
  /**/
  struct sockaddr_in saddr={
    .sin_family=AF_INET,
    .sin_port=htons(443),
  };
  /**/
  /**
  struct sockaddr_in6 saddr={
    .sin6_family=AF_INET6,
    .sin6_port=htons(443),
  };
  /**/
  //const uint8_t ipv6addr[]={0x26,0x07,0xf1,0xc0,0x10,0x0f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x42};
  //const uint8_t ipv4addr[]={74,208,236,36}; // doesn't work
  //#define HOST "aksommerville.com"
  const uint8_t ipv4addr[]={142,250,191,100}; // works
  #define HOST "www.google.com"
  //const uint8_t ipv4addr[]={208,80,154,224}; // works
  //#define HOST "en.wikipedia.org"
  memcpy(&saddr.sin_addr,ipv4addr,4);
  ASSERT_CALL(connect(fd,(struct sockaddr*)&saddr,sizeof(saddr)))
  fprintf(stderr,"Established TCP connection to %s.\n",HOST);
  
  BIO *bio=BIO_new_fd(fd,0);//TODO close_flag?
  ASSERT(bio)
  
  //SSL_set_fd(ssl,fd);
  SSL_set_bio(ssl,bio,bio);
  
  //SSL_set_connect_state(ssl);
  int err=SSL_connect(ssl);
  fprintf(stderr,"SSL_connect: %d\n",err);
  {
    const char *file=0,*func=0,*data=0;
    int line=0,flags=0;
    ERR_peek_error_all(&file,&line,&func,&data,&flags);
    fprintf(stderr,"ERR: file=%s line=%d func=%s data=%s flags=%d\n",file,line,func,data,flags);
    /* ERR: file=../ssl/record/rec_layer_s3.c line=1584 func=ssl3_read_bytes data=SSL alert number 80 flags=3
/usr/include/openssl/obj_mac.h:2209:#define NID_desx_cbc            80
/usr/include/openssl/tls1.h:66:# define TLS1_AD_INTERNAL_ERROR          80/* fatal *
/usr/include/openssl/bio.h:171:# define BIO_CTRL_SET_INDENT                    80
/usr/include/openssl/ssl.h:1334:# define SSL_CTRL_SET_TLS_EXT_SRP_STRENGTH               80
/usr/include/openssl/x509_vfy.h:399:# define X509_V_ERR_PATHLEN_INVALID_FOR_NON_CA           80
    */
  }
  ASSERT_INTS(err,1,"SSL_connect %d",SSL_get_error(ssl,err))
  
  const char req[]="GET / HTTP/1.1\r\nHost: "HOST"\r\n\r\n";
  err=SSL_write(ssl,req,sizeof(req));
  fprintf(stderr,"SSL_write(HTTP request, %d bytes): %d\n",(int)sizeof(req),err);
  
  char rsp[4096];
  err=SSL_read(ssl,rsp,sizeof(rsp));
  fprintf(stderr,"SSL_read: %d\n",err);
  if ((err>0)&&(err<=sizeof(rsp))) {
    fprintf(stderr,"=== received from https://%s:443/ ===\n%.*s\n",HOST,err,rsp);
  }
  
  SSL_free(ssl);
  close(fd);
  SSL_CTX_free(ctx);
  fprintf(stderr,"...normal exit\n");
  return 0;
}

#endif
