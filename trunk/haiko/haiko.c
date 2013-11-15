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


typedef struct draw_data_s {
  double length;		/* cube side length, sphere radius, or so */
  double xoff, yoff, zoff;	/* offsets along x, y, and z */
  double balloondist;		/* distance measure for balloon */
  double warpdist;		/* distance measure for warp */
  double xwarp, ywarp, zwarp;	/* unit warp vector */
} draw_data_t;


typedef struct wibble_s {
  int initialized;
  double duration, period, speed, minval, maxgrow, power;
  double (*compute)(struct wibble_s * ww, double normdist, double instant);
  void (*init)(struct voxel_s * first);
} wibble_t;


typedef struct init_s {
  void (*init_rad)(struct voxel_s * first);
  void (*init_x)(struct voxel_s * first);
  void (*init_y)(struct voxel_s * first);
  void (*init_z)(struct voxel_s * first);
} init_t;


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

static void cube_drawer(struct voxel_s * vv);
static void sphere_drawer(struct voxel_s * vv);
static void init_view();
static void init_drawers_cube(struct voxel_s * first);
static void init_drawers_sphere(struct voxel_s * first);

static void update_drawers_wiggle(struct voxel_s * first);
static void update_balloon(struct voxel_s * first);
static void update_warp(struct voxel_s * first);

static double sawtooth(double duration, double period,
		       double outside_value,
		       double normdist, double instant);

static double wibble_bump(struct wibble_s * ww,
			  double normdist, double instant);
static double wibble_spike(struct wibble_s * ww,
			   double normdist, double instant);

static void init_balloon_rad(struct voxel_s * first);
static void init_balloon_x(struct voxel_s * first);
static void init_balloon_y(struct voxel_s * first);
static void init_balloon_z(struct voxel_s * first);

static void init_warp_rad(struct voxel_s * first);
static void init_warp_x(struct voxel_s * first);
static void init_warp_y(struct voxel_s * first);
static void init_warp_z(struct voxel_s * first);

static int parse_wibble(wibble_t * ww, init_t *ii, char * argument);


static unsigned int timer_delay = 100;
static int winwidth, winheight;
static trackball_state * trackball;
static int left_down = 0;
static double theta = 0;
static int enable_spin = 1;
static int enable_balloon = 1;
static int enable_warp = 1;
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
static int verbose = 0;
static int ortho_projection = 0;
static int show_bounding_sphere = 0;
static int axes_only = 0;
static int reflection_effect = 0;
static char const * filename = "haiko.conf";

static void (*init_drawers)(struct voxel_s * first) = init_drawers_sphere;

static init_t init_balloon;
static wibble_t balloon;

static init_t init_warp;
static wibble_t warp;


int main(int argc, char ** argv)
{
  init_balloon.init_rad = init_balloon_rad;
  init_balloon.init_x = init_balloon_x;
  init_balloon.init_y = init_balloon_y;
  init_balloon.init_z = init_balloon_z;
  
  init_warp.init_rad = init_warp_rad;
  init_warp.init_x = init_warp_x;
  init_warp.init_y = init_warp_y;
  init_warp.init_z = init_warp_z;
  
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
  
  init_view();
  balloon.initialized = 0;
  warp.initialized = 0;
  init_drawers(voxel);
  
  init_glut(&argc, argv, winwidth, winheight);
  
  glClearColor(0.0, 0.0, 0.2, 0.0);
  
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  
  if (reflection_effect) {
    GLfloat spec[] = { 0.5, 0.8, 1.0, 1.0 };
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, 20);
  }
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  {
    GLfloat amb[] = { 0.5, 0.5, 0.5, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
  }
  
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
  
  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutTimerFunc(timer_delay, timer, handle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
}


void init_view()
{
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
    printf("init_view():\n"
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


void reshape(int width, int height)
{
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
  GLfloat position[] = { dim_x, dim_y, dim_z, 1.0 };  
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.1 / view_rad);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01 / view_rad);
  
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
  glRotatef(180 + theta * 100, 0.0, 1.0, 0.0);
#endif
  
  glRotatef(180, 1.0, 0.0, 0.0);
  glTranslatef( - center_x, - center_y, - center_z);  
  
/*   voxel_draw_cube_list(voxel, 0.8); */
/*   voxel_draw_sphere_list(voxel, 0.8); */
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
  case ' ':
    if (enable_spin)
      enable_spin = 0;
    else
      enable_spin = 1;
    break;
  case 'b':
    if (enable_balloon)
      enable_balloon = 0;
    else
      enable_balloon = 1;
    break;
  case 'w':
    if (enable_warp)
      enable_warp = 0;
    else
      enable_warp = 1;
    break;
  }
}


