#include "test/test.h"
#include "img/rawimg/rawimg.h"
#include "fs/fs.h"
#include "serial/serial.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

/* Decode a bunch of images, then reencode into every available format.
 * Successful reencode, redecode those and verify we get the exact same pixels back.
 * Record encoder performance for each file and format.
 */
 
struct decode_images_context {
  const char **fmtv;
  int fmtc,fmta;
  char **pathv;
  int pathc,patha;
  struct decode_images_result {
    int serialc; // 0 if encode failed
    int match;
  } *resultv;
  // Length of (resultv) is (fmtc*pathc). (fmtv,pathv) are final before we start doing things.
};

static void decode_images_context_cleanup(struct decode_images_context *ctx) {
  if (ctx->fmtv) free(ctx->fmtv);
  if (ctx->pathv) {
    while (ctx->pathc-->0) free(ctx->pathv[ctx->pathc]);
    free(ctx->pathv);
  }
  if (ctx->resultv) {
    free(ctx->resultv);
  }
}

static int decode_images_add_fmt(struct decode_images_context *ctx,const char *fmt) {
  if (ctx->fmtc>=ctx->fmta) {
    int na=ctx->fmta+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(ctx->fmtv,sizeof(void*)*na);
    if (!nv) return -1;
    ctx->fmtv=nv;
    ctx->fmta=na;
  }
  ctx->fmtv[ctx->fmtc++]=fmt;
  return 0;
}

static int decode_images_cb_path(const char *path,const char *base,char type,void *userdata) {
  struct decode_images_context *ctx=userdata;

  //XXX artificially limit while working
  //if (ctx->pathc>=4) return 1;
  //if (strcmp(base,"oi9n2c16.png")) return 0;

  if (ctx->pathc>=ctx->patha) {
    int na=ctx->patha+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(ctx->pathv,sizeof(void*)*na);
    if (!nv) return -1;
    ctx->pathv=nv;
    ctx->patha=na;
  }
  if (!(ctx->pathv[ctx->pathc]=strdup(path))) return -1;
  ctx->pathc++;
  return 0;
}

static int images_match(const struct rawimg *a,const struct rawimg *b) {
  if (a==b) return 1;
  if (!a||!b) return 0;
  if (a->w!=b->w) return 0;
  if (a->h!=b->h) return 0;
  if (a->pixelsize!=b->pixelsize) return 0;
  int cmpc=(a->stride<b->stride)?a->stride:b->stride;
  const uint8_t *arow=a->v,*brow=b->v;
  int yi=a->h;
  for (;yi-->0;arow+=a->stride,brow+=b->stride) {
    if (memcmp(arow,brow,cmpc)) return 0;
  }
  return 1;
}

static inline uint8_t extract_with_mask(uint32_t src,uint32_t mask) {
  if (!mask) return 0;
  while (!(mask&1)) { src>>=1; mask>>=1; }
  while (mask>0xff) { src>>=1; mask>>=1; }
  // if mask<0xff, shift left and copy. i don't think that comes up in our test
  return src;
}

static int cvt_to_rgba(int src,void *userdata) {
  struct rawimg *rawimg=userdata;
  if ((src>=0)&&(src<rawimg->ctabc)) {
    const uint8_t *p=rawimg->ctab+src*4;
    return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);
  }
  if (rawimg->rmask) {
    // This is wildly inefficient...
    uint8_t r=extract_with_mask(src,rawimg->rmask);
    uint8_t g=extract_with_mask(src,rawimg->gmask);
    uint8_t b=extract_with_mask(src,rawimg->bmask);
    uint8_t a=extract_with_mask(src,rawimg->amask);
    return r|(g<<8)|(b<<16)|(a<<24);
  }
  if (!memcmp(rawimg->chorder,"RGB\0",4)) {
    return src|0xff000000;
  }
  return src;
}

static int cvt_to_y1(int src,void *userdata) {
  struct rawimg *rawimg=userdata;
  return src;//TODO
}

