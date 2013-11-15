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


#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP


#include <gfx/Subwindow.hpp>
#include <map>


class Mousehandler;


class Viewport:
  public Subwindow
{
public:
  typedef enum { NONE, LEFT, MIDDLE, RIGHT } button_t;


  Viewport(const std::string & name,
	   logical_bbox_t realbbox,
	   logical_bbox_t lbbox,
	   bool preserve_aspect,
	   double remap_thresh);

  Viewport * Clone(const std::string & newname) const;


  virtual void PushProjection() const;
  virtual void PopProjection() const;

  void Remap(logical_bbox_t realbbox);
  logical_point_t PaddedScreen2Real(screen_point_t pixel) const;
  
  void SetMousehandler(button_t button, Mousehandler * mousehandler);


protected:
  virtual void PostResize();
  virtual void Click(int button, int state, screen_point_t mouse);
  virtual void Drag(screen_point_t mouse);
  
  typedef std::map<button_t, Mousehandler *> handler_t;
  handler_t _handler;
  
  
private:
  void CalculatePadding();


  const double _remap_thresh;
  logical_bbox_t _padded_sbbox;
  logical_point_t _padded_ssize;
  logical_bbox_t _rbbox;
  logical_point_t _rsize;
  bool _preserve_aspect;
  button_t _lastdown;
};


class PassiveViewport:
  public Viewport
{
public:
  PassiveViewport(const std::string & name,
		  logical_bbox_t realbbox,
		  logical_bbox_t lbbox,
		  bool preserve_aspect,
		  double remap_thresh);
  
protected:
  virtual void Click(int button, int state, screen_point_t mouse);  
};

#endif // VIEWPORT_HPP
