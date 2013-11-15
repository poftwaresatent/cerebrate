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


#define TIMER_DELAY 100


static void init_glut(int * argc, char ** argv, int width, int height);
static void reshape(int width, int height);
static void draw();
static void keyboard(unsigned char key, int x, int y);
static void timer(int handle);
static void mouse(int button, int state, int x, int y);
static void motion(int x, int y);
static void cleanup();

static void setup_lights();


static int winwidth, winheight, clearbits;
static trackball_state * trackball;
static int left_down;
static double theta;
static voxel_t * voxel;
static int dim_x;
static int dim_y;
static int dim_z;
static double view_rad;
static double view_rel_dist;
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

int main(int argc, char ** argv)
{
  if (0 != atexit(cleanup)) {
    perror("atexit()");
    exit(EXIT_FAILURE);
  }
  
  winwidth = 400;
  winheight = 400;
  trackball = gltrackball_init();
  
  {
    voxel_t * last;
    voxel = voxel_create(0, 0, 0, 1.0, 1.0, 1.0);
    if (NULL == voxel) {
      fprintf(stderr, "out of memory?\n");
      exit(EXIT_FAILURE);
    }
    last = voxel;
    
    last->next = voxel_create(1, 0, 0, 1.0, 0.0, 0.0);
    if (NULL == last->next) {
      fprintf(stderr, "out of memory?\n");
      exit(EXIT_FAILURE);
    }
    last = last->next;
    
    last->next = voxel_create(0, 1, 0, 0.0, 1.0, 0.0);
    if (NULL == last->next) {
      fprintf(stderr, "out of memory?\n");
      exit(EXIT_FAILURE);
    }
    last = last->next;
    
    last->next = voxel_create(0, 0, 1, 0.0, 0.0, 1.0);
    if (NULL == last->next) {
      fprintf(stderr, "out of memory?\n");
      exit(EXIT_FAILURE);
    }
    last = last->next;    	/* not needed, for future extension */
    
    dim_x = 2;
    dim_y = 2;
    dim_z = 2;
  }
  view_rad = -1;  /* -1 means "compute me!" (for several variables) */
  view_rel_dist = 2;
  
  init_glut(&argc, argv, winwidth, winheight);
  
  glClearColor(0.0, 0.0, 0.0, 0.0);
  clearbits = GL_COLOR_BUFFER_BIT;
  
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  setup_lights();
  
  glEnable(GL_DEPTH_TEST);
  clearbits |= GL_DEPTH_BUFFER_BIT;
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
  glutTimerFunc(TIMER_DELAY, timer, handle);
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
    printf("view_rad: %g\n"
	   "view_rel_dist: %g\n"
	   "view_dist: %g\n"
	   "view_lrbt: %g\n"
	   "view_far: %g\n"
	   "view_near: %g\n"
	   "center_x: %g\n"
	   "center_y: %g\n"
	   "center_z: %g\n"
	   "eye_x: %g\n"
	   "eye_y: %g\n"
	   "eye_z: %g\n",
	   view_rad, view_rel_dist, view_dist, view_lrbt, view_far, view_near,
	   center_x, center_y, center_z, eye_x, eye_y, eye_z);
  }
  
  if (width > height)
    glViewport((width - height) / 2, 0, height, height);
  else
    glViewport(0, (height - width) / 2, width, width);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();  
  
#undef JUST_ORTHO
#ifdef JUST_ORTHO
  glOrtho( - view_rad,  + view_rad,
	   - view_rad,  + view_rad,
	   - view_rad,  + view_rad);
#else
  glFrustum(-view_lrbt, view_lrbt, -view_lrbt, view_lrbt, view_near, view_far);
#endif
  
  winwidth = width;
  winheight = height;
}


void draw()
{
  GLfloat amb[] = { 0.8, 0.8, 0.8, 1.0 };
  
  glClear(clearbits);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.2);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.15);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.15);
  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);

  /* JUST_ORTHO defined (or not) further above */
#ifndef JUST_ORTHO
  gluLookAt(eye_x, eye_y, eye_z,
	    0, 0, 0,
	    0, 1, 0);
#endif

#define ENABLE_TRACKBALL
#ifdef ENABLE_TRACKBALL
  gltrackball_rotate(trackball);
#endif
  
#define ENABLE_THETA
#ifdef ENABLE_THETA
  glRotatef(theta * 100, 0.0, 1.0, 0.0);
#endif
  
  glTranslatef( - center_x, - center_y, - center_z);  
  
#undef TESTING
#ifdef TESTING
  glTranslatef(  center_x,    center_y,   center_z);
  glScalef(0.2, 0.2, 0.2);
  glColor3f(1.0, 0.5, 0.5);
  glutSolidDodecahedron();
#else
  voxel_draw_list(voxel);
  
  glTranslatef(  center_x,    center_y,   center_z);
  glColor3f(1.0, 0.5, 0.5);
  glutSolidSphere(0.5 * view_rad, 12, 12);
  glColor3f(1.0, 0.5, 1.0);
  glutWireSphere(view_rad, 12, 12);

#endif
  
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
  glutTimerFunc(TIMER_DELAY, timer, handle);
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


void setup_lights()
{
  GLfloat position[4];
  position[0] = 1;
  position[1] = 1;
  position[2] = 0;
  position[3] = 1;
  
  glEnable(GL_LIGHTING);
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  glEnable(GL_LIGHT0);
}
