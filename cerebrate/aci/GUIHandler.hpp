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


#ifndef GUI_HANDLER_HPP
#define GUI_HANDLER_HPP


#include <gfx/Mousehandler.hpp>
#include <vector>


class Viewport;


class GUICallback
{
public:
  virtual ~GUICallback() {}
  virtual void Do() = 0;
};


class GUIHandler:
  public Mousehandler
{
public:
  ~GUIHandler();
  
  void AddButton(double x0, double y0, double x1, double y1,
		 double r, double g, double b,
		 GUICallback * callback);
  void Draw() const;
  void ConfigureViewport(Viewport & vp) const;
  
  virtual void HandleClick(double x, double y);

private:
  class button {
  public:
    button(double x0, double y0, double x1, double y1,
	   double r, double g, double b,
	   GUICallback * cb);
    const double x0, y0, x1, y1, r, g, b;
    GUICallback * cb;
  };
  
  typedef std::vector<button *> cb_t;
  
  cb_t _cb;
  double _x0, _y0, _x1, _y1;
};

#endif // GUI_HANDLER_HPP
