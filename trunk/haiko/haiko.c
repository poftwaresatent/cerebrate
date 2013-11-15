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


#include "gltrackball.h"
#include "wrap_glut.h"
#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>


static void usage(FILE * os);
static void parse_options(int argc, char ** argv);
static void init_glut(int * argc, char ** argv, int width, int height);
static void reshape(int width, int height);
static void draw();
static void keyboard(unsigned char key, int x, int y);
static void timer(int handle);
static void mouse(int button, int state, int x, int y);
static void motion(int x, int y);
static void cleanup();
static void just_axes();


static unsigned int timer_delay = 100;
static int winwidth, winheight;
static trackball_state * trackball;
static int left_down = 0;
static double theta = 0;
static voxel_t * voxel;
static int dim_x = 0;
static int dim_y = 0;
static int dim_z = 0;
static double view_rad = -1;
static double view_rel_dist = 2;
static double view_dist;
static double view_lrbt;	/* left, right, bottom, top */
static double view_near;
static double view_far;
static double center_x;
static double center_y;
static double center_z;
static double eye_x;
static double eye_y;
static double eye_z;
static int verbose = 0;		/* todo */
static int ortho_projection = 0; /* todo */
static int show_bounding_sphere = 0; /* todo */
static int axes_only = 0;	/* todo */
static char const * filename = "haiko.conf"; /* todo */


int main(int argc, char ** argv)
{
  if (0 != atexit(cleanup)) {
    perror("atexit()");
    exit(EXIT_FAILURE);
  }
  
  parse_options(argc, argv);
  
  winwidth = 400;
  winheight = 400;
  trackball = gltrackball_init();
  
  if (axes_only)
    just_axes();
  else {
    voxel_parse_tab_t voxel_parse_tab;
    FILE * configfile;
    if (0 == strcmp(filename, "--"))
      configfile = stdin;
    else {
      configfile = fopen(filename, "r");
      if (NULL == configfile) {
	perror("fopen()");
	exit(EXIT_FAILURE);
      }
    }
    
    voxel_parse_file(configfile, &voxel_parse_tab);
    if (stdin != configfile)
      fclose(configfile);
    
    voxel = voxel_parse_tab.first;
    if (voxel_parse_tab.error) {
      fprintf(stderr, "%s: error parsing voxels from '%s'\n",
	      argv[0], filename);
      exit(EXIT_FAILURE);
    }
    
    dim_x = voxel_parse_tab.nlayers;
    dim_y = voxel_parse_tab.nlines;
    dim_z = voxel_parse_tab.ncolumns;
  }
  
  if ((dim_x == 0) || (dim_y == 0) || (dim_z == 0)) {
    fprintf(stderr, "%s: degenerate dimensions [%d][%d][%d]\n",
	    argv[0], dim_x, dim_y, dim_z);
    exit(EXIT_FAILURE);
  }
  
  init_glut(&argc, argv, winwidth, winheight);
  
  glClearColor(0.0, 0.0, 0.0, 0.0);
  
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  
  glutMainLoop();
  
  return 0;
}


void init_glut(int * argc, char ** argv, int width, int height)
{
  int handle;
  
  glutInit(argc, argv);
  
#ifdef OSX
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB);
#else
  glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