void timer(int handle)
{
  if (0 == left_down) {
    if (enable_spin)
      theta += 0.01;
    if (enable_balloon)
      update_balloon(voxel);
    if (enable_warp)
      update_warp(voxel);
  }
  
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
  static char const * help =
    "options:\n"
    "  -h           help (this message)\n"
    "  -v           enable debug messages\n"
    "  -o           use orthogonal projection\n"
    "  -s           show bounding sphere\n"
    "  -a           no config file, just axes voxels\n"
    "  -r           enable reflection effect\n"
    "  -f  <file>   configuration file name ('--' means stdin)\n"
    "  -d  <dist>   set relative viewing distance\n"
    "  -t  <timer>  set timer interval [ms]\n"
    "  -D  <style>  drawing style: cube or sphere\n"
    "\n"
    "  Ballooning effect option:\n"
    "  -b  <wibble:direction:duration:period:speed:minval:maxgrow:power>\n"
    "               wibble: spike or bump\n"
    "               direction: rad, x, y, or z\n"
    "               duration: active fraction of period\n"
    "               period: cycle length (normalized, 2 is good)\n"
    "               speed: wave speed (normalized, 0.03 is good)\n"
    "               minval: minimum voxel length (inactive phase)\n"
    "               maxgrow: maximum additional length (active phase)\n"
    "               power: shape exponent (2 is reasonable)\n"
    "\n"
    "  Warping effect option:\n"
    "  -w <wibble:direction:duration:period:speed:minval:maxgrow:power>\n"
    "               see -b for details\n"
    "\n"
    "default: -f haiko.conf -d 2 -t 100 -D sphere\n"
    "\n"
    "example ballooning effects:\n"
    "  -b bump:rad:0.3:2.5:0.03:0.75:0.75:1.5\n"
    "  -b spike:rad:0.8:3.5:0.15:0.7:0.9:3.5\n"
    "  -b spike:rad:0.8:3.5:0.15:1:1:3.5 -D cube\n"
    "\n"
    "example warping effects:\n"
    "  -w spike:rad:0.3:1.5:0.02:0:0.75:1.5 -D cube\n"
    "  -w bump:y:0.3:1.5:0.02:0:0.6:1.2\n"
    "\n";
  fprintf(os, help);
}


void parse_options(int argc, char ** argv)
{
  int ii;
  for (ii = 1; ii < argc; ++ii) {
    if (argv[ii][0] != '-') {
      usage(stderr);
      fprintf(stderr, "\n%s: problem with option '%s'\n", argv[0], argv[ii]);
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
	
      case 'r':
	reflection_effect = 1;
	break;
	
      case 'f':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -f requires a filename argument\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	filename = argv[ii];
	break;
	
      case 'd':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -d requires a relative distance argument\n",
		  argv[0]);
	  exit(EXIT_FAILURE);
	}
	{
	  char * endptr;
	  view_rel_dist = strtod(argv[ii], &endptr);
	  if (endptr == argv[ii]) {
	    usage(stderr);
	    fprintf(stderr,
		    "\n%s: error reading distance '%s' (number expected)\n",
		    argv[0], argv[ii]);
	    exit(EXIT_FAILURE);
	  }
	}
	break;
	
      case 't':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -t requires a timer [ms] argument\n",
		  argv[0]);
	  exit(EXIT_FAILURE);
	}
	{
	  char * endptr;
	  long delay = strtol(argv[ii], &endptr, 0);
	  if ((endptr == argv[ii]) || (0 > delay) || (UINT_MAX < delay)) {
	    usage(stderr);
	    fprintf(stderr,
		    "\n%s: error reading delay '%s'"
		    " (number between 1 and %ud expected)\n",
		    argv[0], argv[ii], UINT_MAX);
	    exit(EXIT_FAILURE);
	  }
	  timer_delay = (unsigned int) delay;
	}
	break;
	
      case 'D':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -D requires a style argument\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	if (0 == strcmp(argv[ii], "cube"))
	  init_drawers = init_drawers_cube;
	else if (0 == strcmp(argv[ii], "sphere"))
	  init_drawers = init_drawers_sphere;
	else {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -D accepts 'cube' or 'sphere', not '%s'\n",
		  argv[0], argv[ii]);
	  exit(EXIT_FAILURE);
	}
	break;

      case 'b':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -b requires (quite) an argument\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	if (0 != parse_wibble(&balloon, &init_balloon, argv[ii])) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: error parsing -b argument"
		  " (see messages above the help text)\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	break;
	
      case 'w':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -w requires (quite) an argument\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	if (0 != parse_wibble(&warp, &init_warp, argv[ii])) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: error parsing -w argument"
		  " (see messages above the help text)\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	break;
	
      default:
	usage(stderr);
	fprintf(stderr, "\n%s: invalid option '%s'\n", argv[0], argv[ii]);
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


