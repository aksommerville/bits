/* gif.h
 * Minimal encoder and decoder for GIF images.
 */
 
#ifndef GIF_H
#define GIF_H

struct sr_encoder;

struct gif_image {
  struct gif_plane {
    void *v;
    int x,y,w,h;
    int stride;
    int pixelsize;
    int delay_ms;
    void *ctab;
    int ctabc;
  } *planev;
  int planec,planea;
  void *ctab;
  int ctabc;
};

void gif_plane_cleanup(struct gif_plane *plane);
void gif_image_cleanup(struct gif_image *image);
void gif_image_del(struct gif_image *image);

int gif_encode(struct sr_encoder *dst,const struct gif_image *image);

struct gif_image *gif_decode(const void *src,int srcc);

/* Force there to be exactly one plane, in place.
 * If this is an animation, we painstakingly run through the whole thing, ending with the last frame.
 * If any local color tables are present, the flattened plane will be 32-bit RGBA.
 */
int gif_image_flatten(struct gif_image *image);

#endif
