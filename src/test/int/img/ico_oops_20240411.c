#include "test/test.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"
#include "opt/rawimg/rawimg.h"
#include "opt/ico/ico.h"
#include "opt/gif/gif.h"

/* 2024-04-11
 * Found that ICO files are decoding in grayscale, at 2x the expected height.
 */
 
ITEST(ico_oops_20240411) {
  const char *path="src/test/int/img/testfiles/oneoff/dot.ico";
  void *serial=0;
  int serialc=file_read(&serial,path);
  ASSERT_INTS_OP(serialc,>,0,"%s: Failed to read file",path)
  
  // Decode as ICO directly, and extract the smallest image (there should be just one).
  struct ico_file *file=ico_decode(serial,serialc);
  ASSERT(file,"%s: Failed to decode as ICO",path)
  struct ico_image *image=ico_file_get_image(file,0,0,1);
  ASSERT(image,"%s: Failed to acquire smallest image",path)
  
  // It's supposed to be 16x16.
  ASSERT_INTS(image->w,16)
  ASSERT_INTS(image->h,16)
  
  // Decode via rawimg. This should do the same as the above, just repackaging it as rawimg.
  struct rawimg *rawimg=rawimg_decode(serial,serialc);
  ASSERT(rawimg,"%s: Failed to decode as rawimg",path);
  ASSERT_INTS(rawimg->w,16)
  ASSERT_INTS(rawimg->h,16)
  ASSERT_INTS(rawimg->stride,image->stride)
  ASSERT(!memcmp(rawimg->v,image->v,image->stride*image->h))
  
  // There's an adjacent GIF file for reference, containing exactly the same image.
  // Ensure they are identical after decoding. Both ICO and GIF promote images to 32-bit RGBA during decode.
  // Not that you should bet your life on our GIF decoder either, but if they fail they should fail in different ways at least.
  const char *gifpath="src/test/int/img/testfiles/oneoff/dot.gif";
  void *gifserial=0;
  int gifserialc=file_read(&gifserial,gifpath);
  ASSERT_INTS_OP(gifserialc,>,0,"%s: Failed to read file",gifpath)
  struct gif_image *gif=gif_decode(gifserial,gifserialc);
  ASSERT(gif,"%s: Failed to decode GIF",gifpath)
  ASSERT_INTS(gif->w,image->w)
  ASSERT_INTS(gif->h,image->h)
  ASSERT_INTS(gif->stride,image->stride)
  ASSERT(!memcmp(gif->v,image->v,image->stride*image->h))
  
  //fprintf(stderr,"%s: Full success. Decodes via both ico and rawimg APIs, and also matches our GIF file.\n",path);
  
  gif_image_del(gif);
  free(gifserial);
  ico_file_del(file);
  rawimg_del(rawimg);
  free(serial);
  return 0;
}
