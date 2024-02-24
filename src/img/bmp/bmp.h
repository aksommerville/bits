/* bmp.h
 * Minimal encoder and decoder for Microsoft BMP images.
 */
 
#ifndef BMP_H
#define BMP_H

struct sr_encoder;

struct bmp_image {
  void *v;
  int w,h; // (h) can be negative for LRBT, usually so.
  int stride;
  int pixelsize; // bits
  void *ctab; // 32-bit rgba
  int ctabc; // in 4-byte entries
};

void bmp_image_cleanup(struct bmp_image *image);
void bmp_image_del(struct bmp_image *image);

int bmp_encode(struct sr_encoder *dst,const struct bmp_image *image);

struct bmp_image *bmp_decode(const void *src,int srcc);

/* If (image) is stored bottom-up, swap it in place.
 */
int bmp_image_force_lrtb(struct bmp_image *image);

#endif
