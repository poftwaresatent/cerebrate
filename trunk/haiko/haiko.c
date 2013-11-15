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
#include "wrap_gl.h"
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


typedef struct option_s {
  unsigned int timer_delay;
  int verbose;
  int ortho_projection;
  int show_bounding_sphere;
  int axes_only;
  int reflection_effect;
  double view_reldist;
  char const * filename;
} option_t;


typedef struct enable_s {
  int spin;
  int balloon;
  int warp;
  int anim;
} enable_t;


typedef struct view_s {
  double radius;
  double dist;
  double lrbt;			/* left, right, bottom, top */
  double near;
  double far;
  double center[3];		/* x, y, z */
  double eye[3];		/* x, y, z */
} view_t;


static void init_static();
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
static void init_view();

static void cube_drawer(struct voxel_s * vv);
static void sphere_drawer(struct voxel_s * vv);

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


static void (*init_drawers)(struct voxel_s * first) = init_drawers_cube;

static int    winwidth = 400;
static int    winheight = 400;
static int    left_down = 0;
static double theta = 0;
static int    dim_x = 0;
static int    dim_y = 0;
static int    dim_z = 0;

static option_t option;
static enable_t enable;
static view_t   view;
static init_t   init_balloon;
static init_t   init_warp;
static wibble_t balloon;
static wibble_t warp;

static trackball_state * trackball;
static voxel_t * voxel;


