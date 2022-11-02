/* png.h
 */

#ifndef PNG_H
#define PNG_H

#include <stdint.h>

/* Image object.
 ****************************************************************************/

struct png_image {
  int refc;
  void *pixels;
  int w,h;
  int stride; // bytes

  uint8_t depth;
  uint8_t colortype;

  struct png_chunk {
    uint32_t chunkid;
    void *v;
    int c;
  } *chunkv;
  int chunkc,chunka;
};

void png_image_del(struct png_image *image);
int png_image_ref(struct png_image *image);

/* (0,0,0,0) is legal, we do not allocate pixels in that case.
 */
struct png_image *png_image_new(int w,int h,uint8_t depth,uint8_t colortype);

/* Crop, pad, or reformat an image.
 * Zeroes to use a default, eg png_image_reformat(image,0,0,0,0,0,0,0) is guaranteed to only retain image.
 * If (always_copy), we produce a fresh image even if retaining was possible.
 * In general, extra chunks are dropped here. We do copy or generate PLTE and tRNS, if needed.
 */
struct png_image *png_image_reformat(
  struct png_image *image,
  int x,int y,int w,int h,
  uint8_t depth,uint8_t colortype,
  int always_copy
);

/* Calls (cb) with each pixel in LRTB order until you return nonzero or we reach the end.
 * We convert to 32-bit RGBA for all valid input formats, and we do use PLTE and tRNS as warranted.
 * (image) is const to us.
 */
int png_image_iterate(
  struct png_image *image,
  int (*cb)(struct png_image *image,int x,int y,uint32_t rgba,void *userdata),
  void *userdata
);

/* Painstakingly read or write individual pixels as 32-bit RGBA.
 */
uint32_t png_image_read(const struct png_image *image,int x,int y);
void png_image_write(struct png_image *img,int x,int y,uint32_t rgba);

/* Conveniences to rewrite a (depth,colortype) pair in place, to meet a given criterion.
 * "8bit" means channels must be 8-bit. Pixels can be 8, 16, 24, or 32.
 * Typically you'll do a few of these just before "reformat". Don't do them in-place on the struct png_image!
 */
void png_depth_colortype_legal(uint8_t *depth,uint8_t *colortype);
void png_depth_colortype_8bit(uint8_t *depth,uint8_t *colortype);
void png_depth_colortype_luma(uint8_t *depth,uint8_t *colortype);
void png_depth_colortype_rgb(uint8_t *depth,uint8_t *colortype);
void png_depth_colortype_alpha(uint8_t *depth,uint8_t *colortype);
void png_depth_colortype_opaque(uint8_t *depth,uint8_t *colortype);

/* Decode.
 ****************************************************************************/

/* Simple one-shot decode to a new image, if you have the entire file in memory.
 */
struct png_image *png_decode(const void *src,int srcc);

struct png_decoder;

void png_decoder_del(struct png_decoder *decoder);
struct png_decoder *png_decoder_new();

/* Call png_decode_more() with the input file, any sizes at all are valid, we use a private cache as needed.
 * Once all input is delivered, call png_decode_finish() to get the image.
 * Finishing *does not* delete the decoder, you have to do that too.
 */
int png_decode_more(struct png_decoder *decoder,const void *src,int srcc);
struct png_imaage *png_decode_finish(struct png_decoder *decoder);

/* Encode.
 * We only offer one-shot encoding, you need enough memory to hold the entire encoded image.
 *****************************************************************************/

int png_encode(void *dstpp,const struct png_image *image);

#endif