#endif
  
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(width, height);
  
  handle = glutCreateWindow("tball");
  if (0 == handle) {
    fprintf(stderr, "%s: init_glut(): glutCreateWindow() failed\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  
  left_down = 0;
  theta = 0;
  
  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutTimerFunc(timer_delay, timer, handle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
}


void reshape(int width, int height)
{
  if (0 > view_rad) {
    view_rad = 0.5 * sqrt(pow(dim_x, 2) + pow(dim_y, 2) + pow(dim_z, 2));
    if (1.1 > view_rel_dist)
      view_rel_dist = 1.1;
    view_dist = view_rel_dist * view_rad;
    view_far = view_dist + 2 * view_rad;
    view_near = view_dist - view_rad;
    view_lrbt = 
      (view_rad * view_dist - pow(view_rad, 2))
      / sqrt(pow(view_dist, 2) - pow(view_rad, 2));
    center_x = 0.5 * (dim_x - 1);
    center_y = 0.5 * (dim_y - 1);
    center_z = 0.5 * (dim_z - 1);
    /* do not ask me why it is TWICE the center... */
    eye_x = 2 * center_x + 0.5 * view_rel_dist * dim_x / view_rad;
    eye_y = 2 * center_y + 0.5 * view_rel_dist * dim_y / view_rad;
    eye_z = 2 * center_z + 0.5 * view_rel_dist * dim_z / view_rad;
    if (verbose)
      printf("reshape() init:\n"
	     " view_rad: %g\n"
	     " view_rel_dist: %g\n"
	     " view_dist: %g\n"
	     " view_lrbt: %g\n"
	     " view_far: %g\n"
	     " view_near: %g\n"
	     " center_x: %g\n"
	     " center_y: %g\n"
	     " center_z: %g\n"
	     " eye_x: %g\n"
	     " eye_y: %g\n"
	     " eye_z: %g\n",
	     view_rad, view_rel_dist, view_dist, view_lrbt, view_far,
	     view_near, center_x, center_y, center_z, eye_x, eye_y, eye_z);
  }
  
  if (width > height)
    glViewport((width - height) / 2, 0, height, height);
  else
    glViewport(0, (height - width) / 2, width, width);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  
  
  if (ortho_projection)
    glOrtho(- view_rad, + view_rad,
	    - view_rad, + view_rad,
	    - view_rad, + view_rad);
  else
    glFrustum(- view_lrbt, + view_lrbt,
	      - view_lrbt, + view_lrbt,
	      view_near, view_far);
  
  winwidth = width;
  winheight = height;
}


void draw()
{
  GLfloat amb[] = { 0.5, 0.5, 0.5, 1.0 };
  GLfloat position[] = { dim_x, dim_y, dim_z, 1.0 };  
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.1 / view_rad);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01 / view_rad);
  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
  
  if (0 == ortho_projection)
    gluLookAt(eye_x, eye_y, eye_z,
	      0, 0, 0,
	      0, 1, 0);
  
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  
#define ENABLE_TRACKBALL
#ifdef ENABLE_TRACKBALL
  gltrackball_rotate(trackball);
#endif
  
#define ENABLE_THETA
#ifdef ENABLE_THETA
  glRotatef(theta * 100, 0.0, 1.0, 0.0);
#endif
  
  glTranslatef( - center_x, - center_y, - center_z);  
  
  voxel_draw_list(voxel);
  
  if (show_bounding_sphere) {
    glTranslatef(center_x, center_y, center_z);
    glColor3f(1.0, 0.5, 1.0);
    glutWireSphere(view_rad, 12, 12);
  }
  
  glFlush();
#ifndef OSX
  glutSwapBuffers();
#endif OSX
}


void keyboard(unsigned char key, int x, int y)
{
  switch(key){
  case 'q':
    exit(EXIT_SUCCESS);
    break;
  }
}


void timer(int handle)
{
  if (0 == left_down)
    theta += 0.01;
  
  glutSetWindow(handle);
  glutPostRedisplay();
  glutTimerFunc(timer_delay, timer, handle);
}


void mouse(int button, int state, int x, int y)
{
  if (GLUT_LEFT_BUTTON == button) {
    if (GLUT_DOWN == state) {
      left_down = 1;
      gltrackball_start (trackball, x, y, winwidth, winheight);
    }
    else
      left_down = 0;
  }
}


void motion(int x, int y)
{
  if (0 != left_down)
    gltrackball_track (trackball, x, y, winwidth, winheight);
}


void cleanup()
{
  if (NULL != trackball) {
    /*     printf("freeing trackball\n"); */
    free(trackball);
  }
  if (NULL != voxel) {
    /*     printf("freeing voxels\n"); */
    voxel_free_list(voxel);
  }
}


