/* 
 * Copyright (C) 2008 Roland Philippsen <roland.philippsen@gmx.net>
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


#ifndef HAIKO_VOXEL_H
#define HAIKO_VOXEL_H

#include <stdio.h>


#define HAIKO_PARSE_COLORMAP_SIZE 256


typedef struct voxel_s {
  /*   double delta; */
  double x, y, z;
  double r, g, b;
  /*   int border_on; */
  /*   double border_r, border_g, border_b; */
  struct voxel_s * next;
} voxel_t;


typedef struct voxel_parse_tab_s {
  char empty;
  double red[HAIKO_PARSE_COLORMAP_SIZE];
  double green[HAIKO_PARSE_COLORMAP_SIZE];
  double blue[HAIKO_PARSE_COLORMAP_SIZE];
  int layer;
  int line;
  int error;			/* 0 as long as no error occurred */
  int debug;			/* print debug messages if debug != 0 */
  voxel_t * first;
  voxel_t * last;
  int nlayers;
  int nlines;
  int ncolumns;
} voxel_parse_tab_t;


voxel_t * voxel_create(double x, double y, double z,
		       double r, double g, double b);
void voxel_free_list(voxel_t * first);

void voxel_draw(voxel_t const * vv);
void voxel_draw_list(voxel_t const * first);

int voxel_parse_file(FILE * configfile, voxel_parse_tab_t * parse_tab);
void voxel_parse_init(voxel_parse_tab_t * parse_tab);
void voxel_parse_empty(voxel_parse_tab_t * parse_tab, char emptychar);
void voxel_parse_color(voxel_parse_tab_t * parse_tab, unsigned char colorchar,
		       double red, double green, double blue);
void voxel_parse_layer(voxel_parse_tab_t * parse_tab);
void voxel_parse_line(voxel_parse_tab_t * parse_tab,
		      const unsigned char * line);

#endif /* HAIKO_VOXEL_H */
