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


#include "Behavior.hpp"
#include "Scanalyzer.hpp"
#include "MotionManager.hpp"
#include "Effects.hpp"
#include "gfx/wrap_gl.hpp"
#include "gfx/wrap_glu.hpp"
#include <sfl/numeric.hpp>
#include <sfl/Frame.hpp>
#include <limits>
#include <iostream>


using namespace std;
using namespace sfl;


Behavior::
Behavior(double home_x, double home_y,
	 double green, double red):
  _home_x(home_x), _home_y(home_y),
  _green(green), _red(red),
  _mode(WAIT),
  _potential_target_dist(-1),
  _active_target_dist(-1),
  _target_timeout(0)
{
}


void Behavior::
FakeUpdate(double target_x, double target_y)
{
  if((target_x == _potential_target_x) && (target_y == _potential_target_y))
    return;
  
  _potential_target_x = target_x;
  _potential_target_y = target_y;
  _potential_target_dist = sqrt(sqr(_home_x - target_x)
				+ sqr(_home_y - target_y));
  
  DoUpdate();
}


void Behavior::
Update(const Scanalysis & scanalysis, const Frame & pose, double deadzone)
{
  FindTarget(scanalysis, pose, deadzone);
  DoUpdate();
}


void Behavior::
DoUpdate()
{
  _target_timeout.UpdateAbsolute();
  if( ! _target_timeout.GetExpired())
    return;
  
  if(_potential_target_dist < 0){
    _active_target_dist = -1;
    _target_timeout.Set(0);
    _mode = WAIT;
    return;
  }
  
  static const double TARGET_DURATION = 1.5;
  _target_timeout.Set(TARGET_DURATION);
  _active_target_x = _potential_target_x;
  _active_target_y = _potential_target_y;
  _active_target_dist = _potential_target_dist;
  
  if(_active_target_dist >= _green)
    _mode = WAIT;
  else if(_active_target_dist >= _red)
    _mode = AIM;
  else
    _mode = ATTACK;
}


void Behavior::
DrawRegions() const
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(_home_x, _home_y, 0);
  glColor3d(0.5, 0.5, 0.5);
  gluDisk(wrap_glu_quadric_instance(), _green, _green, 36, 1);
  gluDisk(wrap_glu_quadric_instance(), _red, _red, 36, 1);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  
  if(_active_target_dist < 0)
    return;
  
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(_home_x, _home_y, 0);
  switch(_mode){
  case WAIT:
    glColor3d(0, 0.5, 0);
    gluDisk(wrap_glu_quadric_instance(), _green, 2 * _green, 36, 1);
    break;
  case AIM:
    glColor3d(0.5, 0.25, 0);
    gluDisk(wrap_glu_quadric_instance(), _red, _green, 36, 1);
    break;
  case ATTACK:
    glColor3d(0.5, 0, 0);
    gluDisk(wrap_glu_quadric_instance(), 0, _red, 36, 1);
    break;
  default:
    cerr << "WARNING in Behavior::Draw(): invalid mode " << _mode << "\n";
  }
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}


void Behavior::
DrawTarget() const
{
  if(_active_target_dist >= 0){
    glLineWidth(3);
    switch(_mode){
    case WAIT:   glColor3d(0, 1, 0); break;
    case AIM:    glColor3d(1, 0.5, 0); break;
    case ATTACK: glColor3d(1, 0, 0); break;
    default:
      cerr << "WARNING in Behavior::DrawTarget(): invalid mode "
	   << _mode << "\n";
    }
    glBegin(GL_LINES);
    glVertex2d(_home_x, _home_y);
    glVertex2d(_active_target_x, _active_target_y);
    glEnd();
  }

  if(_potential_target_dist >= 0){
    glLineWidth(1);
    switch(_mode){
    case WAIT:   glColor3d(0, 0.5, 0); break;
    case AIM:    glColor3d(0.5, 0.25, 0); break;
    case ATTACK: glColor3d(0.5, 0, 0); break;
    default:
      cerr << "WARNING in Behavior::DrawTarget(): invalid mode "
	   << _mode << "\n";
    }
    glBegin(GL_LINES);
    glVertex2d(_home_x, _home_y);
    glVertex2d(_potential_target_x, _potential_target_y);
    glEnd();
  }
}