void usage(FILE * os)
{
  fprintf(os,
	  "options:\n"
	  "  -h           help (this message)\n"
	  "  -v           enable debug messages\n"
	  "  -o           use orthogonal projection\n"
	  "  -s           show bounding sphere\n"
	  "  -a           no config file, just axes voxels\n"
	  "  -f  <file>   configuration file name ('--' means stdin)\n"
	  "  -d  <dist>   set relative viewing distance\n"
	  "  -t  <timer>  set timer interval [ms]\n"
	  "\n"
	  "default: -f haiko.conf -d 2 -t 100\n");
}


void parse_options(int argc, char ** argv)
{
  int ii;
  for (ii = 1; ii < argc; ++ii) {
    if (argv[ii][0] != '-') {
      fprintf(stderr, "%s: problem with option '%s'\n", argv[0], argv[ii]);
      usage(stderr);
      exit(EXIT_FAILURE);
    }
    else
      switch (argv[ii][1]) {
	
      case 'h':
	usage(stdout);
	exit(EXIT_SUCCESS);
	
      case 'v':
	verbose = 1;
	break;
	
      case 'o':
	ortho_projection = 1;
	break;
	
      case 's':
	show_bounding_sphere = 1;
	break;
	
      case 'a':
	axes_only = 1;
	break;
	
      case 'f':
	++ii;
	if (ii >= argc) {
	  fprintf(stderr, "%s: -f requires a filename argument\n", argv[0]);
	  usage(stderr);
	  exit(EXIT_FAILURE);
	}
	filename = argv[ii];
	break;
	
      case 'd':
	++ii;
	if (ii >= argc) {
	  fprintf(stderr, "%s: -d requires a relative distance argument\n",
		  argv[0]);
	  usage(stderr);
	  exit(EXIT_FAILURE);
	}
	{
	  char * endptr;
	  view_rel_dist = strtod(argv[ii], &endptr);
	  if (endptr == argv[ii]) {
	    fprintf(stderr,
		    "%s: error reading distance '%s' (number expected)\n",
		    argv[0], argv[ii]);
	    usage(stderr);
	    exit(EXIT_FAILURE);
	  }
	}
	break;
	
      case 't':
	++ii;
	if (ii >= argc) {
	  fprintf(stderr, "%s: -t requires a timer [ms] argument\n",
		  argv[0]);
	  usage(stderr);
	  exit(EXIT_FAILURE);
	}
	{
	  char * endptr;
	  long delay = strtol(argv[ii], &endptr, 0);
	  if ((endptr == argv[ii]) || (0 > delay) || (UINT_MAX < delay)) {
	    fprintf(stderr,
		    "%s: error reading delay '%s'"
		    " (number between 1 and %ud expected)\n",
		    argv[0], argv[ii], UINT_MAX);
	    usage(stderr);
	    exit(EXIT_FAILURE);
	  }
	  timer_delay = (unsigned int) delay;
	}
	break;

      default:
	fprintf(stderr, "%s: invalid option '%s'\n", argv[0], argv[ii]);
	usage(stderr);
	exit(EXIT_FAILURE);
      }
  }
}


void just_axes()
{
  voxel_t * last;
  voxel = voxel_create(0, 0, 0, 1.0, 1.0, 1.0);
  if (NULL == voxel) {
    fprintf(stderr, "just_axes(): out of memory?\n");
    exit(EXIT_FAILURE);
  }
  last = voxel;
  
  last->next = voxel_create(1, 0, 0, 1.0, 0.0, 0.0);
  if (NULL == last->next) {
    fprintf(stderr, "just_axes(): out of memory?\n");
    exit(EXIT_FAILURE);
  }
  last = last->next;
  
  last->next = voxel_create(0, 1, 0, 0.0, 1.0, 0.0);
  if (NULL == last->next) {
    fprintf(stderr, "just_axes(): out of memory?\n");
    exit(EXIT_FAILURE);
  }
  last = last->next;
  
  last->next = voxel_create(0, 0, 1, 0.0, 0.0, 1.0);
  if (NULL == last->next) {
    fprintf(stderr, "just_axes(): out of memory?\n");
    exit(EXIT_FAILURE);
  }
  last = last->next;    	/* not needed, for future extension */
  
  dim_x = 2;
  dim_y = 2;
  dim_z = 2;
}
