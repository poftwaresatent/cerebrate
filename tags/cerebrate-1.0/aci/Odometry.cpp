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


#include "Odometry.hpp"
#include "MotionManager.hpp"
#include "gfx/wrap_gl.hpp"
#include "drivers/FModIPDCMOT.hpp"
#include <sfl/numeric.hpp>
#include <cmath>


using namespace std;
using namespace sfl;


Odometry::
Odometry(FModIPDCMOT & left, FModIPDCMOT & right,
	 double wheelbase, double wheelradius):
  _wheelbase(wheelbase),
  _wheelradius(wheelradius),
  _left(left),
  _right(right)
{
  Init(Timestamp::Now(), 0, 0, 0, 0);
}


void Odometry::
Init(const Timestamp & t, double x, double y, double theta, MotionManager * mm)
{
  _ql = _left.GetPosition();
  _qr = _right.GetPosition();
  _ancient_history.clear();
  _recent_history.clear();
  _ancient_history.insert(make_pair(t, posechange(t,
						  Frame(x, y, theta),
						  Frame(x, y, theta),
						  true)));
  if(0 != mm)
    mm->InvalidateAnchor();
}


void Odometry::
Update()
{
  const int32_t ql(_left.GetPosition());
  const int32_t qr(_right.GetPosition());
  if((ql == _ql) && (qr == _qr))
    return;
  
  double dl, dr;
  Enc2Rad(ql - _ql, qr - _qr, dl, dr);
  dl *= _wheelradius;
  dr *= _wheelradius;
  _ql = ql;
  _qr = qr;
  
  const double ds     = (dl + dr) / 2;
  const double dtheta = (dr - dl) / _wheelbase;
  double dx, dy;
  if(absval(dtheta) > 1e-9){	// hax: hardcoded epsilon
    // use circular movement
    const double R = ds / dtheta;
    dx = R * sin(dtheta);
    dy = R * (1 - cos(dtheta));
  }
  else {
    // approximate with linear movement
    dx = ds * cos(0.5 * dtheta);
    dy = ds * sin(0.5 * dtheta);
  }
  
  AppendPose(Timestamp::Now(), dx, dy, dtheta);
}


void Odometry::
Correct(const Timestamp & t, const observation & observation)
{
//   if(observation.known_theta)
//     cerr << "{{{ t = " << t
// 	 << " obs = " << observation.x << ", " << observation.y
// 	 << ", " << observation.theta << " }}}\n";
  
  ichange postmatch(FindClosest(t, _recent_history));
  ichange prematch(postmatch);
  if(prematch != _recent_history.begin())
    --prematch;
  
  Frame match_global;
  if(observation.known_theta)
    match_global.Set(observation.x, observation.y, observation.theta);
  else if(prematch != _recent_history.end())
    match_global.Set(observation.x, observation.y,
		     prematch->second.global.Theta());
  else
    match_global.Set(observation.x, observation.y, 0);

  Timestamp match_stamp(t);
  if((postmatch != _recent_history.end())
     && (match_stamp >= postmatch->first))
    match_stamp = postmatch->first - Timestamp(0, 1);
  if((prematch != _recent_history.end())
     && (match_stamp <= prematch->first))
    match_stamp = prematch->first + Timestamp(0, 1);
  
  Frame match_delta;
  if(postmatch != _recent_history.end())
    match_delta.Set(postmatch->second.global.X() - match_global.X(),
		    postmatch->second.global.Y() - match_global.Y(),
		    postmatch->second.global.Theta() - match_global.Theta());
  
  // do Orwellian things with history
  _ancient_history.insert(make_pair(match_stamp,
				    posechange(match_stamp,
					       match_delta,
					       match_global,
					       true)));
  ichange ir(_recent_history.begin());
  while(ir != postmatch){
    ichange tmp(ir++);
    _recent_history.erase(tmp);
  }
  Frame retrace(match_global);
  while(ir != _recent_history.end()){
    retrace.Add(ir->second.delta.X(), ir->second.delta.Y(),
		ir->second.delta.Theta());
    ir->second.global = retrace;
    ++ir;
  }
}


void Odometry::
AppendPose(const Timestamp & t, double dx, double dy, double dtheta)
{
  const Frame current(GetCurrentPose());
  current.RotateTo(dx, dy);
  posechange change(t, Frame(dx, dy, dtheta), current, false);
  change.global.Add(dx, dy, dtheta);
  _recent_history.insert(make_pair(t, change));
  //  cerr << ":" << _recent_history.size();
}


void Odometry::
Enc2Rad(int32_t ql_enc, int32_t qr_enc,
	double & ql_rad, double & qr_rad)
{
  static const double ENC2RAD =
    2 * M_PI			// rad / rev
    / 4				// quadrature
    / 512			// tick / rev
    / 43;			// gear reduction
  ql_rad = - ql_enc * ENC2RAD;
  qr_rad =   qr_enc * ENC2RAD;
}


void Odometry::
Rad2Enc(double ql_rad, double qr_rad,
	int32_t & ql_enc, int32_t & qr_enc)
{
  static const double RAD2ENC =
    4				// quadrature
    * 512			// tick / rev
    * 43			// gear reduction
    / 2 / M_PI;			// rev / rad
  
  ql_enc = (int32_t) rint( - ql_rad * RAD2ENC);
  qr_enc = (int32_t) rint(   qr_rad * RAD2ENC);
}


Odometry::ichange Odometry::
FindClosest(const Timestamp & t, history & hist)
{
  ichange postmatch(hist.begin());
  while(postmatch != hist.end()){
    if(postmatch->first > t)
      break;
    ++postmatch;
  }
  return postmatch;
}


const Odometry::posechange & Odometry::
GetCurrent() const
{
  if(_recent_history.empty())
    return _ancient_history.rbegin()->second;
  return _recent_history.rbegin()->second;
}


const Odometry::posechange & Odometry::
GetMatched() const
{
  return _ancient_history.rbegin()->second;
}


const sfl::Frame & Odometry::
GetCurrentPose() const
{
  return GetCurrent().global;
}


const sfl::Frame & Odometry::
GetMatchedPose() const
{
  return GetMatched().global;
}


void Odometry::
DrawFrame(const Frame & frame) const
{
  const double len(_wheelbase / 2);
  glBegin(GL_LINES);
  double x(len);
  double y(0);
  frame.To(x, y);
  glVertex2d(frame.X(), frame.Y());
  glVertex2d(x, y);
  x = 0;
  y = len;
  frame.To(x, y);
  glVertex2d(frame.X(), frame.Y());
  glVertex2d(x, y);
  x = 0;
  y = - len;
  frame.To(x, y);
  glVertex2d(frame.X(), frame.Y());
  glVertex2d(x, y);
  glEnd();
}


void Odometry::
Draw() const
{
  glColor3d(0, 0.4, 0.8);
  glLineWidth(1);
  for(history::const_iterator ia(_ancient_history.begin());
      ia != _ancient_history.end(); ++ia)
    DrawFrame(ia->second.global);
  glColor3d(0, 0.5, 1);
  glBegin(GL_LINE_STRIP);
  for(history::const_iterator ir(_recent_history.begin());
      ir != _recent_history.end(); ++ir)
    glVertex2d(ir->second.global.X(), ir->second.global.Y());
  glEnd();
  glLineWidth(2);
  DrawFrame(GetCurrentPose());
  glLineWidth(1);
}
