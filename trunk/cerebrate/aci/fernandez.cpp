/* 
 * Copyright (C) 2005 Roland Philippsen <roland dot philippsen at gmx dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */


#include "Cactus.hpp"
#include "GUIHandler.hpp"
#include <gfx/Viewport.hpp>
#include <gfx/wrap_glut.hpp>
#include <drivers/FModIPDCMOT.hpp>
#include <drivers/FModTCP.hpp>
#include <drivers/util.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <pthread.h>


using namespace std;


void * run_glthread(void * nothing_at_all);
void * run_cmdthread(void * nothing_at_all);
void parse_options(int argc, char ** argv);
void init_glut(int * argc, char ** argv,
	       int width, int height);
void reshape(int width, int height);
void draw();
void keyboard(unsigned char key, int x, int y);
void timer(int handle);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void cleanup(void);

GUIHandler * get_gui_handler();


typedef Subwindow::logical_bbox_t bb_t;

static const unsigned int timer_delay(200);
static const double zoom(1);

enum { ALL = 0, ZOOM, STATUS, LIGHTSHOW, GUI, N_VIEWPORTS };

bool step(false), continuous(true);
Viewport * viewport[N_VIEWPORTS];
auto_ptr<Cactus> fernandez;
pthread_t glthread(0);
pthread_t cmdthread(0);
bool please_exit(false);


int main(int argc,
	 char ** argv)
{
  parse_options(argc, argv);
  set_cleanup(cleanup);
  
  viewport[ALL] = new Viewport("all",
			       bb_t(0, -8, 8, 8),
			       bb_t(0, 0, 0.5, 1),
			       true,
			       0);
  viewport[ZOOM] = new Viewport("zoom",
				bb_t(-zoom, -zoom, zoom, zoom),
				bb_t(0.5, 0.3, 1, 1),
				true,
				0.01);
  viewport[STATUS] = new Viewport("status",
				  bb_t(0, 0, 1, 1),
				  bb_t(0.5, 0.1, 1, 0.2),
				  false,
				  0);
  viewport[LIGHTSHOW] = new Viewport("lightshow",
				     bb_t(0, 0, 1, 1),
				     bb_t(0.5, 0.2, 1, 0.3),
				     true,
				     0);
  viewport[GUI] = new Viewport("gui",
			       bb_t(0, 0, 1, 1),
			       bb_t(0.5, 0.0, 1, 0.1),
			       false,
			       0);
  
  fernandez = auto_ptr<Cactus>(new Cactus());
  
  for(int i(0); i < N_VIEWPORTS; ++i)
    viewport[i]->Enable();
  viewport[ALL]->SetMousehandler(Viewport::LEFT, fernandez->GetMouseGoal());
  viewport[ALL]->SetMousehandler(Viewport::RIGHT, fernandez->GetMouseTarget());
  viewport[ALL]->SetMousehandler(Viewport::MIDDLE, fernandez->GetMouseAuto());
  viewport[ZOOM]->SetMousehandler(Viewport::LEFT, fernandez->GetMouseGoal());
  viewport[ZOOM]->SetMousehandler(Viewport::RIGHT,
				  fernandez->GetMouseTarget());
  viewport[ZOOM]->SetMousehandler(Viewport::MIDDLE, fernandez->GetMouseAuto());
  
  viewport[GUI]->SetMousehandler(Viewport::LEFT, get_gui_handler());

  fernandez->ConfigureStatusViewport( * viewport[STATUS]);
  fernandez->ConfigureEffectsViewport( * viewport[LIGHTSHOW]);
  fernandez->ConfigureArenaViewport( * viewport[ALL]);
  get_gui_handler()->ConfigureViewport( * viewport[GUI]);

  fernandez->AutoLocalize( & cout);
  fernandez->SetEnableAutoLocalize(true);
  fernandez->SetState(Cactus::AUTO);
  
  if(0 != pthread_create( & cmdthread, 0, run_cmdthread, 0)){
    cmdthread = 0;
    perror("ERROR creating cmdthread");
    exit(EXIT_FAILURE);
  }

  if(0 != pthread_create( & glthread, 0, run_glthread, 0)){
    glthread = 0;
    perror("ERROR creating glthread");
    exit(EXIT_FAILURE);
  }
  
  while( ! please_exit)
    usleep(100000);

  cout << "\n"
       << "**************************************************\n"
       << "**************************************************\n"
       << "***                                            ***\n"
       << "***    a cactus a day keeps the artist away    ***\n"
       << "***                                            ***\n"
       << "**************************************************\n"
       << "**************************************************\n";
  
  exit(EXIT_SUCCESS);
}


