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


#include "Viewport.hpp"
#include "Mousehandler.hpp"
#include "wrap_glut.hpp"
#include <sfl/numeric.hpp>
#include <cmath>


using namespace std;
using namespace sfl;


Viewport::
Viewport(const string & name,
	 logical_bbox_t realbbox,
	 logical_bbox_t lbbox,
	 bool preserve_aspect,
	 double remap_thresh):
  Subwindow(name, lbbox),
  _remap_thresh(remap_thresh),
  _padded_sbbox(0, 0, 1, 1),
  _padded_ssize(1, 1),
  _rbbox(realbbox),
  _rsize(realbbox.x1 - realbbox.x0, realbbox.y1 - realbbox.y0),
  _preserve_aspect(preserve_aspect)
{
  PostResize();
}


Viewport * Viewport::
Clone(const string & newname)
  const
{
  return
    new Viewport(newname, _rbbox, Lbbox(), _preserve_aspect, _remap_thresh);
}


void Viewport::
PostResize()
{
  _padded_sbbox.x0 = Sbbox().x0;
  _padded_sbbox.y0 = Sbbox().y0;
  _padded_sbbox.x1 = Sbbox().x1;
  _padded_sbbox.y1 = Sbbox().y1;
  _padded_ssize.x = Ssize().x;
  _padded_ssize.y = Ssize().y;
  
  CalculatePadding();
}


void Viewport::
Remap(logical_bbox_t realbbox)
{
  if((absval(_rbbox.x0 - realbbox.x0) < _remap_thresh) &&
     (absval(_rbbox.y0 - realbbox.y0) < _remap_thresh) &&
     (absval(_rbbox.x1 - realbbox.x1) < _remap_thresh) &&
     (absval(_rbbox.y1 - realbbox.y1)) < _remap_thresh)
    return;
  
  _rbbox = realbbox;
  _rsize.x = realbbox.x1 - realbbox.x0;
  _rsize.y = realbbox.y1 - realbbox.y0;
  
  PostResize();
}


void Viewport::
CalculatePadding()
{
  if( ! _preserve_aspect)
    return;
  
  if((Ssize().x == 0) || (Ssize().y == 0) ||
     (_rsize.x == 0) || (_rsize.y == 0))
    return;

  double screenaspect(static_cast<double>(Ssize().x) / Ssize().y);
  double realaspect(_rsize.x / _rsize.y);

  // Note: We calculate the double of the padding, will be halved
  // before applying to bounding box.
  double pad_screenx(0);
  double pad_screeny(0);
  if(screenaspect > realaspect)
    pad_screenx = minval(_padded_ssize.x - 1, // upper bound
			 Ssize().x - Ssize().y * realaspect);
  else if(screenaspect < realaspect)
    pad_screeny = minval(_padded_ssize.y - 1, // upper bound
			 Ssize().y - Ssize().x / realaspect);

  _padded_ssize.x -= pad_screenx;
  _padded_ssize.y -= pad_screeny;
  
  pad_screenx /= 2;
  pad_screeny /= 2;
  _padded_sbbox.x0 += pad_screenx;
  _padded_sbbox.y0 += pad_screeny;
  _padded_sbbox.x1 -= pad_screenx;
  _padded_sbbox.y1 -= pad_screeny;
}


void Viewport::
PushProjection()
  const
{
  glViewport(static_cast<int>(floor(_padded_sbbox.x0)),
	     static_cast<int>(floor(_padded_sbbox.y0)),
	     static_cast<int>(floor(_padded_ssize.x)),
	     static_cast<int>(floor(_padded_ssize.y)));

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(_rbbox.x0, _rbbox.x1, _rbbox.y0, _rbbox.y1);
}


void Viewport::
PopProjection()
  const
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();  
}


void Viewport::
Click(int button,
      int state,
      screen_point_t mouse)
{
  if(state == GLUT_DOWN){
    switch(button){
    case GLUT_RIGHT_BUTTON:
      _lastdown = RIGHT;
      break;
    case GLUT_LEFT_BUTTON:
      _lastdown = LEFT;
      break;
    case GLUT_MIDDLE_BUTTON:
      _lastdown = MIDDLE;
      break;
    default:
      _lastdown = NONE;
    }
    return;
  }
  
  logical_point_t rp(PaddedScreen2Real(mouse));
  if(((button == GLUT_LEFT_BUTTON) && (_lastdown == LEFT))
     || ((button == GLUT_RIGHT_BUTTON) && (_lastdown == RIGHT))
     || ((button == GLUT_MIDDLE_BUTTON) && (_lastdown == MIDDLE))){
    handler_t::iterator ih(_handler.find(_lastdown));
    if((ih != _handler.end()) && (ih->second != 0))
      ih->second->HandleClick(rp.x, rp.y);
  }
  _lastdown = NONE;
}


void Viewport::
Drag(screen_point_t mouse)
{
  // do nothing
}


PassiveViewport::
PassiveViewport(const string & name,
		logical_bbox_t realbbox,
		logical_bbox_t lbbox,
		bool preserve_aspect,
		double remap_thresh):
  Viewport(name,
	   realbbox,
	   lbbox,
	   preserve_aspect,
	   remap_thresh)
{
}


void PassiveViewport::
Click(int button,
      int state,
      screen_point_t mouse)
{
  // do nothing
}


Subwindow::logical_point_t Viewport::
PaddedScreen2Real(screen_point_t pixel)
  const
{
  return logical_point_t(_rbbox.x0 +
			 _rsize.x *
			 (pixel.x - _padded_sbbox.x0) / _padded_ssize.x,
			 _rbbox.y0 +
			 _rsize.y *
			 (pixel.y - _padded_sbbox.y0) / _padded_ssize.y);
}


void Viewport::
SetMousehandler(button_t button,
		Mousehandler * mousehandler)
{
  handler_t::iterator ih(_handler.find(button));
  if(ih == _handler.end())
    _handler.insert(make_pair(button, mousehandler));
  else
    ih->second = mousehandler;
}
