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


#include "Scanalyzer.hpp"
#include "CircleLSQ.hpp"
#include "gfx/wrap_gl.hpp"
#include "gfx/Viewport.hpp"
#include <drivers/util.h>
#include <drivers/sick.h>
#include <sfl/numeric.hpp>
#include <sfl/Polygon.hpp>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <stdint.h>


using namespace std;
using namespace sfl;


Scanalysis::
Scanalysis():
  t0(Timestamp::Now()),
  t1(t0)
{
  memset(category, INVALID, scansize * sizeof(category_t));
}


Scanalysis::
Scanalysis(const Scanalysis & original)
{
  * this = original;
}


Scanalysis::
~Scanalysis()
{
  for(size_t i(0); i < circle.size(); ++i)
    delete circle[i];
}


Scanalysis & Scanalysis::
operator = (const Scanalysis & original)
{
  memcpy(x, original.x, scansize * sizeof(double));
  memcpy(y, original.y, scansize * sizeof(double));
  memcpy(rho, original.rho, scansize * sizeof(double));
  memcpy(bgrho, original.bgrho, scansize * sizeof(double));
  memcpy(category, original.category, scansize * sizeof(category_t));
  memcpy(label, original.label, scansize * sizeof(int));
  startindex = original.startindex;
  endindex = original.endindex;
  
  for(size_t i(0); i < circle.size(); ++i)
    delete circle[i];
  circle.clear();
  for(size_t i(0); i < original.circle.size(); ++i)
    if(original.circle[i] == 0)
      circle.push_back(0);
    else
      circle.push_back(new CircleLSQ(* original.circle[i]));
  
  t0 = original.t0;
  t1 = original.t1;
  
  return * this;
}


bool Scanalyzer::
InitSick(FILE * sick_dbg)
{
  const int fd(serial_open(_comport.c_str(), _baudrate));
  if(fd < 0){
    cerr << "Scanalyzer::InitSick(): serial_open(" << _comport
	 << ", " << _baudrate << ") returned " << fd << "\n";
    return false;
  }
  _sick_poster = sick_poster_new(fd, _usec_cycle, sick_dbg);
  if(0 == _sick_poster){
    cerr << "Scanalyzer::InitSick(): sick_poster_new() failed.\n";
    return false;
  }
  if(0 != sick_poster_start(_sick_poster)){
    cerr << "Scanalyzer::InitSick(): sick_poster_start() failed.\n";
    return false;
  }
  
  // a bit of a hack to keep watchdog from constantly re-starting the sick.
  _analysis.t0 = Timestamp::Now();
  _analysis.t1 = Timestamp::Now();

  return true;
}


void Scanalyzer::
Init(double cluster_thresh, double rhomax)
{
  if( ! InitSick(0)){
    cerr << "Scanalyzer::Init(): InitSick() failed.\n";
    exit(EXIT_FAILURE);
  }
  
  const_cast<double &>(_cluster_thresh) = cluster_thresh;
  const_cast<double &>(_rhomax) = rhomax;
  
  double * cp = const_cast<double *>(_cosphi);
  double * sp = const_cast<double *>(_sinphi);
  for(int i(0); i < scansize; ++i){
    const double p(M_PI / 2 - (M_PI * i / scansize));
    cp[i] = cos(p);
    sp[i] = sin(p);
  }
  memset(_bgx, 0, scansize * sizeof(double));
  memset(_bgy, 0, scansize * sizeof(double));
  memset(_analysis.bgrho, 0, scansize * sizeof(double));
  
  Polygon foo;
  foo.AddPoint(0.1,  3.85);
  foo.AddPoint(3.5,  3.85);
  foo.AddPoint(3.5, -1.0);
  foo.AddPoint(0.1, -1.0);
  _valid_zone = auto_ptr<Polygon>(foo.CreateConvexHull());
}