static void dump_mismatched_images(const char *path,const char *fmt,const struct rawimg *a,const struct rawimg *b) {
  if (!a||!b) return;
  if ((a->w!=b->w)||(a->h!=b->h)||(a->pixelsize!=b->pixelsize)) {
    fprintf(stderr,
      "%s as %s: Geometry mismatch (%d,%d,%d)=>(%d,%d,%d)\n",
      path,fmt,a->w,a->h,a->pixelsize,b->w,b->h,b->pixelsize
    );
    return;
  }
  fprintf(stderr,"=== %s as %s, mismatch ===\n",path,fmt);
  int wlimit=30; // max byte per row. for each image. so output width is like (wlimit*6)
  int cpc=(a->stride<b->stride)?a->stride:b->stride;
  if (cpc>wlimit) cpc=wlimit;
  const uint8_t *arow=a->v,*brow=b->v;
  int y=0;
  for (;y<a->h;y++,arow+=a->stride,brow+=b->stride) {
    char tmp[1024];
    int tmpc,i;
    tmpc=snprintf(tmp,sizeof(tmp),"  [%3d]",y);
    for (i=0;i<cpc;i++) {
      if (arow[i]==brow[i]) {
        tmpc+=snprintf(tmp+tmpc,sizeof(tmp)-tmpc," %02x",arow[i]);
      } else {
        tmpc+=snprintf(tmp+tmpc,sizeof(tmp)-tmpc," \x1b[31m%02x\x1b[0m",arow[i]);
      }
    }
    tmpc+=snprintf(tmp+tmpc,sizeof(tmp)-tmpc," |");
    for (i=0;i<cpc;i++) {
      if (arow[i]==brow[i]) {
        tmpc+=snprintf(tmp+tmpc,sizeof(tmp)-tmpc," %02x",brow[i]);
      } else {
        tmpc+=snprintf(tmp+tmpc,sizeof(tmp)-tmpc," \x1b[31m%02x\x1b[0m",brow[i]);
      }
    }
    fprintf(stderr,"%.*s\n",tmpc,tmp);
  }
}

// The main test body. Run for one file and format, dump results in the provided vector.
static int decode_images_1(
  struct decode_images_result *result,
  struct decode_images_context *ctx,
  struct rawimg *rawimg,
  const void *serial0,int serial0c,
  const char *path,
  const char *fmt
) {

  /* You'll see that rlead and gif fail at the compare, for most images.
   * That's because they're comparing RGBA to their own y1 or i8, it's just not possible to match.
   * We could lend them a hand and convert to something amenable first, but that gets messy.
   */
  
  rawimg->encfmt=fmt;
  struct sr_encoder serial={0};
  if (rawimg_encode(&serial,rawimg)<0) {
    fprintf(stderr,"%s: Failed to encode as %s. size=%d,%d pixelsize=%d\n",path,fmt,rawimg->w,rawimg->h,rawimg->pixelsize);
    sr_encoder_cleanup(&serial);
    // Report will be "xxx".
    return 0;
  }
  // Report will be "!NNN" (mismatch) or "NNN" (match).
  result->serialc=serial.c;
  
  // hexdump all encoded images
  if (0&&!strcmp(fmt,"rawimg")) {
    fprintf(stderr,"=== %s as %s (%d,%d) ===\n",path,fmt,rawimg->w,rawimg->h);
    int p=0;
    for (;p<serial.c;p+=16) {
      char tmp[128];
      int tmpc=snprintf(tmp,sizeof(tmp),"%08x |",p);
      int i=0; for (;i<16;i++) {
        if (p+i>=serial.c) break;
        tmp[tmpc++]=' ';
        tmp[tmpc++]="0123456789abcdef"[((uint8_t*)serial.v)[p+i]>>4];
        tmp[tmpc++]="0123456789abcdef"[((uint8_t*)serial.v)[p+i]&15];
      }
      fprintf(stderr,"%.*s\n",tmpc,tmp);
    }
    fprintf(stderr,"%08x\n",serial.c);
  }
  
  // selectively dump encoded image for manual review
  if (0&&!strcmp(path,"src/test/int/img/testfiles/oi9n2c16.png")&&!strcmp(fmt,"bmp")) {
    const char *dstpath="mid/oi9n2c16.bmp";
    if (file_write(dstpath,serial.v,serial.c)>=0) {
      fprintf(stderr,"%s: Wrote %d bytes reencoded from %s\n",dstpath,serial.c,path);
    }
  }
  
  struct rawimg *redecode=rawimg_decode(serial.v,serial.c);
  result->match=images_match(rawimg,redecode);
  
  if (0&&!result->match) dump_mismatched_images(path,fmt,rawimg,redecode);

  rawimg_del(redecode);
  sr_encoder_cleanup(&serial);
  return 0;
}

