#include "test/test.h"
#include "img/rawimg/rawimg.h"
#include "fs/fs.h"
#include "serial/serial.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

/* Configuration.
 */

// basename of a file in our test set, or empty string to run all.
#define RUN_SINGLE_FILE ""

// Process only the first N files found, if >0.
#define FILE_LIMIT 0

// Replace with rawimg_canonicalize, rawimg_force_rgba, etc, if desired.
#define precanon(rawimg) 0

// Nonzero to write every converted file under mid/test/int/img/testfiles.
// Beware this can be a huge number of files.
#define SAVE_ALL_OUTPUTS 0

// Nonzero to print a side-by-side, some small portion of each image that failed matching after re-decode.
#define PRINT_MISMATCH_COMPARISONS 0

// Dump all outcomes, and encoded sizes where successful.
#define PRINT_BIG_REPORT 0

// Instead of encoded sizes, print ratio to original input file size.
#define PRINT_RELATIVE_SIZES 0

/* Global context.
 */
 
struct di {
  const char **fmtv;
  int fmtc,fmta;
  char **pathv;
  int pathc,patha;
  
  /* Sorted by (path,fmt), which must match our (pathv,fmtv),
   * except results may omit an entire (pathv) row.
   * ie it's naturally in the correct order for tabular output.
   */
  struct di_result {
    const char *path;
    const char *fmt;
    int original_serialc; // From the test file (same for all of a given path).
    int serialc; // Encoded length, or <0 if encode failed.
    int match; // (0,1,2,3)=( Didn't try decode, decode failed, mismatch, success )
  } *resultv;
  int resultc,resulta;
};

static void di_cleanup(struct di *di) {
  if (di->fmtv) free(di->fmtv);
  if (di->pathv) {
    while (di->pathc-->0) free(di->pathv[di->pathc]);
    free(di->pathv);
  }
  if (di->resultv) free(di->resultv);
  memset(di,0,sizeof(struct di));
}

/* Some interesting events where we might want to dump output.
 * These are not for normal reporting, and should be empty unless you're tracking down some specific bug.
 */

static void di_evt_decode_failed(
  struct di *di,
  const char *path,
  const void *serial,int serialc
) {
}

static void di_evt_decoded(
  struct di *di,
  const char *path,
  const void *serial,int serialc,
  const struct rawimg *rawimg
) {
}

static void di_evt_encode_failed(
  struct di *di,
  const char *path,
  const char *fmt,
  const struct rawimg *rawimg
) {
}
 
static void di_evt_encoded(
  struct di *di,
  const char *path,
  const char *fmt,
  const void *serial,int serialc
) {
  if (SAVE_ALL_OUTPUTS) {
    if (memcmp(path,"src/",4)) return;
    int fmtc=0;
    while (fmt[fmtc]) fmtc++;
    int pathc=0;
    while (path[pathc]) pathc++;
    char dstpath[1024];
    if (pathc+1+fmtc>=sizeof(dstpath)) return;
    memcpy(dstpath,path,pathc);
    memcpy(dstpath,"mid/",4);
    dstpath[pathc]='.';
    memcpy(dstpath+pathc+1,fmt,fmtc+1);
    if (dir_mkdirp_parent(dstpath)<0) return;
    if (file_write(dstpath,serial,serialc)<0) return;
    fprintf(stderr,"%s: Wrote file, %d bytes.\n",dstpath,serialc);
  }
}

static void di_evt_redecode_failed(
  struct di *di,
  const char *path,
  const char *fmt,
  const void *serial,int serialc
) {
}

static void di_evt_mismatch(
  struct di *di,
  const char *path,
  const char *fmt,
  const struct rawimg *before,
  const struct rawimg *after
) {
  if (PRINT_MISMATCH_COMPARISONS) {
    const int wlimit=6; // 18 columns per pixel, plus some overhead, there's not much room.
    const int hlimit=40; // No particular reason for a limit on height.
    fprintf(stderr,"=== Mismatch %s:%s ===\n",path,fmt);
    int cmpw=(before->w<after->w)?before->w:after->w;
    int cmph=(before->h<after->h)?before->h:after->h;
    if (cmpw>wlimit) cmpw=wlimit;
    if (cmph>hlimit) cmph=hlimit;
    struct rawimg_iterator aiter,biter;
    if (rawimg_iterate(&aiter,before,0,0,cmpw,cmph,0)<0) return;
    if (rawimg_iterate(&biter,after,0,0,cmpw,cmph,0)<0) return;
    int yi=cmph; while (yi-->0) {
      int xi;
      for (xi=cmpw;xi-->0;) {
        int pixel=rawimg_iterator_read(&aiter);
        //pixel=(pixel|(pixel>>8)|(pixel>>16)|(pixel>>24))&0xff;
        fprintf(stderr," %08x",pixel);
        if (!rawimg_iterator_next(&aiter)) break;
      }
      fprintf(stderr," |");
      for (xi=cmpw;xi-->0;) {
        int pixel=rawimg_iterator_read(&biter);
        //pixel=(pixel|(pixel>>8)|(pixel>>16)|(pixel>>24))&0xff;
        fprintf(stderr," %08x",pixel);
        if (!rawimg_iterator_next(&biter)) break;
      }
      fprintf(stderr,"\n");
    }
    fprintf(stderr,"=== End of mismatch dump ===\n");
  }
}