Scanalyzer::
Scanalyzer(double cluster_thresh, double rhomax,
	   const std::string & comport, unsigned long baudrate,
	   unsigned int usec_cycle):
  _comport(comport),
  _baudrate(baudrate),
  _usec_cycle(usec_cycle)
{
  Init(cluster_thresh, rhomax);
}


Scanalyzer::
Scanalyzer(double cluster_thresh, double rhomax,
	   const std::string & fname,
	   const std::string & comport, unsigned long baudrate,
	   unsigned int usec_cycle):
  _comport(comport),
  _baudrate(baudrate),
  _usec_cycle(usec_cycle)
{
  Init(cluster_thresh, rhomax);
  if( ! LoadBackground(fname)){
    cerr << "ERROR in Scanalyzer ctor: LoadBackground("
	 << fname << ") failed!\n";
    exit(EXIT_FAILURE);
  }
}


Scanalyzer::
~Scanalyzer()
{
  if(0 != _sick_poster){
    if(0 != sick_poster_stop(_sick_poster))
      cerr << "WARNING from Scanalyzer::~Scanalyzer():"
	   << " sick_poster_stop() failed.\n";
    if(0 != serial_close(_sick_poster->fd))
      cerr << "WARNING from Scanalyzer::~Scanalyzer():"
	   << " serial_close() failed.\n";
    sick_poster_delete(_sick_poster);
  }
}


bool Scanalyzer::
Update(std::ostream * dbg)
{
  if(0 == _sick_poster)
    return false;		// restarting (probably)
  
  if(_sick_poster->current_t0 <= _analysis.t0)
    return false;
  
  struct sick_scan_s scan;
  int result = sick_poster_getscan(_sick_poster, & scan);
  if(0 != result){
    if(0 != dbg)
      (* dbg) << "ERROR in  Scanalyzer::Update():\n"
	      << "  sick_poster_getscan returned " << result << "\n";
    return false;
  }
  
  _analysis.t0 = scan.t0;
  _analysis.t1 = scan.t1;
  
  double rprev(_rhomax);
  for(int i(0); i < scansize; ++i){
    double r(scan.rho[i] * 0.001);
    
    // replace single infinity readings by the mean of its two
    // neighbors.
    if((r >= _rhomax)
       && (rprev < _rhomax)
       && (i < 360)){
      const double rnext(scan.rho[i + 1] * 0.001);
      if(rnext < _rhomax){
	r = (rprev + rnext) / 2;
	if(0 != dbg)
	  (* dbg) << "DBG Scanalyzer::Update(): replaced " << i
		  << " by mean of neighbors.\n";
      }
    }
    
    // store the possibly filtered value
    _analysis.x[i] = r * _cosphi[i];
    _analysis.y[i] = r * _sinphi[i];
    _analysis.rho[i] = r;
    
    // classify reading and update background map
    _analysis.category[i] = Scanalysis::BACKGROUND;
    if(r > _analysis.bgrho[i]){
      _bgx[i] = _analysis.x[i];
      _bgy[i] = _analysis.y[i];
      _analysis.bgrho[i] = r;
    }
    else if((_analysis.bgrho[i] - r >= _cluster_thresh)
	    && _valid_zone->Contains(_analysis.x[i], _analysis.y[i]))
      _analysis.category[i] = Scanalysis::OBJECT;
    
    rprev = r;
  }
  
  Cluster(dbg);
  Extract();
  return true;
}


const Scanalysis & Scanalyzer::
GetScanalysis()
  const
{
  return _analysis;
}


