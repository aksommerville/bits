/* rawimg.h
 * Generic in-memory image format, and plain uncompressed serial format.
 *
 * Serial format:
 *   0000   4 Signature: "\x00raw"
 *   0004   2 Width
 *   0006   2 Height
 *   0008   1 Pixel size, bits (factor or multiple of 8)
 *   0009   1 Format description, RAWIMG_DESC_*
 *   000a
 */
 
#ifndef RAWIMG_H
#define RAWIMG_H

#include <stdint.h>

struct sr_encoder;

/* Values for "desc".
 * These are a hint as to how pixel values should be interpretted.
 * They are not necessary for encoding or decoding.
 */
#define RAWIMG_DESC_UNSPEC       0 /* Unknown. */
#define RAWIMG_DESC_INDEX        1 /* Index in some color table that we don't provide. Not divisible or scalable. */
#define RAWIMG_DESC_SCALAR       2 /* Single plain scalar value, exact meaning unspecified. */
#define RAWIMG_DESC_Y            3 /* One luminance channel. */
#define RAWIMG_DESC_A            4 /* One alpha channel. */
#define RAWIMG_DESC_YA           5 /* Luminance and alpha, luma in the more significant portion. */
#define RAWIMG_DESC_AY           6 /* Alpha and luminance. */
#define RAWIMG_DESC_RGB          7
#define RAWIMG_DESC_BGR          8
#define RAWIMG_DESC_RGBA        10
#define RAWIMG_DESC_BGRA        11
#define RAWIMG_DESC_ARGB        12
#define RAWIMG_DESC_ABGR        13
#define RAWIMG_DESC_RGB565BE    14
#define RAWIMG_DESC_BGR565BE    15
#define RAWIMG_DESC_RGB565LE    16
#define RAWIMG_DESC_BGR565LE    17
#define RAWIMG_DESC_RGBA5551BE  18
#define RAWIMG_DESC_BGRA5551BE  19
#define RAWIMG_DESC_ARGB1555BE  20
#define RAWIMG_DESC_ABGR1555BE  21
#define RAWIMG_DESC_RGBA5551LE  22
#define RAWIMG_DESC_BGRA5551LE  23
#define RAWIMG_DESC_ARGB1555LE  24
#define RAWIMG_DESC_ABGR1555LE  25

#define RAWIMG_XFORM_XREV 1
#define RAWIMG_XFORM_YREV 2
#define RAWIMG_XFORM_SWAP 4

struct rawimg {
  void *v;
  int ownv;
  int w,h;
  int stride;
  int pixelsize;
  int desc;
};

/* Image object.
 *******************************************************************/

void rawimg_cleanup(struct rawimg *rawimg);
void rawimg_del(struct rawimg *rawimg);

struct rawimg *rawimg_new_alloc(int w,int h,int pixelsize,int desc);
struct rawimg *rawimg_new_handoff(void *v,int w,int h,int stride,int pixelsize,int desc);
struct rawimg *rawimg_new_borrow(void *v,int w,int h,int stride,int pixelsize,int desc);

struct rawimg *rawimg_new_convert(
  const struct rawimg *src,
  int pixelsize,int desc,
  int x,int y,int w,int h,
  int (*cvt)(int srcpx,void *userdata),
  void *userdata
);

/* Encoding.
 *****************************************************************/

/* We check other available image-format units, and can produce or consume any of them.
 * (encoding) can be null or empty string for "rawimg" format.
 * Otherwise: "png" "jpeg" "gif" "bmp" "ico" "qoi" "rlead" "rawimg"
 * GIF gets flattened on decode, and ICO we select the largest image.
 */
int rawimg_encode(struct sr_encoder *encoder,const struct rawimg *rawimg,const char *encoding);
struct rawimg *rawimg_decode(const void *src,int srcc);

/* Iteration.
 ******************************************************************/

struct rawimg_iterator_1 {
  void *p;
  uint8_t mask;
  int d;
  int c;
};

struct rawimg_iterator {
  struct rawimg_iterator_1 minor;
  struct rawimg_iterator_1 major;
  int minorc;
  int (*read)(const void *p,uint8_t mask);
  void (*write)(void *p,uint8_t mask,int pixel);
};

/* (x,y,w,h) must be within (image) bounds.
 * (xform) tells us which corner to start at and which direction to travel.
 * RAWIMG_XFORM_SWAP makes us travel vertically first then horizontally -- it does NOT change (w,h).
 * On success, (iter) is pointing at the first pixel and ready to use.
 */
int rawimg_iterate(
  struct rawimg_iterator *iter,
  const struct rawimg *image,
  int x,int y,int w,int h,
  uint8_t xform
);

/* Advance to the next pixel.
 * Returns zero if complete, or nonzero if we're pointing to the next pixel.
 */
static inline int rawimg_iterator_next(struct rawimg_iterator *iter) {
  return 0;//TODO
}

static inline int rawimg_iterator_read(struct rawimg_iterator *iter) {
  return iter->read(iter->minor.p,iter->minor.mask);
}

static inline void rawimg_iterator_write(struct rawimg_iterator *iter,int pixel) {
  iter->write(iter->minor.p,iter->minor.mask,pixel);
}

#endif
