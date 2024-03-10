#include "test/test.h"
#include "opt/rawimg/rawimg.h"
#include "opt/jpeg/jpeg.h"
#include "opt/png/png.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Decode a JPEG file, then reencode it.
 * I've added JPEG somewhat after most formats, so didn't bother to incorporate in the larger "decode_images.c" test.
 */
 
ITEST(jpeg_reencode) {

  // This is a picture of a duck I found on unsplash.com, which says it's licensed freely.
  // Modified: I scaled it to (731,1024) from the original (3417,4784).
  const char *path="src/test/int/img/testfiles/ross-sokolovski-kCZSzqvIei4-unsplash.jpg";
  
  void *original=0;
  int originalc=file_read(&original,path);
  ASSERT_CALL(originalc,"%s: Failed to read file",path)
  
  struct jpeg_image *image1=jpeg_decode(original,originalc);
  ASSERT(image1,"%s: First decode failed (%d bytes)",path,originalc)
  //fprintf(stderr,"%s: Decoded OK. w=%d h=%d chanc=%d stride=%d\n",path,image1->w,image1->h,image1->chanc,image1->stride);
  
  struct sr_encoder reencoded={0};
  ASSERT_CALL(jpeg_encode(&reencoded,image1))
  
  struct jpeg_image *image2=jpeg_decode(reencoded.v,reencoded.c);
  ASSERT(image2,"%s: Second decode failed (%d bytes)",path,reencoded.c)
  
  // Optionally save this as a PNG file, to validate decode manually.
  // ...validated, looks fine (RGB).
  if (0) {
    struct png_image png={
      .v=image2->v,
      .w=image2->w,
      .h=image2->h,
      .stride=image2->stride,
      .depth=8,
    };
    switch (image2->chanc) {
      case 1: png.colortype=0; png.pixelsize=8; break;
      case 3: png.colortype=2; png.pixelsize=24; break;
      case 4: png.colortype=6; png.pixelsize=32; break;
      default: png.depth=0;
    }
    if (png.depth) {
      struct sr_encoder dst={0};
      if (png_encode(&dst,&png)>=0) {
        if (file_write("mid/verify2.png",dst.v,dst.c)>=0) {
          fprintf(stderr,"mid/verify2.png: Wrote image as PNG for manual validation.\n");
        } else {
          fprintf(stderr,"mid/verify2.png: Failed to write PNG, %d bytes.\n",dst.c);
        }
      } else {
        fprintf(stderr,"Failed to reencode as PNG!\n");
      }
      sr_encoder_cleanup(&dst);
    }
    png.v=0;
    png_image_cleanup(&png);
  }
  
  /* We can't just compare pixels directly; JPEG is a lossy format.
   * Ensure that sizes match exactly, then measure the average deviation in color values.
   * The tolerance below at ASSERT_FLOATS is completely arbitrary.
   * We happen to observe 1.728882 with the duck test image, and to my eye, I can't distinguish the pics.
   * That's in sample points, so 1/256ths of the full range.
   */
  ASSERT_INTS(image1->w,image2->w)
  ASSERT_INTS(image1->h,image2->h)
  const uint8_t *row1=image1->v;
  const uint8_t *row2=image2->v;
  int totaldiff=0;
  int yi=image1->h;
  for (;yi-->0;row1+=image1->stride,row2+=image2->stride) {
    const uint8_t *p1=row1;
    const uint8_t *p2=row2;
    int xi=image1->w;
    for (;xi-->0;p1++,p2++) {
      int diff=(*p1)-(*p2);
      if (diff<0) diff=-diff;
      totaldiff+=diff;
    }
  }
  double averagediff=(double)totaldiff/(double)(image1->w*image1->h);
  ASSERT_FLOATS(averagediff,0.0,4.0)
  
  jpeg_image_del(image1);
  jpeg_image_del(image2);
  free(original);
  sr_encoder_cleanup(&reencoded);
  return 0;
}

/* Verify that decoding JPEG fails sanely, it's a bit of a drama in libjpeg.
 */
 
ITEST(jpeg_decode_error) {
  struct jpeg_image *image=jpeg_decode(0,0);
  ASSERT_NOT(image)
  // The real assertion is that we complete at all.
  return 0;
}