void Scanalyzer::
Cluster(ostream * dbg)
{
  if(0 != dbg)
    (*dbg) << "DEBUG Scanalyzer::Cluster():\n";
  
  _analysis.startindex.clear();
  _analysis.endindex.clear();
  int label;
  int oldi;
  double rprev(-1);
  for(int i(0); i < scansize; ++i){    
    // skip BG and INFTY
    if(0 != dbg)
      (*dbg) << "**************************************************\n"
	     << "  i = " << i << ": Skipping background:";
    oldi = i;
    bool skipped_bg(false);
    for(/**/; (i < scansize)
	      && (Scanalysis::BACKGROUND == _analysis.category[i]); ++i){
      if(0 != dbg)
	(*dbg) << " " << i;
      _analysis.label[i] = -1;
      skipped_bg = true;
    }
    if(0 != dbg)
      (*dbg) << "\n";
    if(i >= scansize)
      break;
    
    const double r(_analysis.rho[i]);
    if(rprev < 0){
      if(0 != dbg)
	(*dbg) << "  i = " << i <<  ": First cluster.\n";
      label = 0;
      _analysis.startindex.push_back(i);
    }
    else if(skipped_bg){
      if(0 != dbg)
	(*dbg) << "  i = " << i <<  ": Skipped BG after " << label << ".\n";
      ++label;
      _analysis.startindex.push_back(i);
      _analysis.endindex.push_back(oldi - 1);
    }
    else if(sfl::absval(r - rprev) > _cluster_thresh){
      if(0 != dbg)
	(*dbg) << "  i = " << i << ": Threshold exceeded after "
	       << label << "\n";
      ++label;
      _analysis.startindex.push_back(i);
      _analysis.endindex.push_back(i - 1);
    }
    
    if(0 != dbg)
      (*dbg) << "  i = " << i <<  ": label = " << label << "\n";
    _analysis.label[i] = label;
    
    rprev = r;
  }

  if(_analysis.startindex.size() == 0){
    if(0 != dbg)
      (*dbg) << "  No clusters found (initialization or empty environment).\n";
    return;
  }
  
  _analysis.endindex.push_back(oldi - 1);
  if(_analysis.startindex.size() != _analysis.endindex.size()){
    cerr << "BUG in Scanalyzer::Cluster():\n"
	 << "  _analysis.startindex.size() != _analysis.endindex.size()\n";
    exit(EXIT_FAILURE);
  }
  
  if(0 != dbg){
    (*dbg) << "  ------------------------------------------------\n"
	   << "  label\tstartindex\tendindex\n";
    for(size_t i(0); i < _analysis.startindex.size(); ++i)
      (*dbg) << "  " << i << "\t" << _analysis.startindex[i]
	     << "\t" << _analysis.endindex[i] << "\n";
  }
}


void Scanalysis::
Draw() const
{
  static const double colormap[18][3] = {
    {1,   0,   0},
    {0,   1,   0},
    {0,   0,   1},
    
    {1,   0.5, 0},
    {0,   1,   0.5},
    {0.5, 0,   1},
    
    {1,   0,   0.5},
    {0.5, 1,   0},
    {0,   0.5,   1},
    
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 1},
    
    {1,   0.5, 0.5},
    {0.5, 1,   0.5},
    {0.5, 0.5, 1},
    
    {0.5, 1,    1},
    {1,   0.5,  1},
    {1,   1,    0.5}
  };
  
  // scan and clusters
  glBegin(GL_POINTS);
  for(int i(0); i < scansize; ++i){
    const int label(label[i]);
    if(label < 0){
      glColor3d(0.5, 0.5, 0.5);
      glPointSize(1);
    }
    else{
      const int entry(label % 18);
      glColor3d(colormap[entry][0], colormap[entry][1], colormap[entry][2]);
      if((circle.size() > 0) && (circle[label] != 0))
	glPointSize(3);
      else
	glPointSize(1);
    }
    glVertex2d(x[i], y[i]);
  }
  glEnd();
  
  if(circle.size() <= 0)
    return;
  
  // circles
  for(size_t label(0); label < startindex.size(); ++label)
    if(circle[label] != 0){
      const size_t entry(label % 18);
      glColor3d(colormap[entry][0], colormap[entry][1], colormap[entry][2]);
      circle[label]->Draw(false);
    }
}


