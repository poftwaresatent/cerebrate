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


#ifndef SUBWINDOW_HPP
#define SUBWINDOW_HPP


#include <string>
#include <set>
#include <cmath>


/**
   \note (MAYBE ONLY FOR MOUSE?) Screen coordinates (pixels, window
   sizes) are measured with the y-axis "hanging down", but logical
   coordinates (subwindow bounding boxes, logical points) have a
   conventional y-axis. Subclasses need to be aware of this
   discrepancy.
*/
class Subwindow
{
public:
  template<typename T>
  class bbox {
  public:
    bbox(T _x0, T _y0, T _x1, T _y1):
      x0(_x0), y0(_y0), x1(_x1), y1(_y1) { }
    
    friend std::ostream & operator << (std::ostream & os, const bbox & b) {
      return os << "(" << b.x0 << ", " << b.y0
		<< ", " << b.x1 << ", " << b.y1 << ")";
    }
    
    T x0, y0, x1, y1;
  };

  template<typename T>
  class point {
  public:
    point(T _x, T _y):
      x(_x), y(_y) { }
    
    T x, y;
  };

  typedef bbox<double> logical_bbox_t;
  typedef bbox<int> screen_bbox_t;
  typedef point<double> logical_point_t;
  typedef point<int> screen_point_t;
  
  
  Subwindow(const std::string & name,
	    logical_bbox_t lbbox);
  virtual ~Subwindow();
  
  static Subwindow * GetSubwindow(logical_point_t lpoint);
  static Subwindow * GetSubwindow(screen_point_t spoint);
  static void DispatchUpdate();
  static void DispatchResize(screen_point_t winsize);
  static void DispatchDrag(screen_point_t mouse);
  static void DispatchClick(int button, int state, screen_point_t mouse);

  virtual void PushProjection() const = 0;
  virtual void PopProjection() const = 0;
  
  /** \note Default implementation is NOP. */
  virtual void Update();

  void Resize(screen_point_t winsize);
  void Reposition(logical_bbox_t lbbox);
  void Enable();
  void Disable();
  inline bool ContainsLogicalPoint(logical_point_t lpoint) const;
  inline bool ContainsScreenPoint(screen_point_t spoint) const;
  inline const std::string & Name() const;
  inline bool Enabled() const;

  inline logical_point_t Screen2Logical(screen_point_t pixel) const;
  inline logical_bbox_t Screen2Logical(screen_bbox_t sbbox) const;
  inline screen_point_t Logical2Screen(logical_point_t lpoint) const;
  inline screen_bbox_t Logical2Screen(logical_bbox_t lbbox) const;

  inline void InvertYaxis(screen_point_t & spoint) const;
  inline void InvertYaxis(screen_bbox_t & sbbox) const;
  inline void InvertYaxis(logical_point_t & lpoint) const;
  inline void InvertYaxis(logical_bbox_t & lbbox) const;

protected:
  virtual void PostResize() = 0;
  virtual void Click(int button, int state, screen_point_t mouse) = 0;
  virtual void Drag(screen_point_t mouse) = 0;

  inline const logical_bbox_t & Lbbox() const { return _lbbox; }
  inline const logical_point_t & Lsize() const { return _lsize; }
  inline const screen_bbox_t & Sbbox() const { return _sbbox; }
  inline const screen_point_t & Ssize() const { return _ssize; }
  inline const screen_point_t & Winsize() const { return _winsize; }


private:
  typedef std::set<Subwindow *> registry_t;

  
  static Subwindow * Root();


  static registry_t _registry;
  static Subwindow * _lastclicked;

  logical_bbox_t _lbbox;
  logical_point_t _lsize;
  screen_bbox_t _sbbox;
  screen_point_t _ssize;
  screen_point_t _winsize;
  bool _enabled;
  std::string _name;
};


const std::string & Subwindow::
Name()
  const 
{
  return _name;
}


bool Subwindow::
ContainsLogicalPoint(logical_point_t lpoint)
  const 
{
  return
    (lpoint.x >= _lbbox.x0) &&
    (lpoint.y >= _lbbox.y0) &&
    (lpoint.x <= _lbbox.x1) &&
    (lpoint.y <= _lbbox.y1);
}


bool Subwindow::
ContainsScreenPoint(screen_point_t spoint)
  const 
{
  return
    (spoint.x >= _sbbox.x0) &&
    (spoint.y >= _sbbox.y0) &&
    (spoint.x <= _sbbox.x1) &&
    (spoint.y <= _sbbox.y1);
}


bool Subwindow::
Enabled()
  const 
{
  return _enabled;
}


Subwindow::logical_point_t Subwindow::
Screen2Logical(screen_point_t pixel)
  const 
{
  return logical_point_t(static_cast<double>(pixel.x) / _winsize.x,
			 static_cast<double>(pixel.y) / _winsize.y);
}


Subwindow::logical_bbox_t Subwindow::
Screen2Logical(screen_bbox_t sbbox)
  const 
{
  return logical_bbox_t(static_cast<double>(sbbox.x0) / _winsize.x,
			static_cast<double>(sbbox.y0) / _winsize.y,
			static_cast<double>(sbbox.x1) / _winsize.x,
			static_cast<double>(sbbox.y1) / _winsize.y);
}


Subwindow::screen_point_t Subwindow::
Logical2Screen(logical_point_t lpoint)
  const 
{
  return screen_point_t(static_cast<int>(rint(lpoint.x * _winsize.x)),
			static_cast<int>(rint(lpoint.y * _winsize.y)));
}


Subwindow::screen_bbox_t Subwindow::
Logical2Screen(logical_bbox_t lbbox)
  const 
{
  return screen_bbox_t(static_cast<int>(floor(lbbox.x0 * _winsize.x)),
		       static_cast<int>(floor(lbbox.y0 * _winsize.y)),
		       static_cast<int>(ceil( lbbox.x1 * _winsize.x)),
		       static_cast<int>(ceil( lbbox.y1 * _winsize.y)));
}


void Subwindow::
InvertYaxis(screen_point_t & spoint)
  const 
{
  spoint.y = _winsize.y - spoint.y;
}


void Subwindow::
InvertYaxis(screen_bbox_t & sbbox)
  const 
{
  int y0bak(sbbox.y0);
  sbbox.y0 = _winsize.y - sbbox.y1;
  sbbox.y1 = _winsize.y - y0bak;
}


void Subwindow::
InvertYaxis(logical_point_t & lpoint)
  const 
{
  lpoint.y = 1 - lpoint.y;
}


void Subwindow::
InvertYaxis(logical_bbox_t & lbbox)
  const
{
  double y0bak(lbbox.y0);
  lbbox.y0 = 1 - lbbox.y1;
  lbbox.y1 = 1 - y0bak;
}

#endif // SUBWINDOW_HPP