int main(int argc, char ** argv)
{
  init_static();
  
  if (0 != atexit(cleanup)) {
    perror("atexit()");
    exit(EXIT_FAILURE);
  }
  
  parse_options(argc, argv);
  trackball = gltrackball_init();
  
  if (option.axes_only)
    just_axes();
  else {
    voxel_parse_tab_t voxel_parse_tab;
    FILE * configfile;
    if (0 == strcmp(option.filename, "--"))
      configfile = stdin;
    else {
      configfile = fopen(option.filename, "r");
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
	      argv[0], option.filename);
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
  init_drawers(voxel);
  init_glut(&argc, argv, winwidth, winheight);
  
  glClearColor(0.0, 0.0, 0.2, 0.0);
  
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  
  if (option.reflection_effect) {
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
  glutTimerFunc(option.timer_delay, timer, handle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
}


void init_view()
{
  view.radius = 0.5 * sqrt(pow(dim_x, 2) + pow(dim_y, 2) + pow(dim_z, 2));
  if (1.1 > option.view_reldist)
    option.view_reldist = 1.1;
  view.dist = option.view_reldist * view.radius;
  view.far = view.dist + 2 * view.radius;
  view.near = view.dist - view.radius;
  view.lrbt = 
    (view.radius * view.dist - pow(view.radius, 2))
    / sqrt(pow(view.dist, 2) - pow(view.radius, 2));
  view.center[0] = 0.5 * (dim_x - 1);
  view.center[1] = 0.5 * (dim_y - 1);
  view.center[2] = 0.5 * (dim_z - 1);
  /* do not ask me why it is TWICE the center... */
  view.eye[0] =
    2 * view.center[0] + 0.5 * option.view_reldist * dim_x / view.radius;
  view.eye[1] =
    2 * view.center[1] + 0.5 * option.view_reldist * dim_y / view.radius;
  view.eye[2] =
    2 * view.center[2] + 0.5 * option.view_reldist * dim_z / view.radius;
  if (option.verbose)
    printf("init_view():\n"
	   " view.radius:         %g\n"
	   " option.view_reldist: %g\n"
	   " view.dist:           %g\n"
	   " view.lrbt:           %g\n"
	   " view.far:            %g\n"
	   " view.near:           %g\n"
	   " view.center[0]:      %g\n"
	   " view.center[1]:      %g\n"
	   " view.center[2]:      %g\n"
	   " view.eye[0]:         %g\n"
	   " view.eye[1]:         %g\n"
	   " view.eye[2]:         %g\n",
	   view.radius, option.view_reldist, view.dist, view.lrbt, view.far,
	   view.near, view.center[0], view.center[1], view.center[2],
	   view.eye[0], view.eye[1], view.eye[2]);
}


void reshape(int width, int height)
{
  if (width > height)
    glViewport((width - height) / 2, 0, height, height);
  else
    glViewport(0, (height - width) / 2, width, width);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  
  
  if (option.ortho_projection)
    glOrtho(- view.radius, + view.radius,
	    - view.radius, + view.radius,
	    - view.radius, + view.radius);
  else
    glFrustum(- view.lrbt, + view.lrbt,
	      - view.lrbt, + view.lrbt,
	      view.near, view.far);
  
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
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.1 / view.radius);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.01 / view.radius);
  
  if (0 == option.ortho_projection)
    gluLookAt(view.eye[0], view.eye[1], view.eye[2],
	      0, 0, 0,
	      0, 1, 0);
  
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  
  gltrackball_rotate(trackball);
  glRotatef(180 + theta * 100, 0.0, 1.0, 0.0);
  glRotatef(180, 1.0, 0.0, 0.0);
  glTranslatef( - view.center[0], - view.center[1], - view.center[2]);  
  
  voxel_draw_list(voxel);
  
  if (option.show_bounding_sphere) {
    glTranslatef(view.center[0], view.center[1], view.center[2]);
    glColor3f(1.0, 0.5, 1.0);
    glutWireSphere(view.radius, 12, 12);
  }
  
  glFlush();
  
#ifndef OSX
  glutSwapBuffers();
#endif OSX
  
  if (enable.anim) {
    static int animcount = 0;
    char png_filename[64];
    if (64 <= snprintf(png_filename, 64, "anim/%05d.png", animcount))
      fprintf(stderr, "buffer too small for filename 'anim/%05d.png'\n",
	      animcount);
    else {
      if (0 != wrap_gl_write_png(png_filename, winwidth, winheight))
	fprintf(stderr, "error writing PNG file '%s'\n", png_filename);
      else {
	animcount++;
	if (option.verbose)
	  printf("wrote PNG file '%s'\n", png_filename);
      }
    }
  }
}


void keyboard(unsigned char key, int x, int y)
{
  static int scrcount = 0;
  switch (key) {
  case 'q':
    exit(EXIT_SUCCESS);
    break;
  case ' ':
    enable.spin = enable.spin ? 0 : 1;
    if (option.verbose)
      printf("spin %s\n", enable.spin ? "enabled" : "disabled");
    break;
  case 'b':
    enable.balloon = enable.balloon ? 0 : 1;
    if (option.verbose)
      printf("balloon %s\n", enable.balloon ? "enabled" : "disabled");
    break;
  case 'w':
    enable.warp = enable.warp ? 0 : 1;
    if (option.verbose)
      printf("warp %s\n", enable.warp ? "enabled" : "disabled");
    break;
  case 'a':
    enable.anim = enable.anim ? 0 : 1;
    if (option.verbose)
      printf("anim %s\n", enable.anim ? "enabled" : "disabled");
    break;
  case 'p':
    {
      char png_filename[64];
      if (64 <= snprintf(png_filename, 64, "scr/%05d.png", scrcount))
	fprintf(stderr, "buffer too small for filename 'scr/%05d.png'\n",
		scrcount);
      else {
	if (0 != wrap_gl_write_png(png_filename, winwidth, winheight))
	  fprintf(stderr, "error writing PNG file '%s'\n", png_filename);
	else {
	  scrcount++;
	  if (option.verbose)
	    printf("wrote PNG file '%s'\n", png_filename);
	}
      }
    }
    break;
  }
}


void timer(int handle)
{
  if (0 == left_down) {
    if (enable.spin)
      theta += 0.01;
    if (enable.balloon)
      update_balloon(voxel);
    if (enable.warp)
      update_warp(voxel);
  }
  
  glutSetWindow(handle);
  glutPostRedisplay();
  glutTimerFunc(option.timer_delay, timer, handle);
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
    if (option.verbose)
      printf("freeing trackball\n");
    free(trackball);
  }
  if (NULL != voxel) {
    if (option.verbose)
      printf("freeing voxels\n");
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
    "default: -f haiko.conf -d 2 -t 100 -D cube\n"
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
	option.verbose = 1;
	break;
	
      case 'o':
	option.ortho_projection = 1;
	break;
	
      case 's':
	option.show_bounding_sphere = 1;
	break;
	
      case 'a':
	option.axes_only = 1;
	break;
	
      case 'r':
	option.reflection_effect = 1;
	break;
	
      case 'f':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -f requires a filename argument\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	option.filename = argv[ii];
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
	  option.view_reldist = strtod(argv[ii], &endptr);
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
	  option.timer_delay = (unsigned int) delay;
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
    data->balloondist = sqrt(pow(first->x - view.center[0], 2) +
			     pow(first->y - view.center[1], 2) +
			     pow(first->z - view.center[2], 2)) / view.radius;
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
    data->xwarp = (first->x - view.center[0]);
    data->ywarp = (first->y - view.center[1]);
    data->zwarp = (first->z - view.center[2]);
    data->warpdist = sqrt(pow(data->xwarp, 2) + pow(data->ywarp, 2) +
			  pow(data->zwarp, 2));
    if (fabs(data->warpdist) > 1e-3) {	/* hm, magic number... */
      data->xwarp /= data->warpdist;
      data->ywarp /= data->warpdist;
      data->zwarp /= data->warpdist;
    }
    data->warpdist /= view.radius;
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
  
  if (option.verbose)
    printf("DEBUG wibble argument parsing:\n"
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


void init_static()
{
  option.timer_delay = 100;
  option.verbose = 0;
  option.ortho_projection = 0;
  option.show_bounding_sphere = 0;
  option.axes_only = 0;
  option.reflection_effect = 0;
  option.view_reldist = 2;
  option.filename = "haiko.conf";
  
  enable.spin = 1;
  enable.balloon = 1;
  enable.warp = 1;
  enable.anim = 0;
  
  view.radius = -1;
  
  init_balloon.init_rad = init_balloon_rad;
  init_balloon.init_x = init_balloon_x;
  init_balloon.init_y = init_balloon_y;
  init_balloon.init_z = init_balloon_z;
  
  init_warp.init_rad = init_warp_rad;
  init_warp.init_x = init_warp_x;
  init_warp.init_y = init_warp_y;
  init_warp.init_z = init_warp_z;
  
  balloon.initialized = 0;
  warp.initialized = 0;
}