void cube_drawer(struct voxel_s * vv)
{
  draw_data_t * data = (draw_data_t *) vv->draw_data;
  voxel_draw_cube(vv, data->length, data->xoff, data->yoff, data->zoff);
}


void sphere_drawer(struct voxel_s * vv)
{
  draw_data_t * data = (draw_data_t *) vv->draw_data;
  voxel_draw_sphere(vv, data->length, data->xoff, data->yoff, data->zoff);
}


void init_drawers_cube(struct voxel_s * first)
{
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    first->draw = cube_drawer;
    data->length = 1;
    data->xoff = 0;
    data->yoff = 0;
    data->zoff = 0;
    first = first->next;
  }
}


void init_drawers_sphere(struct voxel_s * first)
{
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    first->draw = sphere_drawer;
    data->length = 1;
    data->xoff = 0;
    data->yoff = 0;
    data->zoff = 0;
    first = first->next;
  }
}


void update_drawers_wiggle(struct voxel_s * first)
{
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    data->xoff = (0.1 * random()) / LONG_MAX;
    data->yoff = (0.1 * random()) / LONG_MAX;
    data->zoff = (0.1 * random()) / LONG_MAX;
    first = first->next;
  }
}


void update_balloon(struct voxel_s * first)
{
  static double tt = 0;
  if (NULL == balloon.compute)
    return;
  if (0 == balloon.initialized) {
    balloon.init(voxel);
    balloon.initialized = 1;
  }
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    data->length = balloon.compute(&balloon, data->balloondist, tt);
    first = first->next;
  }
  tt += balloon.speed;
}


void update_warp(struct voxel_s * first)
{
  static double tt = 0;
  if (NULL == warp.compute)
    return;
  if (0 == warp.initialized) {
    warp.init(voxel);
    warp.initialized = 1;
  }
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    double len = warp.compute(&warp, data->warpdist, tt);
    data->xoff = len * data->xwarp;
    data->yoff = len * data->ywarp;
    data->zoff = len * data->zwarp;
    first = first->next;
  }
  tt += warp.speed;
}


double sawtooth(double duration, double period,
		double outside_value,
		double dist, double instant)
{
  dist = fmod(dist - instant, period);
  if (dist < 0)
    dist += period;
  if ((dist >= 0) && (dist <= duration))
    return 2 * dist / duration - 1;
  return outside_value;
}


double wibble_bump(struct wibble_s * ww,
		   double dist, double instant)
{
  double aa = sawtooth(ww->duration, ww->period, 1, dist, instant);
  return ww->maxgrow * (1 - pow(fabs(aa), ww->power)) + ww->minval;
}


double wibble_spike(struct wibble_s * ww,
		    double dist, double instant)
{
  double aa = sawtooth(ww->duration, ww->period, 1, dist, instant);
  return ww->maxgrow * pow(1 - fabs(aa), ww->power) + ww->minval;
}


void init_balloon_rad(struct voxel_s * first)
{
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    data->balloondist = sqrt(pow(first->x - center_x, 2) +
			     pow(first->y - center_y, 2) +
			     pow(first->z - center_z, 2)) / view_rad;
    first = first->next;
  }
}


void init_balloon_x(struct voxel_s * first)
{
  if (dim_x > 1)
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->balloondist = first->x / (dim_x - 1);
      first = first->next;
    }
  else
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->balloondist = 0;
      first = first->next;
    }
}


void init_balloon_y(struct voxel_s * first)
{
  if (dim_y > 1)
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->balloondist = first->y / (dim_y - 1);
      first = first->next;
    }
  else
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->balloondist = 0;
      first = first->next;
    }
}


void init_balloon_z(struct voxel_s * first)
{
  if (dim_z > 1)
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->balloondist = first->z / (dim_z - 1);
      first = first->next;
    }
  else
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->balloondist = 0;
      first = first->next;
    }
}


