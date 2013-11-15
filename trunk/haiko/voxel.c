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


#include "voxel.h"
#include "wrap_glut.h"
#include <stdlib.h>
#include <stdio.h>


voxel_t * voxel_create(double x, double y, double z,
		       double r, double g, double b)
{
  voxel_t * result = calloc(1, sizeof(voxel_t));
  if (NULL == result)
    return NULL;
  result->x = x;
  result->y = y;
  result->z = z;
  result->r = r;
  result->g = g;
  result->b = b;
  result->next = NULL;
  return result;
}


void voxel_free_list(voxel_t * first)
{
  while (NULL != first) {
    voxel_t * tmp = first->next;
    free(first);
    first = tmp;
  }
}


void voxel_draw(voxel_t const * vv)
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(vv->x, vv->y, vv->z);
  glColor3d(vv->r, vv->g, vv->b);
  glutSolidCube(1.0);
  glPopMatrix();
}


void voxel_draw_list(voxel_t const * first)
{
  while (NULL != first) {
    voxel_draw(first);
    first = first->next;
  }
}


void voxel_parse_init(voxel_parse_tab_t * parse_tab)
{
  int ii;
  parse_tab->empty = ' ';
  for (ii = 0; ii < HAIKO_PARSE_COLORMAP_SIZE; ++ii)
    parse_tab->red[ii] = -1;	/* red signals 'un-initialized' */
  parse_tab->layer = -1;
  parse_tab->line = -1;
  parse_tab->error = 0;
  parse_tab->debug = 0;
  parse_tab->first = NULL;
  parse_tab->last = NULL;
  parse_tab->nlayers = 0;
  parse_tab->nlines = 0;
  parse_tab->ncolumns = 0;
}


void voxel_parse_empty(voxel_parse_tab_t * parse_tab, char emptychar)
{
  if ( ! parse_tab->error)
    parse_tab->empty = emptychar;
}


void voxel_parse_color(voxel_parse_tab_t * parse_tab, unsigned char colorchar,
		       double red, double green, double blue)
{
  if (parse_tab->error)
    return;
  
  if (HAIKO_PARSE_COLORMAP_SIZE <= colorchar) {
    /* "never" happens ... for now */
    fprintf(stderr,
	    "voxel_parse_color(): colorchar %c (%d) is out of range %d\n",
	    (int) colorchar, (int) colorchar, HAIKO_PARSE_COLORMAP_SIZE);
    parse_tab->error = 1;
    return;
  }
  parse_tab->red[colorchar] =   red;
  parse_tab->green[colorchar] = green;
  parse_tab->blue[colorchar] =  blue;
}


void voxel_parse_layer(voxel_parse_tab_t * parse_tab)
{
  if (parse_tab->error)
    return;
  
  ++parse_tab->layer;
  ++parse_tab->nlayers;
  parse_tab->line = 0;
}


void voxel_parse_line(voxel_parse_tab_t * parse_tab,
		      const unsigned char * line)
{
  int column;
  
  if (parse_tab->error)
    return;
  
  if (parse_tab->debug)
    printf("voxel_parse_line(%s)\n", line);
  
  if (0 > parse_tab->layer) {
    fprintf(stderr, "voxel_parse_line(): no layer for line\n");
    parse_tab->error = 1;
    return ;
  }
  
  for (column = 0; *line != '\0'; ++column, ++line) {
    voxel_t * newvoxel;
    
    if (parse_tab->empty == *line)
      continue;
    
    if (HAIKO_PARSE_COLORMAP_SIZE <= *line) { /* "never" happens */
      fprintf(stderr,
	      "voxel_parse_line(): colorchar %c (%d) is out of range %d\n",
	      (int) *line, (int) *line, HAIKO_PARSE_COLORMAP_SIZE);
      parse_tab->error = 1;
      return;
    }
    if (0 > parse_tab->red[*line]) {
      fprintf(stderr,
	      "voxel_parse_line(): undeclared color char %c (%d)"
	      " at layer %d line %d column %d\n",
	      (int) *line, (int) *line, parse_tab->layer, parse_tab->line,
	      column);
      parse_tab->error = 1;
      return;
    }
    
    if (parse_tab->debug)
      printf("  voxel [%d %d %d] (%f  %f  %f)\n",
	     parse_tab->layer, parse_tab->line, column,
	     parse_tab->red[*line], parse_tab->green[*line],
	     parse_tab->blue[*line]);
    
    newvoxel = voxel_create(parse_tab->layer, parse_tab->line, column,
			    parse_tab->red[*line], parse_tab->green[*line],
			    parse_tab->blue[*line]);
    if (NULL == newvoxel) {
      fprintf(stderr,
	      "voxel_parse_line(): could not create"
	      " voxel [%d %d %d] (%f  %f  %f)\n",
	      parse_tab->layer, parse_tab->line, column,
	      parse_tab->red[*line], parse_tab->green[*line],
	      parse_tab->blue[*line]);
      parse_tab->error = 1;
      return;
    }
    
    if (NULL == parse_tab->first) {
      parse_tab->first = newvoxel;
      parse_tab->last = newvoxel;
    }
    else {
      parse_tab->last->next = newvoxel;
      parse_tab->last = newvoxel;      
    }
  }
  
  ++parse_tab->line;
  
  if (column > parse_tab->ncolumns)
    parse_tab->ncolumns = column;
  if (parse_tab->line > parse_tab->nlines)
    parse_tab->nlines = parse_tab->line;
}
