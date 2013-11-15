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


#include "voxel.h"
#include "wrap_glut.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <errno.h>

static char const dirsep = '/';
static char const * userdir = ".haiko";

#ifdef DATADIR
static char const * sharedir = DATADIR;
#else
static char const * sharedir = "/usr/local/share/haiko";
#endif


voxel_t * voxel_create(double x, double y, double z,
		       double r, double g, double b)
{
  voxel_t * result = calloc(1, sizeof(voxel_t));
  if (NULL == result)
    return NULL;
  result->pos[0] = x;
  result->pos[1] = y;
  result->pos[2] = z;
  result->color[0] = r;
  result->color[1] = g;
  result->color[2] = b;
  result->length = 1;
  result->draw = voxel_draw_cube;
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


void voxel_draw_cube(voxel_t const * vv)
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(vv->pos[0] + vv->off[0],
	       vv->pos[1] + vv->off[1],
	       vv->pos[2] + vv->off[2]);
  glColor3d(vv->color[0], vv->color[1], vv->color[2]);
  glutSolidCube(vv->length);
  glPopMatrix();
}


void voxel_draw_sphere(voxel_t const * vv)
{
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(vv->pos[0] + vv->off[0],
	       vv->pos[1] + vv->off[1],
	       vv->pos[2] + vv->off[2]);
  glColor3d(vv->color[0], vv->color[1], vv->color[2]);
  glutSolidSphere(vv->length, 12, 12);
  glPopMatrix();
}


void voxel_draw_list(voxel_t * first)
{
  while (NULL != first) {
    first->draw(first);
    first = first->next;
  }
}