void init_warp_rad(struct voxel_s * first)
{
  while (NULL != first) {
    draw_data_t * data = (draw_data_t*) first->draw_data;
    data->xwarp = (first->x - center_x);
    data->ywarp = (first->y - center_y);
    data->zwarp = (first->z - center_z);
    data->warpdist = sqrt(pow(data->xwarp, 2) + pow(data->ywarp, 2) +
			  pow(data->zwarp, 2));
    if (fabs(data->warpdist) > 1e-3) {	/* hm, magic number... */
      data->xwarp /= data->warpdist;
      data->ywarp /= data->warpdist;
      data->zwarp /= data->warpdist;
    }
    data->warpdist /= view_rad;
    first = first->next;
  }
}


void init_warp_x(struct voxel_s * first)
{
  if (dim_x > 1)
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->xwarp = 1;
      data->ywarp = 0;
      data->zwarp = 0;
      data->warpdist = first->x / (dim_x - 1);
      first = first->next;
    }
  else
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->xwarp = 1;
      data->ywarp = 0;
      data->zwarp = 0;
      data->warpdist = 0;
      first = first->next;
    }
}


void init_warp_y(struct voxel_s * first)
{
  if (dim_y > 1)
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->xwarp = 0;
      data->ywarp = 1;
      data->zwarp = 0;
      data->warpdist = first->y / (dim_y - 1);
      first = first->next;
    }
  else
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->xwarp = 0;
      data->ywarp = 1;
      data->zwarp = 0;
      data->warpdist = 0;
      first = first->next;
    }
}


void init_warp_z(struct voxel_s * first)
{
  if (dim_z > 1)
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->xwarp = 0;
      data->ywarp = 0;
      data->zwarp = 1;
      data->warpdist = first->z / (dim_z - 1);
      first = first->next;
    }
  else
    while (NULL != first) {
      draw_data_t * data = (draw_data_t*) first->draw_data;
      data->xwarp = 0;
      data->ywarp = 0;
      data->zwarp = 1;
      data->warpdist = 0;
      first = first->next;
    }
}


int parse_wibble(wibble_t * ww, init_t * ii, char * argument)
{
  char * s_wibble = argument;
  char * s_direction;
  char * s_rest;
  int result;
  
  for (s_direction = s_wibble; (*s_direction != ':') && (*s_direction != '\0');
       s_direction++) /* just ffwd */;
  if (*s_direction == '\0') {
    fprintf(stderr, "parse_wibble(): error parsing direction part from '%s'\n",
	    argument);
    return -1;
  }
  *s_direction = '\0';	/* terminate s_wibble */
  s_direction++;
  
  if (0 == strcmp(s_wibble, "spike"))
    ww->compute = wibble_spike;
  else if (0 == strcmp(s_wibble, "bump"))
    ww->compute = wibble_bump;
  else {
    fprintf(stderr, "parse_wibble(): invalid wibble '%s'"
	    " (must be 'spike' or 'bump')\n", s_wibble);
    return -1;
  }
  
  for (s_rest = s_direction; (*s_rest != ':') && (*s_rest != '\0');
       s_rest++) /* just ffwd */;
  if (*s_rest == '\0') {
    fprintf(stderr,
	    "parse_wibble(): error parsing beyond direction argument\n");
    return -1;
  }
  *s_rest = '\0';		/* terminate s_direction */
  s_rest++;
  
  if (0 == strcmp(s_direction, "rad"))
    ww->init = ii->init_rad;
  else if (0 == strcmp(s_direction, "x"))
    ww->init = ii->init_x;
  else if (0 == strcmp(s_direction, "y"))
    ww->init = ii->init_y;
  else if (0 == strcmp(s_direction, "z"))
    ww->init = ii->init_z;
  else {
    fprintf(stderr, "parse_wibble(): invalid direction '%s'"
	    " (must be 'rad', 'x', 'y', or 'z')\n", s_direction);
    return -1;
  }
  
  result = sscanf(s_rest, "%la:%la:%la:%la:%la:%la",
		  &ww->duration, &ww->period, &ww->speed,
		  &ww->minval, &ww->maxgrow, &ww->power);
  if (6 != result) {
    fprintf(stderr,
	    "parse_wibble(): error parsing numbers (sscanf returned %d)\n",
	    result);
    return -1;
  }
  
  if (verbose)
    printf("DEBUG -b argument parsing:\n"
	   " wibble:    '%s'\n"
	   " direction: '%s'\n"
	   " rest:      '%s'\n"
	   " duration:   %f\n"
	   " period:     %f\n"
	   " speed:      %f\n"
	   " minval:     %f\n"
	   " maxgrow:    %f\n"
	   " power:      %f\n",
	   s_wibble, s_direction, s_rest,
	   ww->duration, ww->period, ww->speed,
	   ww->minval, ww->maxgrow, ww->power);
  
  return 0;
}