/* Compare two images, should be identical.
 * Nonzero if identical.
 */
 
static int di_compare_images(
  const struct rawimg *a,
  const struct rawimg *b,
  const char *path,
  const char *fmt
) {
  
  // First a couple easy answers...
  if (a==b) return 1;
  if (!a||!b) return 0;
  if ((a->w!=b->w)||(a->h!=b->h)) return 0;
  
  /* For lossy formats, we have to assume that pixels will change a little.
   * Call it ok as long as the dimensions match, I think that's all we can do.
   */
  if (!strcmp(fmt,"jpeg")) return 1;
  
  /* If (pixelsize,chorder,bitorder,rmask) match, compare by memcmp, easy.
   * But not if color tables are in play, then all bets are off.
   */
  if (
    (a->pixelsize==b->pixelsize)&&
    !memcmp(a->chorder,b->chorder,4)&&
    (a->bitorder==b->bitorder)&&
    (a->rmask==b->rmask)&&
    !a->ctabc&&!b->ctabc
  ) {
    if (a->stride==b->stride) {
      return memcmp(a->v,b->v,a->stride*a->h)?0:1;
    }
    const uint8_t *arow=a->v;
    const uint8_t *brow=b->v;
    int cmpc=(a->stride<b->stride)?a->stride:b->stride;
    int yi=a->h;
    for (;yi-->0;arow+=a->stride,brow+=b->stride) {
      if (memcmp(arow,brow,cmpc)) return 0;
    }
    return 1;
  }
  
  /* OK now the tricky compare.
   * Lots of conversions will involve some destructive down-sampling.
   * So we need rules loose enough to accomodate that:
   *  - Iterate both images and record what pixel was produced for each source pixel.
   *  - If a given source value produces multiple outputs, mismatch.
   *  - If source has at least 2 distinct colors, dest must have at least 2 too.
   * That last point won't always work. A dark input converting to y1 might go all black.
   * We'll trust that test images contain sufficient contrast.
   * I do want to assert that the output isn't blank, that seems important.
   * (a) is INPUT, (b) is OUTPUT.
   */
  struct cvtpx { int a,b; } *cvtpxv=0;
  int cvtpxc=0,cvtpxa=0;
  struct rawimg_iterator aiter,biter;
  if (rawimg_iterate(&aiter,a,0,0,a->w,a->h,0)<0) return 0;
  if (rawimg_iterate(&biter,b,0,0,b->w,b->h,0)<0) return 0;
  do {
    int apx=rawimg_iterator_read(&aiter);
    int bpx=rawimg_iterator_read(&biter);
    
    int lo=0,hi=cvtpxc,p=-1;
    while (lo<hi) {
      int ck=(lo+hi)>>1;
      const struct cvtpx *cvtpx=cvtpxv+ck;
           if (apx<cvtpx->a) hi=ck;
      else if (apx>cvtpx->a) lo=ck+1;
      else { p=ck; break; }
    }
    if (p<0) {
      p=lo;
      if (cvtpxc>=cvtpxa) {
        int na=cvtpxa+256;
        if (na>INT_MAX/sizeof(struct cvtpx)) {
          if (cvtpxv) free(cvtpxv);
          return 0;
        }
        void *nv=realloc(cvtpxv,sizeof(struct cvtpx)*na);
        if (!nv) {
          if (cvtpxv) free(cvtpxv);
          return 0;
        }
        cvtpxv=nv;
        cvtpxa=na;
      }
      struct cvtpx *cvtpx=cvtpxv+p;
      memmove(cvtpx+1,cvtpx,sizeof(struct cvtpx)*(cvtpxc-p));
      cvtpxc++;
      cvtpx->a=apx;
      cvtpx->b=bpx;
    } else {
      if (cvtpxv[p].b!=bpx) {
        fprintf(stderr,"%s:%s: pixel %08x became both %08x and %08x p=%d/%d [%08x]\n",path,fmt,apx,cvtpxv[p].b,bpx,p,cvtpxc,cvtpxv[p].a);
        free(cvtpxv);
        return 0;
      }
    }
    
  } while (rawimg_iterator_next(&aiter)&&rawimg_iterator_next(&biter));
   
  if (cvtpxv) free(cvtpxv);
  return 1;
}