void voxel_parse_init(voxel_parse_tab_t * parse_tab)
{
  int ii;
  parse_tab->empty = ' ';
  for (ii = 0; ii < VOXEL_PARSE_COLORMAP_SIZE; ++ii)
    parse_tab->red[ii] = -1;	/* red signals 'un-initialized' */
  parse_tab->layer = -1;
  parse_tab->line = -1;
  parse_tab->error = 0;
  /*   parse_tab->debug = 0; */
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
  
  if (VOXEL_PARSE_COLORMAP_SIZE <= colorchar) {
    /* "never" happens ... for now */
    fprintf(stderr,
	    "voxel_parse_color(): colorchar %c (%d) is out of range %d\n",
	    (int) colorchar, (int) colorchar, VOXEL_PARSE_COLORMAP_SIZE);
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
    
    if (VOXEL_PARSE_COLORMAP_SIZE <= *line) { /* "never" happens */
      fprintf(stderr,
	      "voxel_parse_line(): colorchar %c (%d) is out of range %d\n",
	      (int) *line, (int) *line, VOXEL_PARSE_COLORMAP_SIZE);
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


/**
   \return 1 if fname begins with dirsep, 0 if not, and -1 on error
   (NULL or empty fname)
*/
static int is_absolute(char const * fname)
{
  if (NULL == fname)
    return -1;
  if (1 > strlen(fname))
    return -1;
  if (dirsep == fname[0])
    return 1;
  return 0;
}


/**
   \return 1 if fname contains a dirsep, 0 if not, and -1 on error
   (NULL or empty fname)
*/
static int has_dirsep(char const * fname)
{
  size_t len, ii;
  if (NULL == fname)
    return -1;
  len = strlen(fname);
  if (1 > len)
    return -1;
  for (ii = 0; ii < len; ii++)
    if (dirsep == fname[ii])
      return 1;
  return 0;
}


/**
   Concatenates two paths lhs+rhs, adding dirsep between them, then
   calls realpath() to canonicalize the result.
   
   \return 0 on success, -1 on argument error (NULL or empty lhs or
   rhs), -2 if realpath() fails (in which case an error message is
   written to stderr).
*/
static int path_concat(/** buf is assumed to be char[PATH_MAX] and
			   will be overwritten */
		       char * buf, char const * lhs, char const * rhs,
		       int verbose)
{
  char lbuf[PATH_MAX];
  size_t lhslen, rhslen;
  
  if (verbose)
    printf("  path_concat(buf, \"%s\", \"%s\")\n", lhs, rhs);
  
  if ((NULL == lhs) || (NULL == rhs)) {
    if (verbose)
      printf("  path_concat(): lhs or rhs is NULL\n");
    return -1;
  }
  lhslen = strlen(lhs);
  rhslen = strlen(rhs);
  if ((1 > lhslen) || (1 > rhslen)) {
    if (verbose)
      printf("  path_concat(): lhs or rhs is empty\n");
    return -1;
  }
  if (PATH_MAX - 1 <= lhslen + rhslen) { /* -1 because we add a dirsep */
    if (verbose)
      printf("  path_concat(): lhs+rhs is too long\n");
    return -1;
  }
  
#ifndef OPENBSD
  {
    char * cur = stpcpy(lbuf, lhs);
    *cur = dirsep;
    strcpy(cur + 1, rhs);
  }
#else
  {
    char dsbuf[] = { dirsep, '\0' };
    strcpy(lbuf, lhs);
    strcat(lbuf, dsbuf);
    strcat(lbuf, rhs);
  }
#endif
  
  if (verbose)
    printf("  path_concat(): calling realpath(\"%s\", buf)\n", lbuf);
  
  if (NULL == realpath(lbuf, buf)) {
    if (verbose)
      printf("  path_concat(): %s: %s\n", buf, strerror(errno));
    return -2;
  }
  return 0;
}


FILE * voxel_resolve_file(char const * fname, int verbose)
{
  FILE * result = NULL;
  char buf[PATH_MAX];
  char * home = getenv("HOME");
  char * haiko_path = getenv("HAIKO_PATH");
  char userdir_path[PATH_MAX];
  char sharedir_path[PATH_MAX];
  
  if (NULL == home) {
    if (verbose)
      printf("no home directory\n");
    userdir_path[0] = '\0';
  }
  else {
    if (verbose)
      printf("abspath of userdir \"%s\" + \"%s\"\n", home, userdir);
    switch (path_concat(userdir_path, home, userdir, verbose)) {
    case -1:
      if (verbose)
	printf("  ERROR in path_concat()\n");
      userdir_path[0] = '\0';
      break;
    case -2:
      if (verbose)
	printf("  realpath() failed\n");
      userdir_path[0] = '\0';
      break;
    case 0:
      if (verbose)
	printf("  RESOLVED \"%s\"\n", userdir_path);
      break;
    default:
      fprintf(stderr,
	      "voxel_resolve_file(): BUG? unhandled path_concat() retval\n");
      userdir_path[0] = '\0';
    }
    if (('\0' != userdir_path[0]) && verbose)
      printf("userdir_path \"%s\"\n", userdir_path);
  }
  
  if (NULL == realpath(sharedir, sharedir_path)) {
    if (verbose)
      printf("invalid sharedir \"%s\"\n  %s: %s\n",
	     sharedir, sharedir_path, strerror(errno));
    sharedir_path[0] = '\0';
  }
  else if (verbose)
    printf("sharedir_path \"%s\"\n", sharedir_path);

  if (verbose)
    printf("resolving filename \"%s\"\n", fname);
  
  switch (is_absolute(fname)) {
  case -1:
    if (verbose)
      printf("  ERROR in is_absolute()\n");
    break;
  case 0:
    /* not absolute */
    break;
  case 1:
    if (verbose)
      printf("  resolved absolute path\n");
    result = fopen(fname, "r");
    if (NULL != result) {
      if (verbose)
	printf("  FOUND absolute path\n");
      return result;
    }
    if (verbose)
      printf("  absolute path is not a (valid) file\n");
    break;
  default:
    fprintf(stderr,
	    "voxel_resolve_file(): BUG? unhandled is_absolute() retval\n");
  }
  
  switch (has_dirsep(fname)) {
  case -1:
    if (verbose)
      printf("  ERROR in has_dirsep()\n");
    break;
  case 0:
    /* no dirsep in fname */
    break;
  case 1:
    if (verbose)
      printf("  resolved relative path\n");
    result = fopen(fname, "r");
    if (NULL != result) {
      if (verbose)
	printf("  FOUND relative path\n");
      return result;
    }
    if (verbose)
      printf("  relative path is not a (valid) file\n");
    break;
  default:
    fprintf(stderr,
	    "voxel_resolve_file(): BUG? unhandled has_dirsep() retval\n");
  }
  
  if (NULL == haiko_path) {
    if (verbose)
      printf("  no HAIKO_PATH\n");
  }
  else {
    if (verbose)
      printf("  HAIKO_PATH + fname = \"%s\" + \"%s\"\n", haiko_path, fname);
    switch (path_concat(buf, haiko_path, fname, verbose)) {
    case -1:
      if (verbose)
	printf("    ERROR in path_concat()\n");
      break;
    case -2:
      if (verbose)
	printf("    realpath() failed\n");
      break;
    case 0:
      if (verbose)
	printf("    resolved file in HAIKO_PATH: \"%s\"\n", buf);
      result = fopen(buf, "r");
      if (NULL != result) {
	if (verbose)
	  printf("  FOUND in HAIKO_PATH\n");
	return result;
      }
      if (verbose)
	printf("  invalid file in HAIKO_PATH\n");
      break;
    default:
      fprintf(stderr,
	      "voxel_resolve_file(): BUG? unhandled path_concat() retval\n");
    }
  }
  
  if ('\0' == userdir_path[0]) {
    if (verbose)
      printf("  no userdir_path\n");
  }
  else {
    if (verbose)
      printf("  userdir_path + fname = \"%s\" + \"%s\"\n",
	     userdir_path, fname);
    switch (path_concat(buf, userdir_path, fname, verbose)) {
    case -1:
      if (verbose)
	printf("    ERROR in path_concat()\n");
      break;
    case -2:
      if (verbose)
	printf("    realpath() failed\n");
      break;
    case 0:
      if (verbose)
	printf("    resolved file in userdir: \"%s\"\n", buf);
      result = fopen(buf, "r");
      if (NULL != result) {
	if (verbose)
	  printf("  FOUND in userdir\n");
	return result;
      }
      if (verbose)
	printf("  invalid file in userdir\n");
      break;
    default:
      fprintf(stderr,
	      "voxel_resolve_file(): BUG? unhandled path_concat() retval\n");
    }
  }
  
  if ('\0' == sharedir_path[0]) {
    if (verbose)
      printf("  no sharedir_path\n");
  }
  else {
    if (verbose)
      printf("  sharedir_path + fname = \"%s\" + \"%s\"\n",
	     sharedir_path, fname);
    switch (path_concat(buf, sharedir_path, fname, verbose)) {
    case -1:
      if (verbose)
	printf("    ERROR in path_concat()\n");
      break;
    case -2:
      if (verbose)
	printf("    realpath() failed\n");
      break;
    case 0:
      if (verbose)
	printf("    resolved file in sharedir: \"%s\"\n", buf);
      result = fopen(buf, "r");
      if (NULL != result) {
	if (verbose)
	  printf("  FOUND in sharedir\n");
	return result;
      }
      if (verbose)
	printf("  invalid file in sharedir\n");
      break;
    default:
      fprintf(stderr,
	      "voxel_resolve_file(): BUG? unhandled path_concat() retval\n");
    }
  }
  
  if (verbose)
    printf("  SORRY, could not resolve file \"%s\"\n", fname);
  return NULL;
}
