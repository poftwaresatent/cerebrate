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


#include "Timeout.hpp"
#include "gfx/wrap_gl.hpp"


Timeout::
Timeout(const Timestamp & duration):
  _duration(duration),
  _duration_sec(duration.ConvertToSeconds())
{
}


void Timeout::
UpdateRelative(const Timestamp & delta)
{
  _delta = delta;
  _delta_sec = delta.ConvertToSeconds();
  _expired = (delta > _duration);
  _fraction = _delta_sec / _duration_sec;
}


void Timeout::
UpdateAbsolute()
{
  UpdateRelative(Timestamp::Now() - _absolute_t0);
}


void Timeout::
Draw(double x0, double x1) const
{
  if(_fraction < 0.5)
    glColor3d(0, 1, 0);
  else if(_fraction < 1)
    glColor3d(1, 0.5, 0);
  else
    glColor3d(1, 0, 0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glRectd(x0, 0, x1, _fraction);
}


void Timeout::
Set(double seconds)
{
  _duration = Timestamp(seconds);
  _duration_sec = seconds;
  _absolute_t0 = Timestamp::Now();
}