void Behavior::
FindTarget(const Scanalysis & scanalysis, const Frame & pose, double deadzone)
{
  _potential_target_dist = numeric_limits<double>::max();
  
  for(size_t ic(0); ic < scanalysis.circle.size(); ++ic){
    if(0 == scanalysis.circle[ic])
      continue;
    for(size_t ip(scanalysis.startindex[ic]);
	ip <= scanalysis.endindex[ic];
	++ip){
      double dx(pose.X() - scanalysis.x[ip]);
      double dy(pose.Y() - scanalysis.y[ip]);
      double ds(sqrt(sqr(dx) + sqr(dy)));
      if(ds < deadzone)
	continue;
      dx = _home_x - scanalysis.x[ip];
      dy = _home_y - scanalysis.y[ip];
      ds = sqrt(sqr(dx) + sqr(dy));
      if(ds < _potential_target_dist){
	_potential_target_dist = ds;
	_potential_target_x = scanalysis.x[ip];
	_potential_target_y = scanalysis.y[ip];
      }
    }
  }

  if(numeric_limits<double>::max() == _potential_target_dist)
    _potential_target_dist = -1;
}


void Behavior::
Perform(const Frame & pose, MotionManager & mm, Effects & effects)
{
  switch(_mode){
  case WAIT: DoWait(mm, effects); break;
  case AIM: DoAim(pose, mm, effects); break;
  case ATTACK: DoAttack(mm, effects); break;
  default:
    cerr << "ABORT in Behavior::Perform(): invalid mode " << _mode << "\n";
    exit(EXIT_FAILURE);
  }
}


void Behavior::
DoWait(MotionManager & mm, Effects & effects)
{
  static const double goal_dr(0.15);
  static const double shoulder_on_tmin(3);
  static const double shoulder_on_tmax(6);
  static const double shoulder_off_tmin(3);
  static const double shoulder_off_tmax(6);
  static const double ear_on_tmin(0.5);
  static const double ear_on_tmax(2);
  static const double ear_off_tmin(0.5);
  static const double ear_off_tmax(2);
  static const double light_tmin(3);
  static const double light_tmax(6);
  
  mm.SetGoalAndPrecision(_home_x, _home_y, 0, goal_dr, M_PI);
  effects.SetLightshow(shoulder_on_tmin, shoulder_on_tmax,
		       shoulder_off_tmin, shoulder_off_tmax,
		       ear_on_tmin, ear_on_tmax,
		       ear_off_tmin, ear_off_tmax,
		       light_tmin, light_tmax);
}


void Behavior::
DoAim(const Frame & pose, MotionManager & mm, Effects & effects)
{
  static const double goal_dr(0.15);
  static const double goal_dtheta(5 * M_PI / 180);
  static const double shoulder_on_tmin(0.5);
  static const double shoulder_on_tmax(2);
  static const double shoulder_off_tmin(0.5);
  static const double shoulder_off_tmax(2);
  static const double ear_on_tmin(3);
  static const double ear_on_tmax(6);
  static const double ear_off_tmin(3);
  static const double ear_off_tmax(6);
  static const double light_tmin(1);
  static const double light_tmax(2);

  const double dx(_active_target_x - pose.X());
  const double dy(_active_target_y - pose.Y());
  mm.SetGoalAndPrecision(_home_x, _home_y, atan2(dy, dx),
			 goal_dr, goal_dtheta);
  effects.SetLightshow(shoulder_on_tmin, shoulder_on_tmax,
		       shoulder_off_tmin, shoulder_off_tmax,
		       ear_on_tmin, ear_on_tmax,
		       ear_off_tmin, ear_off_tmax,
		       light_tmin, light_tmax);
}


void Behavior::
DoAttack(MotionManager & mm, Effects & effects)
{
  static const double goal_dr(0.15);
  static const double shoulder_on_tmin(3);
  static const double shoulder_on_tmax(6);
  static const double shoulder_off_tmin(3);
  static const double shoulder_off_tmax(6);
  static const double ear_on_tmin(0.5);
  static const double ear_on_tmax(2);
  static const double ear_off_tmin(0.5);
  static const double ear_off_tmax(2);
  static const double light_tmin(0.1);
  static const double light_tmax(0.5);
  
  mm.SetGoalAndPrecision(_active_target_x, _active_target_y, 0, goal_dr, M_PI);
  effects.SetLightshow(shoulder_on_tmin, shoulder_on_tmax,
		       shoulder_off_tmin, shoulder_off_tmax,
		       ear_on_tmin, ear_on_tmax,
		       ear_off_tmin, ear_off_tmax,
		       light_tmin, light_tmax);
}