void * run_cmdthread(void *)
{
  do{
    cout << "fernandez> ";
  }while(fernandez->Command(cin, cout, & cout));

  please_exit = true;
  while(true)
    usleep(1000000);
  
  return 0;
}


void init_glut(int * argc, char ** argv,
	       int width, int height)
{
  glutInit(argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowPosition(0, 0);
  glutInitWindowSize(width, height);
  
  int handle(glutCreateWindow("<<<<<<cactus lab>>>>>>"));
  if(0 == handle){
    cerr << argv[0] << ": init_glut(): couldn't create parent window\n";
    exit(EXIT_FAILURE);
  }
  
  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutTimerFunc(timer_delay, timer, handle);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
}


void * run_glthread(void * nothing_at_all)
{
  static char * fake_argv[] = {"FERNANDEZ"};
  static int fake_argc = 1;
  init_glut(& fake_argc, fake_argv, 1000, 700);
  glutMainLoop();
  return 0;
}


void cleanup()
{
  if(0 != fernandez.get())
    fernandez->SetQd(0, 0);
  
  if(0 != glthread)
    pthread_cancel(glthread);
  
  if(0 != cmdthread)
    pthread_cancel(cmdthread);
  
  if(0 != glthread){
    cerr << "DBG: joining glthread...";
    if(0 != pthread_join(glthread, 0))
      perror("WARNING joining glthread");
    else
      cerr << "OK\n";
  }

  if(0 != cmdthread){
    cerr << "DBG: joining cmdthread...";
    if(0 != pthread_join(cmdthread, 0))
      perror("WARNING joining cmdthread");
    else
      cerr << "OK\n";
  }
  
  for(int i(0); i < N_VIEWPORTS; ++i)
    delete viewport[i];
}


void reshape(int width, int height)
{
  Subwindow::DispatchResize(Subwindow::screen_point_t(width, height));
}


void draw()
{
  glClear(GL_COLOR_BUFFER_BIT);
  
  viewport[ALL]->PushProjection();
  fernandez->Draw();
  viewport[ALL]->PopProjection();
  
  viewport[ZOOM]->PushProjection();
  fernandez->Draw();
  viewport[ZOOM]->PopProjection();

  viewport[STATUS]->PushProjection();
  fernandez->DrawStatus();
  viewport[STATUS]->PopProjection();

  viewport[LIGHTSHOW]->PushProjection();
  fernandez->DrawEffects();
  viewport[LIGHTSHOW]->PopProjection();

  viewport[GUI]->PushProjection();
  get_gui_handler()->Draw();
  viewport[GUI]->PopProjection();
  
  glFlush();
  glutSwapBuffers();
}


void keyboard(unsigned char key, int x, int y)
{
  return;

  switch(key){
  case ' ':
    step = true;
    continuous = false;
    break;
  case 'c':
    step = false;
    continuous = true;
    break;
  case 'q':
    please_exit = true;
    break;
  }
}


void timer(int handle)
{
  if(step || continuous){
    if(step)
      step = false;
    fernandez->Update();
    double x, y, theta;
    fernandez->GetPose(x, y, theta);
    const bb_t nbb(x - zoom, y - zoom, x + zoom, y + zoom);
    viewport[ZOOM]->Remap(nbb);
  }
  
  Subwindow::DispatchUpdate();
  
  glutSetWindow(handle);
  glutPostRedisplay();
  
  if( ! please_exit)
    glutTimerFunc(timer_delay, timer, handle);
}


void mouse(int button, int state, int x, int y)
{
  Subwindow::DispatchClick(button, state,
			   Subwindow::screen_point_t(x, y));
}


void motion(int x, int y)
{
  Subwindow::DispatchDrag(Subwindow::screen_point_t(x, y));
}


void parse_options(int argc, char ** argv)
{
}


class quit_cb: public GUICallback {
public:
  void Do() {
    cout << "HELLO from quit_cb!\n";
    please_exit = true;
  }
};


class reloc_cb: public GUICallback {
public:
  void Do() {
    fernandez->AutoLocalize(0);
  }
};


GUIHandler * get_gui_handler()
{
  static auto_ptr<GUIHandler> mh;
  static auto_ptr<quit_cb> qcb;
  static auto_ptr<reloc_cb> rcb;
  if(mh.get() == 0){
    mh = auto_ptr<GUIHandler>(new GUIHandler());
    qcb = auto_ptr<quit_cb>(new quit_cb());
    rcb = auto_ptr<reloc_cb>(new reloc_cb());
    mh->AddButton(0, 0, 1, 1, 1, 0, 0, qcb.get());
    mh->AddButton(1.1, 0, 1.6, 1, 0, 1, 0, rcb.get());
  }
  return mh.get();
}
