/* 
 * Copyright (C) 2008 Roland Philippsen <roland dot philippsen at gmx dot net>
 * 
 * BSD-style license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of
 *    contributors to this software may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR THE CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "wrap_gl.h"
#include <stdio.h>
#include <stdlib.h>

/* until end of file */
#ifdef HAVE_PNG_H

# include <png.h>

# define BPP 3


int wrap_gl_write_png(char const * filename, int width, int height)
{
  png_byte * pixel;
  png_bytep * row;
  int ii;
  FILE * fp;
  png_structp png_ptr;
  png_infop info_ptr;
  
  fp = fopen(filename, "wb");
  if (NULL == fp) {
    fprintf(stderr, "wrap_gl_write_png(%s, %d, %d): ",
	    filename, width, height);
    perror("fopen()");
    goto fail_open;
  }
  
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if ( NULL == png_ptr) {
    fprintf(stderr, "wrap_gl_write_png(%s, %d, %d):"
	    " png_create_write_struct() failed\n",
	    filename, width, height);
    goto fail_png_cwrite;
  }
  
  info_ptr = png_create_info_struct(png_ptr);
  if ( NULL == info_ptr){
    fprintf(stderr, "wrap_gl_write_png(%s, %d, %d):"
	    " png_create_info_struct() failed\n",
	    filename, width, height);
    goto fail_png_cinfo;
  }
  
  if (setjmp(png_jmpbuf(png_ptr))) {
    fprintf(stderr, "wrap_gl_write_png(%s, %d, %d): setjmp() failed\n",
	    filename, width, height);
    goto fail_setjmp;
  }
  
  /* allocate pixel buffers */
  
  pixel = calloc(width * height * BPP, sizeof(*pixel));
  if (NULL == pixel) {
    fprintf(stderr, "wrap_gl_write_png(%s, %d, %d): out of memory\n",
	    filename, width, height);
    goto fail_pixel_alloc;
  }
  
  row = calloc(height, sizeof(*row));
  if (NULL == row) {
    fprintf(stderr, "wrap_gl_write_png(%s, %d, %d): out of memory\n",
	    filename, width, height);
    goto fail_row_alloc;
  }
  
  /* Open GL uses lower-left, PNG uses upper left */
  for (ii = 0; ii < height; ++ii)
    row[height - ii - 1] = (png_bytep) pixel + ii * width * BPP;  
  
  /* read Open GL buffer into PNG pixel buffer */
  
  glReadBuffer(GL_FRONT);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixel);
  
  /* save file */
  
  png_init_io(png_ptr, fp);
  png_set_IHDR(png_ptr, info_ptr, width, height,
	       sizeof(png_byte) * 8,
	       PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE,
	       PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, row);
  png_write_end(png_ptr, info_ptr);
  
  free(row);
  free(pixel);
  png_destroy_info_struct(png_ptr, &info_ptr);
  png_destroy_write_struct(&png_ptr,  (png_infopp) NULL);
  fclose(fp);
  
  return 0;
  
 fail_row_alloc:
  free(pixel);
 fail_pixel_alloc:
 fail_setjmp:
  png_destroy_info_struct(png_ptr, &info_ptr);
 fail_png_cinfo:
  png_destroy_write_struct(&png_ptr,  (png_infopp) NULL);
 fail_png_cwrite:
  fclose(fp);
 fail_open:
  return -1;
}

#endif /* HAVE_PNG_H */