/* Test one decoded image against one format.
 */
 
static int di_test_one_format(
  struct di *di,
  const char *path,
  const char *fmt,
  const void *original_serial,int original_serialc, // for reference, probly not useful
  struct rawimg *rawimg
) {
  if (di->resultc>=di->resulta) {
    int na=di->resulta+256;
    if (na>INT_MAX/sizeof(struct di_result)) return -1;
    void *nv=realloc(di->resultv,sizeof(struct di_result)*na);
    if (!nv) return -1;
    di->resultv=nv;
    di->resulta=na;
  }
  struct di_result *result=di->resultv+di->resultc++;
  memset(result,0,sizeof(struct di_result));
  result->path=path;
  result->fmt=fmt;
  result->original_serialc=original_serialc;
  
  const char *original_fmt=rawimg->encfmt;
  rawimg->encfmt=fmt;
  struct sr_encoder serial={0};
  int err=rawimg_encode(&serial,rawimg);
  rawimg->encfmt=original_fmt;
  if (err<0) {
    di_evt_encode_failed(di,path,fmt,rawimg);
    sr_encoder_cleanup(&serial);
    result->serialc=-1;
    return 0;
  }
  result->serialc=serial.c;
  
  di_evt_encoded(di,path,fmt,serial.v,serial.c);
  
  result->match=1; // decode attempted
  struct rawimg *redecode=rawimg_decode(serial.v,serial.c);
  if (!redecode) {
    di_evt_redecode_failed(di,path,fmt,serial.v,serial.c);
    sr_encoder_cleanup(&serial);
    return 0;
  }
  sr_encoder_cleanup(&serial);
  result->match=2; // decode succeeded
  
  if (di_compare_images(rawimg,redecode,path,fmt)) {
    result->match=3; // perfect!
  } else {
    di_evt_mismatch(di,path,fmt,rawimg,redecode);
  }
  
  rawimg_del(redecode);
  return 0;
}

/* Process files. The main event.
 */
 
static int di_process_file(struct di *di,const char *path) {
  void *serial=0;
  int serialc=file_read(&serial,path);
  ASSERT_CALL(serialc,"path=%s",path)
  
  struct rawimg *rawimg=rawimg_decode(serial,serialc);
  if (rawimg) {
    di_evt_decoded(di,path,serial,serialc,rawimg);
  } else {
    di_evt_decode_failed(di,path,serial,serialc);
    free(serial);
    return -1;
  }
  ASSERT_CALL(precanon(rawimg),"path=%s",path)

  int fmti=0;
  for (;fmti<di->fmtc;fmti++) {
    int err=di_test_one_format(di,path,di->fmtv[fmti],serial,serialc,rawimg);
    if (err<0) {
      free(serial);
      rawimg_del(rawimg);
      return err;
    }
  }
  
  rawimg_del(rawimg);
  free(serial);
  return 0;
}
 
static int di_process_files(struct di *di) {
  int i=0,err;
  for (;i<di->pathc;i++) {
    if ((err=di_process_file(di,di->pathv[i]))<0) {
      fprintf(stderr,"%s: Error processing file.\n",di->pathv[i]);
      return err;
    }
  }
  return 0;
}

/* Report results.
 */
 
