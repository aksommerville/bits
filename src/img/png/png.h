/* png.h
 * Simple PNG decoder and encoder.
 */
 
#ifndef PNG_H
#define PNG_H

struct sr_encoder;

struct png_image {
  void *v;
  int w,h,stride;
  int depth,colortype;
  int pixelsize; // bits, total
  struct png_chunk { // IHDR,IDAT,IEND are never included
    char chunktype[4];
    int c;
    void *v;
  } *chunkv;
  int chunkc,chunka;
};

void png_image_cleanup(struct png_image *image);
void png_image_del(struct png_image *image);

/* New image with newly-allocated and zeroed pixels.
 */
struct png_image *png_image_new(int w,int h,int depth,int colortype);

struct png_chunk *png_image_add_chunk(struct png_image *image,const char chunktype[4],const void *v,int c);
void png_image_remove_chunk(struct png_image *image,int p);

/* Rewrite to the given depth and colortype, in place.
 * (image->v) may be freed and re-allocated during the conversion.
 * Stride will always be its minimum after reformat. We'll do only that, if (depth,colortype) already match.
 */
int png_image_reformat(struct png_image *image,int depth,int colortype);

/* Produce a complete PNG file in (dst).
 * Stride does not need to be minimized first.
 * Chunks are encoded just as is, and all before the IDAT chunk.
 */
int png_encode(struct sr_encoder *dst,const struct png_image *image);

/* Decode PNG file in one shot.
 * All chunks except IHDR,IDAT,IEND are preserved blindly.
 */
struct png_image *png_decode(const void *src,int srcc);

/* Aside from "reformat" and raw access, we don't provide general image operations.
 * This unit should be used in conjunction with some other generic software-imaging unit.
 */

#endif
