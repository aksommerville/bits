#include "rawimg.h"

/* Accessors.
 */
 
#define SRC ((const uint8_t*)src)
#define DST ((uint8_t*)dst)
 
static int rawimg_pxr_1(const void *src,uint8_t mask) {
  return ((*SRC)&mask)?1:0;
}

static void rawimg_pxw_1(void *dst,uint8_t mask,int pixel) {
  if (pixel) *DST|=mask;
  else *DST&=~mask;
}
 
static int rawimg_pxr_2(const void *src,uint8_t mask) {
  switch (mask) {
    case 0xc0: return ((*SRC)>>6)&3;
    case 0x30: return ((*SRC)>>4)&3;
    case 0x0c: return ((*SRC)>>2)&3;
    default: return (*SRC)&3;
  }
}

static void rawimg_pxw_2(void *dst,uint8_t mask,int pixel) {
  switch (mask) {
    case 0xc0: *DST=((*DST)&0x3f)|(pixel<<6); return;
    case 0x30: *DST=((*DST)&0xcf)|((pixel&3)<<4); return;
    case 0x0c: *DST=((*DST)&0xf3)|((pixel&3)<<2); return;
    case 0x03: *DST=((*DST)&0xfc)|(pixel&3); return;
  }
}
 
static int rawimg_pxr_4(const void *src,uint8_t mask) {
  switch (mask) {
    case 0xf0: return (*SRC)>>4;
    default: return (*SRC)&15;
  }
}

static void rawimg_pxw_4(void *dst,uint8_t mask,int pixel) {
  switch (mask) {
    case 0xf0: *DST=((*DST)&0x0f)|(pixel<<4);
    case 0x0f: *DST=((*DST)&0xf0)|(pixel&15);
  }
}
 
static int rawimg_pxr_8(const void *src,uint8_t mask) {
  return *(uint8_t*)src;
}

static void rawimg_pxw_8(void *dst,uint8_t mask,int pixel) {
  *(uint8_t*)dst=pixel;
}
 
static int rawimg_pxr_16(const void *src,uint8_t mask) {
  return *(uint16_t*)src;
}

static void rawimg_pxw_16(void *dst,uint8_t mask,int pixel) {
  *(uint16_t*)dst=pixel;
}
 
static int rawimg_pxr_32(const void *src,uint8_t mask) {
  return *(uint32_t*)src;
}

static void rawimg_pxw_32(void *dst,uint8_t mask,int pixel) {
  *(uint32_t*)dst=pixel;
}

// 48 and 64 are odd cases that only come up in PNG, far as I've seen.
// Assume that they are 3 or 4 channels of big-endian 16-bit integers, and repack with the most significant bytes only.
static int rawimg_pxr_48(const void *src,uint8_t mask) {
  return SRC[0]|(SRC[2]<<8)|(SRC[4]<<16);
}
static void rawimg_pxw_48(void *dst,uint8_t mask,int pixel) {
  DST[0]=DST[1]=pixel;
  DST[2]=DST[3]=pixel>>8;
  DST[4]=DST[5]=pixel>>16;
}
static int rawimg_pxr_64(const void *src,uint8_t mask) {
  return SRC[0]|(SRC[2]<<8)|(SRC[4]<<16)|(SRC[6]<<24);
}
static void rawimg_pxw_64(void *dst,uint8_t mask,int pixel) {
  DST[0]=DST[1]=pixel;
  DST[2]=DST[3]=pixel>>8;
  DST[4]=DST[5]=pixel>>16;
  DST[6]=DST[7]=pixel>>24;
}
 
static int rawimg_pxr_bytewise(const void *src,uint8_t mask) {
  int pixel=0;
  while (mask-->0) {
    pixel<<=8;
    pixel|=*SRC;
    src=SRC+1;
  }
  return pixel;
}

static void rawimg_pxw_bytewise(void *dst,uint8_t mask,int pixel) {
  uint8_t *p=DST+mask;
  while (mask-->0) {
    p--;
    *p=pixel;
    pixel>>=8;
  }
}
 