void Scanalyzer::
Draw() const
{
  // valid zone
  glColor3d(0.5, 0.5, 0);
  glBegin(GL_LINE_LOOP);
  for(unsigned int i(0); i < _valid_zone->GetNLines(); ++i){
    const Point * p(_valid_zone->GetPoint(i));
    if(0 == p)
      break;
    glVertex2d(p->X(), p->Y());
  }
  glEnd();
  
  // current scan
  _analysis.Draw();
  
  // background
  glColor3d(1, 1, 1);
  glBegin(GL_LINE_LOOP);
  for(int i(0); i < scansize; ++i)
    glVertex2d(_bgx[i], _bgy[i]);
  glEnd();
}


void Scanalyzer::
Extract()
{
  for(size_t i(0); i < _analysis.circle.size(); ++i)
    delete _analysis.circle[i];
  _analysis.circle.clear();
  
  for(size_t i(0); i < _analysis.startindex.size(); ++i){
    CircleLSQ * circle(CircleLSQ::Create(_analysis,
					 _analysis.startindex[i],
					 _analysis.endindex[i],
					 i));
    // Can be 0, the important thing is to have a correspondence
    // between _analysis.circle[] and its other vectors!
    _analysis.circle.push_back(circle);
  }
}


bool Scanalyzer::
LoadBackground(std::string fname)
{
  ifstream is(fname.c_str());
  for(int i(0); i < scansize; ++i){
    double r;
    if( ! (is >> r))
      return false;
    _analysis.bgrho[i] = r;
    _bgx[i] = r * _cosphi[i];
    _bgy[i] = r * _sinphi[i];
  }
  return true;
}


const Timestamp & Scanalyzer::
GetCurrentStamp() const
{
  return _analysis.t0;
}


bool Scanalyzer::
RestartSick() const
{
  Scanalyzer * that(const_cast<Scanalyzer *>(this));
  
  if(0 == _sick_poster){
    cerr << "WARNING in Scanalyzer::RestartSick(): 0 == _sick_poster.\n";
    return that->InitSick(0);
  }
  
  FILE * old_sick_dbg = _sick_poster->dbg;
  that->_sick_poster->dbg = stderr;
  
  bool ok(false);
  if(0 != sick_poster_abort(_sick_poster)){
    cerr << "Scanalyzer::RestartSick(): serial_poster_abort() failed.\n";
    ok = false;
  }

  if(ok && (0 != serial_close(_sick_poster->fd))){
    cerr << "Scanalyzer::RestartSick(): serial_close() failed.\n";
    ok = false;
  }
  
  sick_poster_delete(_sick_poster);
  that->_sick_poster = 0;
  
  ok = that->InitSick(stderr);
  that->_sick_poster->dbg = old_sick_dbg;
  return ok;
}


void Scanalyzer::
GetSickStats(double & tmin_sec, double & tmean_sec,
	     double & tmax_sec) const
{
  if((0 == _sick_poster) || (_sick_poster->tc_count < 1)){
    tmin_sec = -1;
    tmean_sec = -1;
    tmax_sec = -1;
    return;
  }
  
  tmin_sec = _sick_poster->tc_msec_min * 1e-3;
  tmax_sec = _sick_poster->tc_msec_max * 1e-3;
  tmean_sec = _sick_poster->tc_msec_sum * 1e-3 / _sick_poster->tc_count;
}


void Scanalyzer::
ConfigureViewport(Viewport & vp) const
{
  if(_valid_zone.get() == 0){
    vp.Remap(Subwindow::logical_bbox_t(0, -8, 8, 8));
    return;
  }
  
  double x0, y0, x1, y1;
  _valid_zone->BoundingBox(x0, y0, x1, y1);
  vp.Remap(Subwindow::logical_bbox_t(x0-0.5, y0-0.5, x1+0.5, y1+0.5));
}