static int di_report_results(struct di *di) {
  const int filecolw=20,resultcolw=10;
  int i;
  char buf[1024];
  int bufc=0;
  
  fprintf(stderr,"=== decode_images results ===\n");
  bufc=0;
  memset(buf+bufc,' ',filecolw);
  bufc+=filecolw;
  for (i=0;i<di->fmtc;i++) bufc+=snprintf(buf+bufc,sizeof(buf)-bufc,"%*s",resultcolw,di->fmtv[i]);
  fprintf(stderr,"%.*s\n",bufc,buf);
  bufc=0;
  
  const char *curpath=0;
  const struct di_result *result=di->resultv;
  for (i=di->resultc;i-->0;result++) {
    const char *base=result->path;
    int pathi=0; for (;result->path[pathi];pathi++) if (result->path[pathi]=='/') base=result->path+pathi+1;
    if (curpath!=result->path) {
      if (bufc) fprintf(stderr,"%.*s\n",bufc,buf);
      bufc=snprintf(buf,sizeof(buf),"%*s",filecolw,base);
      curpath=result->path;
    }
  
    if (result->serialc<0) {
      bufc+=snprintf(buf+bufc,sizeof(buf)-bufc,"%*s",resultcolw,"xxxxxxxx");
    } else if (result->match>=3) {
      if (PRINT_RELATIVE_SIZES) {
        bufc+=snprintf(buf+bufc,sizeof(buf)-bufc,"%*.3f",resultcolw,(double)result->serialc/(double)result->original_serialc);
      } else {
        bufc+=snprintf(buf+bufc,sizeof(buf)-bufc,"%*d",resultcolw,result->serialc);
      }
    } else if (result->match==2) {
      bufc+=snprintf(buf+bufc,sizeof(buf)-bufc,"%*s",resultcolw,"mismatch");
    } else {
      bufc+=snprintf(buf+bufc,sizeof(buf)-bufc,"%*s",resultcolw,"error");
    }
    
  }
  if (bufc) fprintf(stderr,"%.*s\n",bufc,buf);
  
  fprintf(stderr,"=== end of decode_images report ===\n");
  return 0;
}

/* Review results and fail if any individual failed.
 * Report has already been printed by this point so it's ok to fail hard.
 */
 
static int di_assert_results(struct di *di) {
  const struct di_result *result=di->resultv;
  int i=di->resultc;
  for (;i-->0;result++) {
    switch (result->match) {
      case 0: FAIL("%s => %s, failed to encode",result->path,result->fmt) break;
      case 1: FAIL("%s => %s, failed to re-decode",result->path,result->fmt) break;
      case 2: FAIL("%s => %s, mismatch after re-decode",result->path,result->fmt) break;
    }
  }
  return 0;
}

/* Discover and add test file paths.
 */
 
static int di_cb_testfiles_bottom(const char *path,const char *base,char type,void *userdata) {
  struct di *di=userdata;
  if (base[0]=='.') return 0;
  if (!type) type=file_get_type(path);
  if (type!='f') return 0;
  
  if (RUN_SINGLE_FILE[0]&&strcmp(base,RUN_SINGLE_FILE)) return 0;
  if ((FILE_LIMIT>0)&&(di->pathc>=FILE_LIMIT)) return 0;
  
  if (di->pathc>=di->patha) {
    int na=di->patha+256;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(di->pathv,sizeof(void*)*na);
    if (!nv) return -1;
    di->pathv=nv;
    di->patha=na;
  }
  if (!(di->pathv[di->pathc]=strdup(path))) return -1;
  di->pathc++;
  return 0;
}
 
static int di_cb_testfiles_top(const char *path,const char *base,char type,void *userdata) {
  struct di *di=userdata;
  if (!type) type=file_get_type(path);
  if (type!='d') return 0;
  
  // Opportunity to filter top-level directories.
  
  return dir_read(path,di_cb_testfiles_bottom,di);
}

/* Gather requirements.
 */
 
static int di_gather_requirements(struct di *di) {
  int p=0,err;
  for (;;p++) {
    const char *fmt=rawimg_supported_format_by_index(p);
    if (!fmt||!fmt[0]) break;
    if (di->fmtc>=di->fmta) {
      int na=di->fmta+16;
      if (na>INT_MAX/sizeof(void*)) return -1;
      void *nv=realloc(di->fmtv,sizeof(void*)*na);
      if (!nv) return -1;
      di->fmtv=nv;
      di->fmta=na;
    }
    di->fmtv[di->fmtc++]=fmt;
  }
  if ((err=dir_read("src/test/int/img/testfiles",di_cb_testfiles_top,di))<0) return err;
  return 0;
}

/* Main entry point.
 */
 
ITEST(decode_images) {
  struct di di={0};
  int result=0;
  if ((result=di_gather_requirements(&di))<0) goto _done_;
  if ((result=di_process_files(&di))<0) goto _done_;
  if (PRINT_BIG_REPORT) {
    if ((result=di_report_results(&di))<0) goto _done_;
  }
  result=di_assert_results(&di);
 _done_:;
  di_cleanup(&di);
  return result;
}
