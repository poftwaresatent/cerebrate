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


#include "Localizer.hpp"
#include "Odometry.hpp"
#include "CircleLSQ.hpp"
#include "MotionManager.hpp"
#include "gfx/wrap_gl.hpp"
#include "gfx/wrap_glu.hpp"
#include <sfl/Polygon.hpp>
#include <sfl/Frame.hpp>
#include <iostream>
#include <fstream>


using namespace std;
using namespace sfl;


Localizer::
Localizer(Odometry & odometry,
	  MotionManager & motion_manager,
	  Scanalyzer & scanalyzer):
  _cactus_radius(0.14),
  _dr_thresh(0.02),
  _deadzone(0.5),
  _odometry(odometry),
  _motion_manager(motion_manager),
  _scanalyzer(scanalyzer),
  _sick_success(Timestamp::First())
{  
}


bool Localizer::
UpdateScan()
{
  if( ! _scanalyzer.Update(0))
    return false;
  
  _sick_success = Timestamp::Now();  
  _scanalysis = _scanalyzer.GetScanalysis();
  _robot_circle = 0;
  return true;
}


bool Localizer::
UpdateMatch()
{
  static const double extract_thresh(numeric_limits<double>::max());//0.02);
  _robot_circle = _FindRobot(extract_thresh);
  if(0 == _robot_circle)
    return false;
  RelabelRobot();
  
  const MotionManager::Anchor & anchor(_motion_manager.GetAnchor());
  if( ! anchor.IsValid())
    return false;
  
  const double dx(_robot_circle->xc - anchor.GetPose().X());
  const double dy(_robot_circle->yc - anchor.GetPose().Y());
  const double ds(sqrt(sqr(dx) + sqr(dy)));
  static const double correct_theta_ds_threshold(0.5);
  if(ds < correct_theta_ds_threshold)
    return false;
  
  const double theta(atan2(dy, dx));
  if(anchor.GoingForwards())
    _odometry.Correct(_scanalysis.t0, Odometry::observation(_robot_circle->xc, 
							    _robot_circle->yc,
							    true,
							    theta));
  else
    _odometry.Correct(_scanalysis.t0,
		      Odometry::observation(_robot_circle->xc, 
					    _robot_circle->yc,
					    true,
					    mod2pi(theta + M_PI)));
  _motion_manager.InvalidateAnchor();
  return true;
}


bool Localizer::
SaveScan(const std::string & fname) const
{
  ofstream os(fname.c_str());
  for(int i(0); i < 361; ++i)
    if( ! (os << _scanalysis.rho[i] << "\n"))
      return false;
  
  return true;
}


bool Localizer::
SaveBackground(const std::string & fname) const
{
  ofstream os(fname.c_str());
  for(int i(0); i < 361; ++i)
    if( ! (os << _scanalysis.bgrho[i] << "\n"))
      return false;
  return true;
}


bool Localizer::
LoadBackground(const std::string & fname)
{
  return _scanalyzer.LoadBackground(fname);
}


const CircleLSQ * Localizer::
_FindBest()
{
  const size_t csize(_scanalysis.circle.size());
  if(csize < 1)
    return 0;
  
  double drmin(numeric_limits<double>::max());
  const CircleLSQ * best(0);
  for(size_t i(0); i < csize; ++i){
    const CircleLSQ * circle(_scanalysis.circle[i]);
    if(0 == circle)
      continue;
    const double dr(absval(circle->radius - _cactus_radius));
    if(dr < drmin){
      drmin = dr;
      best = circle;
    }
  }
  
  if(drmin > _dr_thresh)
    return 0;
  
  return best;
}


const CircleLSQ * Localizer::
_FindRobot(double extract_thresh)
{
  const size_t csize(_scanalysis.circle.size());
  if(csize < 1)
    return 0;
  
  double dmin(numeric_limits<double>::max());
  const CircleLSQ * robot(0);
  for(size_t i(0); i < csize; ++i){
    const CircleLSQ * circle(_scanalysis.circle[i]);
    if(0 == circle)
      continue;
    if(circle->maxdist > extract_thresh)
      continue;
    const double dr(absval(circle->radius - _cactus_radius));
    if(dr > _dr_thresh)
      continue;
    const Frame & pose(_odometry.GetCurrentPose());
    const double dist(sqrt(sqr(pose.X() - circle->xc)
			   + sqr(pose.Y() - circle->yc)));
    if(dist > _deadzone)
      continue;
    if(dist < dmin){
      dmin = dist;
      robot = circle;
    }
  }
  
  return robot;
}


void Localizer::
Update()
{
  if(UpdateScan())
    UpdateMatch();
}


void Localizer::
Draw() const
{
  _scanalysis.Draw();
  
  if(0 != _robot_circle){
    glColor3d(0.5, 1, 0);
    _robot_circle->Draw(false);
  }
}


void Localizer::
DrawPrediction(const sfl::Frame & pose) const
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(pose.X(), pose.Y(), 0);
  
  glColor3d(0.3, 0.3, 0.3);
  gluDisk(wrap_glu_quadric_instance(), 0, _deadzone, 36, 1);
  glColor3d(0.3, 0.8, 0.3);
  gluDisk(wrap_glu_quadric_instance(),
	  _cactus_radius - _dr_thresh,
	  _cactus_radius + _dr_thresh, 36, 1);
  
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}


const Scanalysis & Localizer::
GetScanalysis() const
{
  return _scanalysis;
}


int Localizer::
GetRobotLabel() const
{
  if(0 == _robot_circle)
    return -1;
  return _robot_circle->label;
}


void Localizer::
RelabelRobot()
{
  if(_robot_circle == 0)
    return;
  
  const int robot_label(GetRobotLabel());
  const double robot_x(_robot_circle->xc);
  const double robot_y(_robot_circle->yc);
  for(int i(0); i < _scanalysis.scansize; ++i)
    if((_scanalysis.category[i] == Scanalysis::OBJECT)
       && (_scanalysis.label[i] != robot_label)){
      const double dx(robot_x - _scanalysis.x[i]);
      const double dy(robot_y - _scanalysis.y[i]);
      const double r(sqrt(sqr(dx) + sqr(dy)));
      const double ds(absval(r - _cactus_radius));
      if(ds < _dr_thresh)
	_scanalysis.label[i] = robot_label;
    }
}


const Timestamp & Localizer::
GetTMatch() const
{
  return _odometry.GetMatched().stamp;
}
