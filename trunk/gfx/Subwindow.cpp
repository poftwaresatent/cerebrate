/* 
 * Copyright (C) 2004
 * Swiss Federal Institute of Technology, Lausanne. All rights reserved.
 * 
 * Author: Roland Philippsen <roland dot philippsen at gmx dot net>
 *         Autonomous Systems Lab <http://asl.epfl.ch/>
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


#include "Subwindow.hpp"
#include "wrap_glut.hpp"
#include <iostream>

using namespace std;


Subwindow::registry_t Subwindow::_registry;
Subwindow * Subwindow::_lastclicked(0);


Subwindow::
Subwindow(const string & name,
	  logical_bbox_t lbbox):
  _lbbox(lbbox),
  _lsize(lbbox.x1 - lbbox.x0, lbbox.y1 - lbbox.y0),
  _sbbox(0, 0, 1, 1),
  _ssize(1, 1),
  _winsize(1, 1),
  _enabled(false),
  _name(name)
{
  _registry.insert(this);
}


Subwindow::
~Subwindow()
{
  _registry.erase(this);
}


Subwindow * Subwindow::
GetSubwindow(logical_point_t lpoint)
{
  for(registry_t::iterator is(_registry.begin());
      is != _registry.end();
      ++is)
    if((*is)->_enabled && (*is)->ContainsLogicalPoint(lpoint))
      return *is;
  return 0;
}


Subwindow * Subwindow::
GetSubwindow(screen_point_t spoint)
{
  for(registry_t::iterator is(_registry.begin());
      is != _registry.end();
      ++is)
    if((*is)->_enabled && (*is)->ContainsScreenPoint(spoint))
      return *is;
  return 0;
}


void Subwindow::
DispatchUpdate()
{
  for(registry_t::iterator is(_registry.begin());
      is != _registry.end();
      ++is)
    if((*is)->_enabled)
      (*is)->Update();
}


void Subwindow::
DispatchResize(screen_point_t winsize)
{
  for(registry_t::iterator is(_registry.begin());
      is != _registry.end();
      ++is)
    if((*is)->_enabled)
      (*is)->Resize(winsize);
  Root()->Resize(winsize);
}


void Subwindow::
DispatchClick(int button,
	      int state,
	      screen_point_t mouse)
{
  Root()->InvertYaxis(mouse);
  Subwindow * sw(GetSubwindow(mouse));
  if(sw == 0){
    _lastclicked = 0;
    return;
  }
  
  sw->Click(button, state, mouse);
  
  if(state == GLUT_DOWN)
    _lastclicked = sw;
}


void Subwindow::
DispatchDrag(screen_point_t mouse)
{
  if(_lastclicked == 0){
    return;
  }

  Root()->InvertYaxis(mouse);
  _lastclicked->Drag(mouse);
}


void Subwindow::
Update()
{
}


void Subwindow::
Resize(screen_point_t winsize)
{
  if((winsize.x == _winsize.x) &&
     (winsize.y == _winsize.y))
    return;

  _winsize = winsize;
  _sbbox = Logical2Screen(_lbbox);
  _ssize.x = _sbbox.x1 - _sbbox.x0;
  _ssize.y = _sbbox.y1 - _sbbox.y0;
  
  PostResize();
}


void Subwindow::
Reposition(logical_bbox_t lbbox)
{
  if((lbbox.x0 == _lbbox.x0) &&
     (lbbox.y0 == _lbbox.y0) &&
     (lbbox.x1 == _lbbox.x1) &&
     (lbbox.y1 == _lbbox.y1))
    return;

  _lbbox = lbbox;
  _lsize.x = lbbox.x1 - lbbox.x0;
  _lsize.y = lbbox.y1 - lbbox.y0;
  _sbbox = Logical2Screen(lbbox);
  _ssize.x = _sbbox.x1 - _sbbox.x0;
  _ssize.y = _sbbox.y1 - _sbbox.y0;
  
  PostResize();
}


Subwindow * Subwindow::
Root()
{
  // NOTE: Takes up some space and time due to _registry, but stays
  // disabled forever
  class Rootwin: public Subwindow {
  public:
    Rootwin() throw(): Subwindow("__root", logical_bbox_t(0, 0, 1, 1)) {}
    virtual void PushProjection() const throw(){}
    virtual void PopProjection() const throw(){}
  protected:
    virtual void PostResize() throw(){}
    virtual void Click(int button, int state, screen_point_t mouse) throw(){}
    virtual void Drag(screen_point_t mouse) throw(){}
  };

  static Rootwin root;
  return & root;
}


void Subwindow::
Enable()
{
  _enabled = true;
  Resize(Root()->_winsize);
}


void Subwindow::
Disable()
{
  _enabled = false;
}
