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


#include "gltrackball.h"
#include "wrap_gl.h"
#include "wrap_glut.h"
#include "voxel.h"
#include "effect.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


typedef struct option_s {
  unsigned int timer_delay;
  int verbose;
  int ortho_projection;
  int show_bounding_sphere;
  int show_effect_distref;
  int show_axes;
  int show_grid;
  int axes_only;
  int reflection_effect;
  double view_reldist;
  char const * voxel_filename;
  char const * effect_filename;
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

static void init_drawers_cube(struct voxel_s * first);
static void init_drawers_sphere(struct voxel_s * first);

static void update_drawers_wiggle(struct voxel_s * first);


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
static effect_t balloon;
static effect_t warp;

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
    if (0 == strcmp(option.voxel_filename, "--"))
      configfile = stdin;
    else {
      configfile = fopen(option.voxel_filename, "r");
      if (NULL == configfile) {
	fprintf(stderr, "error opening voxel file %s: ",
		option.voxel_filename);
	perror("fopen()");
	exit(EXIT_FAILURE);
      }
    }
    
    voxel_parse_tab.debug = option.verbose;
    voxel_parse_file(configfile, &voxel_parse_tab);
    if (stdin != configfile)
      fclose(configfile);
    
    voxel = voxel_parse_tab.first;
    if (voxel_parse_tab.error) {
      fprintf(stderr, "%s: error parsing voxels from '%s'\n",
	      argv[0], option.voxel_filename);
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
  
  if (strlen(option.effect_filename) > 0) {
    FILE * configfile;
    int result;
    
    if (0 == strcmp(option.effect_filename, "--"))
      configfile = stdin;
    else {
      configfile = fopen(option.effect_filename, "r");
      if (NULL == configfile) {
	fprintf(stderr, "error opening effect file %s: ",
		option.effect_filename);
	perror("fopen()");
	exit(EXIT_FAILURE);
      }
    }
    
    result = effect_parse_file(configfile, &balloon, &warp, option.verbose);
    if (stdin != configfile)
      fclose(configfile);
    
    if (0 != result) {
      fprintf(stderr, "error parsing effect file: %d", result);
      exit(EXIT_FAILURE);
    }
  }
  
  init_view();
  init_drawers(voxel);
  balloon.init(&balloon, voxel);
  balloon.update(&balloon, voxel);
  warp.init(&warp, voxel);    
  warp.update(&warp, voxel);
  
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
  
  handle = glutCreateWindow(option.voxel_filename);
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
  
  if (option.show_effect_distref) {
    if (NULL != balloon.amplitude) {
      glColor3d(1, 0.5, 0.5);
      glPushMatrix();
      glTranslatef(balloon.distance.point[0],
		   balloon.distance.point[1],
		   balloon.distance.point[2]);
      glutSolidSphere(0.25, 12, 12);
      glutWireSphere(2.5, 12, 12);
      
      switch (balloon.distance.type) {
      case EFFECT_DISTREF_LINE:
      case EFFECT_DISTREF_PLANE:
	glColor3d(0.5, 1, 0.5);
	glTranslatef(balloon.distance.unit[0],
		     balloon.distance.unit[1],
		     balloon.distance.unit[2]);
	glutSolidSphere(0.25, 12, 12);
	glutWireSphere(2.5, 12, 12);
/* 	break; */
      }
      glPopMatrix();
    }
  }
  
  if (option.show_grid) {
    int ii;
    glBegin(GL_LINES);
    glColor3d(0.5, 0.5, 0.5);
    
    for (ii = 0; ii < dim_x; ii++) {
      glVertex3d(ii, 0,         0);
      glVertex3d(ii, 0,         dim_z - 1);
      glVertex3d(ii, dim_y - 1, 0);
      glVertex3d(ii, dim_y - 1, dim_z - 1);
      glVertex3d(ii, 0,         0);
      glVertex3d(ii, dim_y - 1, 0);
      glVertex3d(ii, 0,         dim_z - 1);
      glVertex3d(ii, dim_y - 1, dim_z - 1);
    }
    
    for (ii = 0; ii < dim_y; ii++) {
      glVertex3d(0,         ii, 0);
      glVertex3d(0,         ii, dim_z - 1);
      glVertex3d(dim_x - 1, ii, 0);
      glVertex3d(dim_x - 1, ii, dim_z - 1);
      glVertex3d(0,         ii, 0);
      glVertex3d(dim_x - 1, ii, 0);
      glVertex3d(0,         ii, dim_z - 1);
      glVertex3d(dim_x - 1, ii, dim_z - 1);
    }
    
    for (ii = 0; ii < dim_z; ii++) {
      glVertex3d(0,         0,         ii);
      glVertex3d(0,         dim_y - 1, ii);    
      glVertex3d(dim_x - 1, 0,         ii);
      glVertex3d(dim_x - 1, dim_y - 1, ii);    
      glVertex3d(0,         0,         ii);
      glVertex3d(dim_x - 1, 0,         ii);    
      glVertex3d(0,         dim_y - 1, ii);
      glVertex3d(dim_x - 1, dim_y - 1, ii);
    }
    
    glEnd();
  }
  
  if (option.show_axes) {
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3d(1, 0, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(dim_x, 0, 0);
    glColor3d(0, 1, 0);
    glVertex3d(0, 0, 0);
    glVertex3d(0, dim_y, 0);    
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, dim_z);    
    glEnd();
    glLineWidth(1);
  }
  
  glFlush();
  
#ifndef OSX
  glutSwapBuffers();
#endif OSX
  
  if (enable.anim) {
#ifndef HAVE_PNG_H
    fprintf(stderr, "sorry, no animation: PNG support not built in\n");
#else
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
#endif
  }
}


void keyboard(unsigned char key, int x, int y)
{
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
#ifndef HAVE_PNG_H
    fprintf(stderr, "sorry, no screenshot: PNG support not built in\n");
#else
    {
      static int scrcount = 0;
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
#endif
    break;
  }
}


void timer(int handle)
{
  if (0 == left_down) {
    if (enable.spin)
      theta += 0.01;
    if (enable.balloon)
      balloon.update(&balloon, voxel);
    if (enable.warp)
      warp.update(&warp, voxel);
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
    "  -c           show coordinate axes\n"
    "  -g           show coordinate grid\n"
    "  -E           show distance reference of effect\n"
    "  -a           no config file, just axes voxels\n"
    "  -r           enable reflection effect\n"
    "  -f  <file>   voxel file name ('--' means stdin)\n"
    "  -e  <file>   effect file name ('--' means stdin)\n"
    "  -d  <dist>   set relative viewing distance\n"
    "  -t  <timer>  set timer interval [ms]\n"
    "  -D  <style>  drawing style: cube or sphere\n"
    "\n"
    "default: -f haiko.vox -d 2 -t 100 -D cube\n";
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
	
      case 'c':
	option.show_axes = 1;
	break;
	
      case 'g':
	option.show_grid = 1;
	break;
	
      case 'E':
	option.show_effect_distref = 1;
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
	option.voxel_filename = argv[ii];
	break;
	
      case 'e':
	++ii;
	if (ii >= argc) {
	  usage(stderr);
	  fprintf(stderr, "\n%s: -e requires a filename argument\n", argv[0]);
	  exit(EXIT_FAILURE);
	}
	option.effect_filename = argv[ii];
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


void init_drawers_cube(struct voxel_s * first)
{
  while (NULL != first) {
    first->draw = voxel_draw_cube;
    first = first->next;
  }
}


void init_drawers_sphere(struct voxel_s * first)
{
  while (NULL != first) {
    first->draw =voxel_draw_sphere;
    first = first->next;
  }
}


void update_drawers_wiggle(struct voxel_s * first)
{
  while (NULL != first) {
    int ii;
    for (ii = 0; ii < 3; ii++)
      first->off[ii] = (0.1 * random()) / LONG_MAX;
    first = first->next;
  }
}


void init_static()
{
  option.timer_delay = 100;
  option.verbose = 0;
  option.ortho_projection = 0;
  option.show_bounding_sphere = 0;
  option.show_effect_distref = 0;
  option.show_axes = 0;
  option.show_grid = 0;
  option.axes_only = 0;
  option.reflection_effect = 0;
  option.view_reldist = 2;
  option.voxel_filename = "haiko.vox";
  option.effect_filename = "";
  
  enable.spin = 1;
  enable.balloon = 1;
  enable.warp = 1;
  enable.anim = 0;
  
  view.radius = -1;
  
  effect_configure_balloon(&balloon);
  effect_configure_warp(&warp);
}
