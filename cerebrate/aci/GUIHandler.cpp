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


#include "GUIHandler.hpp"
#include "gfx/Viewport.hpp"
#include "gfx/wrap_glu.hpp"
#include <sfl/numeric.hpp>
#include <iostream>		// dbg


using namespace std;
using namespace sfl;


GUIHandler::
~GUIHandler()
{
  for(cb_t::iterator ic(_cb.begin()); ic != _cb.end(); ++ic)
    delete * ic;
}


void GUIHandler::
AddButton(double x0, double y0, double x1, double y1,
	  double r, double g, double b,
	  GUICallback * callback)
{
  if(_cb.empty()){
    _x0 = x0;
    _y0 = y0;
    _x1 = x1;
    _y1 = y1;
    cerr << "DBG foo " << _x0 << " " << _y0 << " " << _x1 << " " << _y1 << "\n"; 
  }
  else{
    _x0 = minval(_x0, x0);
    _y0 = minval(_y0, y0);
    _x1 = maxval(_x1, x1);
    _y1 = maxval(_y1, y1);
    cerr << "DBG bar " << _x0 << " " << _y0 << " " << _x1 << " " << _y1 << "\n"; 
  }
  
  _cb.push_back(new button(x0, y0, x1, y1, r, g, b, callback));
}


void GUIHandler::
Draw() const
{
  if(_cb.empty())
    return;
  
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for(cb_t::const_iterator ic(_cb.begin()); ic != _cb.end(); ++ic){
    glColor3d((*ic)->r, (*ic)->g, (*ic)->b);
    glRectd((*ic)->x0, (*ic)->y0, (*ic)->x1, (*ic)->y1);
  }
}


void GUIHandler::
ConfigureViewport(Viewport & vp) const
{
  if(_cb.empty())
    return;

  cerr << "DBG foobar " << _x0 << " " << _y0 << " " << _x1 << " " << _y1 << "\n"; 
  
  vp.Remap(Subwindow::logical_bbox_t(_x0, _y0, _x1, _y1));
}


void GUIHandler::
HandleClick(double x, double y)
{
  for(cb_t::iterator ic(_cb.begin()); ic != _cb.end(); ++ic)
    if((x >= (*ic)->x0) && (y >= (*ic)->y0)
       && (x <= (*ic)->x1) && (y <= (*ic)->y1)){
      (*ic)->cb->Do();
      return;
    }
}


GUIHandler::button::
button(double _x0, double _y0, double _x1, double _y1,
       double _r, double _g, double _b,
       GUICallback * _cb):
  x0(_x0), y0(_y0), x1(_x1), y1(_y1), r(_r), g(_g), b(_b), cb(_cb)
{
}