static void decode_images_report_results(struct decode_images_context *ctx) {
  int colw=8; // Long enough for the longest encoding name and cell content, and a space.
  int hdrcolw=10; // Long enough for the longest test file basename minus suffix; we will truncate if longer.
  int rowbufc=hdrcolw+colw*ctx->fmtc;
  char *rowbuf=malloc(rowbufc);
  if (!rowbuf) return;
  
  { // Header row.
    memset(rowbuf,' ',rowbufc);
    char *dst=rowbuf+hdrcolw;
    int fmtp=0; for (;fmtp<ctx->fmtc;fmtp++,dst+=colw) {
      const char *name=ctx->fmtv[fmtp];
      int namec=0; while (name[namec]) namec++;
      if (namec>colw) namec=colw;
      memcpy(dst+colw-namec,name,namec);
    }
    fprintf(stderr,"%.*s\n",rowbufc,rowbuf);
  }
  
  const struct decode_images_result *result=ctx->resultv;
  int pathp=0; for (;pathp<ctx->pathc;pathp++) {
    memset(rowbuf,' ',rowbufc);
    
    const char *path=ctx->pathv[pathp];
    int pathc=0; while (path[pathc]) pathc++;
    int sepp=path_split(path,pathc);
    const char *base=path+sepp+1;
    int basec=pathc-sepp-1;
    int i=0; for (;i<basec;i++) if (base[i]=='.') basec=i;
    if (basec>hdrcolw) basec=hdrcolw;
    memcpy(rowbuf,base,basec);
    
    char *dst=rowbuf+hdrcolw;
    int fmtp=0; for (;fmtp<ctx->fmtc;fmtp++,result++,dst+=colw) {
      char tmp[32];
      int tmpc=0;
      
      if (result->serialc<1) { // Failed to encode.
        memcpy(tmp,"xxxxxxx",7);
        tmpc=7;
      } else if (!result->match) { // Encoded but failed to decode or mismatch after decode.
        tmpc=snprintf(tmp,sizeof(tmp),"!%d",result->serialc);
      } else { // Everything worked. Report the encoded length.
        tmpc=sr_decsint_repr(tmp,sizeof(tmp),result->serialc);
      }
      
      if (tmpc>colw) tmpc=colw;
      memcpy(dst+colw-tmpc,tmp,tmpc);
    }
    fprintf(stderr,"%.*s\n",rowbufc,rowbuf);
  }
  
  free(rowbuf);
}
 
ITEST(decode_images) {
  struct decode_images_context ctx={0};
  
  for (;;) {
    const char *fmt=rawimg_supported_format_by_index(ctx.fmtc);
    if (!fmt) break;
    if (decode_images_add_fmt(&ctx,fmt)<0) FAIL()
  }
  ASSERT_INTS_OP(ctx.fmtc,>,0)
  
  ASSERT_CALL(dir_read("src/test/int/img/testfiles",decode_images_cb_path,&ctx))
  ASSERT_INTS_OP(ctx.pathc,>,0)
  
  int resultc=ctx.fmtc*ctx.pathc;
  if (!(ctx.resultv=calloc(resultc,sizeof(struct decode_images_result)))) FAIL()
  
  struct decode_images_result *result=ctx.resultv;
  int pathp=0; for (;pathp<ctx.pathc;pathp++) {
    const char *path=ctx.pathv[pathp];
    //fprintf(stderr,"%s:%d: Processing %s...\n",__FILE__,__LINE__,path);
    void *serial0=0;
    int serial0c=file_read(&serial0,path);
    ASSERT_INTS_OP(serial0c,>,0,"path=%s",path)
    struct rawimg *rawimg=rawimg_decode(serial0,serial0c);
    if (rawimg) {
      //rawimg_canonicalize(rawimg);
      rawimg_force_rgba(rawimg);
      int fmtp=0; for (;fmtp<ctx.fmtc;fmtp++,result++) {
        const char *fmt=ctx.fmtv[fmtp];
        ASSERT_CALL(decode_images_1(result,&ctx,rawimg,serial0,serial0c,path,fmt),"path=%s fmt=%s",path,fmt)
      }
    } else {
      // Failed to decode initially, skip everything. They'll have (serialc,match)==0 by default.
      result+=ctx.fmtc;
    }
    rawimg_del(rawimg);
    free(serial0);
  }
  
  // Enable this line for a huge report.
  decode_images_report_results(&ctx);

  decode_images_context_cleanup(&ctx);
  return 0;
}