static int rawimg_iterator_get_accessors(struct rawimg_iterator *iter,int pixelsize) {
  switch (pixelsize) {
    case 1: iter->read=rawimg_pxr_1; iter->write=rawimg_pxw_1; return 0;
    case 2: iter->read=rawimg_pxr_2; iter->write=rawimg_pxw_2; return 0;
    case 4: iter->read=rawimg_pxr_4; iter->write=rawimg_pxw_4; return 0;
    case 8: iter->read=rawimg_pxr_8; iter->write=rawimg_pxw_8; return 0;
    case 16: iter->read=rawimg_pxr_16; iter->write=rawimg_pxw_16; return 0;
    case 32: iter->read=rawimg_pxr_32; iter->write=rawimg_pxw_32; return 0;
    case 48: iter->read=rawimg_pxr_48; iter->write=rawimg_pxw_48; return 0;
    case 64: iter->read=rawimg_pxr_64; iter->write=rawimg_pxw_64; return 0;
    default: {
        if (pixelsize&3) return -1;
        if (pixelsize<1) return -1;
        iter->read=rawimg_pxr_bytewise;
        iter->write=rawimg_pxw_bytewise;
      } return 0;
  }
  return -1;
}

/* Begin iteration.
 */
 
int rawimg_iterate(
  struct rawimg_iterator *iter,
  const struct rawimg *image,
  int x,int y,int w,int h,
  uint8_t xform
) {
  if (!iter||!image) return -1;
  if ((x<0)||(y<0)||(w<1)||(h<1)||(x>image->w-w)||(y>image->h-h)) return -1;
  
  if (rawimg_iterator_get_accessors(iter,image->pixelsize)<0) return -1;
  iter->pixelsize=image->pixelsize;
  
  if (image->pixelsize<8) {
    int dxx=1,dyx=0,dxy=0,dyy=1;
    int startx=x,starty=y;
    if (xform&RAWIMG_XFORM_XREV) {
      startx+=w-1;
      dxx=-1;
    }
    if (xform&RAWIMG_XFORM_YREV) {
      starty+=h-1;
      dyy=-1;
    }
    if (xform&RAWIMG_XFORM_SWAP) {
      dyx=dyy;
      dxy=dxx;
      dyy=0;
      dxx=0;
      iter->major.c=w;
      iter->minor.c=h;
    } else {
      iter->major.c=h;
      iter->minor.c=w;
    }
    iter->major.p=iter->minor.p=((uint8_t*)image->v)+starty*image->stride+((startx*image->pixelsize)>>3);
    iter->minor.d=((dyx*image->stride)<<3)+dxx*image->pixelsize;
    iter->major.d=((dyy*image->stride)<<3)+dxy*image->pixelsize;
    if (image->bitorder=='<') {
      switch (image->pixelsize) {
        case 1: iter->major.mask=iter->minor.mask=1<<(startx&7); break;
        case 2: iter->major.mask=iter->minor.mask=0x03<<((startx&3)<<1); break;
        case 4: iter->major.mask=iter->minor.mask=(startx&1)?0xf0:0x0f; break;
        default: return -1;
      }
    } else {
      switch (image->pixelsize) {
        case 1: iter->major.mask=iter->minor.mask=0x80>>(startx&7); break;
        case 2: iter->major.mask=iter->minor.mask=0xc0>>((startx&3)<<1); break;
        case 4: iter->major.mask=iter->minor.mask=(startx&1)?0x0f:0xf0; break;
        default: return -1;
      }
    }
    
  } else {
    int xstride=image->pixelsize>>3;
    int dxx=1,dyx=0,dxy=0,dyy=1;
    uint8_t *p=((uint8_t*)image->v)+y*image->stride+x*xstride;
    if (xform&RAWIMG_XFORM_XREV) {
      p+=(w-1)*xstride;
      dxx=-1;
    }
    if (xform&RAWIMG_XFORM_YREV) {
      p+=(h-1)*image->stride;
      dyy=-1;
    }
    if (xform&RAWIMG_XFORM_SWAP) {
      dyx=dyy;
      dxy=dxx;
      dyy=0;
      dxx=0;
      iter->major.c=w;
      iter->minor.c=h;
    } else {
      iter->major.c=h;
      iter->minor.c=w;
    }
    iter->major.p=iter->minor.p=p;
    iter->major.mask=iter->minor.mask=0;
    iter->minor.d=dyx*image->stride+dxx*xstride;
    iter->major.d=dyy*image->stride+dxy*xstride;
  }
  
  iter->minor.c--;
  iter->major.c--;
  iter->minorc=iter->minor.c;
  iter->bitorder=image->bitorder;
  return 0;
}
