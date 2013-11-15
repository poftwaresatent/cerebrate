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


#include "Watchdog.hpp"
#include "Scanalyzer.hpp"
#include "Localizer.hpp"
#include "gfx/wrap_gl.hpp"
#include "gfx/Viewport.hpp"
#include "drivers/FModIPDCMOT.hpp"
#include <sfl/numeric.hpp>
#include <iostream>


using namespace std;
using namespace sfl;


Watchdog::
Watchdog(const Localizer & localizer,
	 const Timestamp & localizer_timeout,
	 const Scanalyzer & scanalyzer,
	 const Timestamp & sick_ok_timeout,
	 const Timestamp & sick_recover_timeout,
	 FModIPDCMOT & left, FModIPDCMOT & right):
  _localizer(localizer),
  _localizer_timeout(localizer_timeout),
  _localizer_state(OK),
  _scanalyzer(scanalyzer),
  _sick_ok_timeout(sick_ok_timeout),
  _sick_recover_timeout(sick_ok_timeout + sick_recover_timeout),
  _sick_state(OK),
  _left(left),
  _right(right)
{
}


void Watchdog::
Update()
{
  const Timestamp now(Timestamp::Now());
  
  _localizer_timeout.UpdateRelative(now - _localizer.GetTMatch());
  if(_localizer_timeout.GetExpired())
    _localizer_state = ILL;
  else
    _localizer_state = OK;
  
  _sick_ok_timeout.UpdateRelative(now - _scanalyzer.GetCurrentStamp());
  _sick_recover_timeout.UpdateRelative(_sick_ok_timeout.GetDelta());
  switch(_sick_state){
  case OK:
    if(_sick_ok_timeout.GetExpired())
      _sick_state = ILL;
    break;
  case RECOVERING:
    if(_sick_recover_timeout.GetExpired())
      _sick_state = DEAD;
    break;
  case ILL:
  case DEAD:
    // resolved by external action (RestartSick() or similar).
    break;
  default:
    cerr << "ERROR in Watchdog::Update(): Illegal sick_state " << _sick_state
	 << "\n";
    exit(EXIT_FAILURE);
  }
  
  // update motor state
  _leftcom = _left.GetCommand();
  _rightcom = _right.GetCommand();
  _motors_ok =
    (absval(_leftcom) <= FModIPDCMOT::max_command)
    && (absval(_rightcom) <= FModIPDCMOT::max_command);
}


Watchdog::state_t Watchdog::
GetSickState() const
{
  return _sick_state;
}


void Watchdog::
RestartSick()
{
  if(_sick_state == ILL){
    _sick_state = RECOVERING;
    if( ! _scanalyzer.RestartSick())
      _sick_state = DEAD;
    else
      _sick_state = OK;
  }
}


bool Watchdog::
MotorsOk() const
{
  return _motors_ok;
}


void Watchdog::
ConfigureViewport(Viewport & vp) const
{
  vp.Remap(Subwindow::logical_bbox_t(0, 0, 5, 1));
}


void Watchdog::
Draw() const
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  
  double load(absval(_leftcom / (double) FModIPDCMOT::max_command));
  if(load < 0.5)
    glColor3d(0, 1, 0);
  else if(load < 1)
    glColor3d(1, 0.5, 0);
  else
    glColor3d(1, 0, 0);
  glRectd(0.1, 0, 0.95, load);
  
  load = absval(_rightcom / (double) FModIPDCMOT::max_command);
  if(load < 0.5)
    glColor3d(0, 1, 0);
  else if(load < 1)
    glColor3d(1, 0.5, 0);
  else
    glColor3d(1, 0, 0);
  glRectd(1.05, 0, 1.9, load);
  
  _sick_ok_timeout.Draw(2.1, 2.95);
  _sick_recover_timeout.Draw(3.05, 3.9);
  _localizer_timeout.Draw(4.1, 4.9);
  
  //   double tmin, tmean, tmax;
  //   _scanalyzer.GetSickStats(tmin, tmean, tmax);
  //   if(tmin > 0){
  //     tmin /= _sick_ok_timeout.GetDeltaSec();
  //     tmean /= _sick_ok_timeout.GetDeltaSec();
  //     tmax /= _sick_ok_timeout.GetDeltaSec();
  //     glColor3d(1, 1, 1);
  //     glBegin(GL_LINES);
  //     glVertex2d(2.3, tmin);
  //     glVertex2d(2.75, tmin);
  //     glVertex2d(2.2, tmean);
  //     glVertex2d(2.85, tmean);
  //     glVertex2d(2.3, tmax);
  //     glVertex2d(2.75, tmax);
  //     glVertex2d(2.5, tmin);
  //     glVertex2d(2.5, tmax);
  //     glEnd();
  //   }
  if(_sick_state == RECOVERING){
    glColor3d(0, 1, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2);
    glRectd(2.1, 0, 2.95, 1);
    glLineWidth(1);
  }
}


Watchdog::state_t Watchdog::
GetLocalizerState() const
{
  return _localizer_state;
}
