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


#include "MotionManager.hpp"
#include "Odometry.hpp"
#include "gfx/wrap_gl.hpp"
#include "gfx/wrap_glu.hpp"
#include "drivers/FModIPDCMOT.hpp"
#include <sfl/numeric.hpp>


using namespace sfl;
using namespace std;


class MPState
{
protected:
  MPState(MotionManager * _that): that(_that) {}
  
  MotionManager * that;
  
public:
  static MPState * WaitInstance(MotionManager * that);
  static MPState * ManualInstance(MotionManager * that);
  static class MPGoal * GoalInstance(MotionManager * that);
  static class MPUnblock * UnblockInstance(MotionManager * that);
  
  virtual MPState * Do() = 0;
  virtual void Draw() const = 0;
};


class MPGoal: public MPState {
public:
  static const double _dtheta_home = 3 * M_PI / 180;
  static const double _dtheta_reaim = 20 * M_PI / 180;
  static const double _kp_s = 0.6;
  static const double _kp_theta = 1.5;
  static const double _forbidden_theta = M_PI / 2;
  
  typedef enum { AIMING, HOMING, ADJUSTING, ATGOAL } mode;

  const double _sd_max;
  const double _thetad_max;
  
  mode _mode;
  
  MPGoal(MotionManager * that):
    MPState(that),
    _sd_max(that->GetSdMax()),
    _thetad_max(that->GetThetadMax())
  {
  }

  bool ForbiddenDTheta(double theta, double dtheta) {
    const double dtheta_lim(mod2pi(_forbidden_theta - theta));
    if((dtheta_lim > 0) && (dtheta > dtheta_lim))
      return true;
    if((dtheta_lim < 0) && (dtheta < dtheta_lim))
      return true;
    return false;
  }
  
  
  virtual MPState * Do() {
    // copy some objects because of multithreading
    Frame pose(that->GetPose());
    switch(_mode){
    case AIMING:    _mode = DoAiming(pose); break;
    case HOMING:    _mode = DoHoming(pose); break;
    case ADJUSTING: _mode = DoAdjusting(pose); break;
    case ATGOAL:    that->DoSetSpeed(0, 0); break;
    default:
      cerr << "ERROR in MPGoal::Do(): invalid mode " << _mode << "\n";
      exit(EXIT_FAILURE);
    }
    return this;
  }

  
  virtual void Draw() const {
    const Frame pose(that->GetPose());
    
    switch(_mode){

    case AIMING:
      glColor3d(1, 0.5, 0.5);
      glLineWidth(3);
      glBegin(GL_LINES);
      glVertex2d(pose.X(), pose.Y());
      glVertex2d(that->_goal_x, that->_goal_y);
      glEnd();
      glLineWidth(1);
      glPolygonMode(GL_FRONT, GL_LINE);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glTranslated(that->_goal_x, that->_goal_y, 0);
      gluDisk(wrap_glu_quadric_instance(), that->_goal_dr, that->_goal_dr,
	      36, 1);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      break;
      
    case HOMING:
      glColor3d(1, 0.5, 0);
      glLineWidth(3);
      glBegin(GL_LINES);
      glVertex2d(pose.X(), pose.Y());
      glVertex2d(that->_goal_x, that->_goal_y);
      glEnd();
      glLineWidth(1);
      glPolygonMode(GL_FRONT, GL_LINE);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glTranslated(that->_goal_x, that->_goal_y, 0);
      gluDisk(wrap_glu_quadric_instance(), that->_goal_dr, that->_goal_dr,
	      36, 1);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      break;

    case ADJUSTING:
      glColor3d(1, 1, 0);
      if(that->_goal_dtheta < M_PI){
	glLineWidth(3);
	glBegin(GL_LINES);
	for(double theta(that->_goal_theta - that->_goal_dtheta);
	    theta <= that->_goal_theta + that->_goal_dtheta;
	    theta += that->_goal_dtheta){
	  glVertex2d(pose.X(), pose.Y());
	  glVertex2d(pose.X() + that->_goal_dr * cos(theta),
		     pose.Y() + that->_goal_dr * sin(theta));
	}
	glEnd();
      }
      glLineWidth(1);
      glPolygonMode(GL_FRONT, GL_LINE);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glTranslated(that->_goal_x, that->_goal_y, 0);
      gluDisk(wrap_glu_quadric_instance(), that->_goal_dr, that->_goal_dr,
	      36, 1);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      break;
      
    case ATGOAL:
      glColor3d(0, 1, 0);
      glLineWidth(1);
      if(that->_goal_dtheta < M_PI){
	glBegin(GL_LINES);
	for(double theta(that->_goal_theta - that->_goal_dtheta);
	    theta <= that->_goal_theta + that->_goal_dtheta;
	    theta += that->_goal_dtheta){
	  glVertex2d(pose.X(), pose.Y());
	  glVertex2d(pose.X() + that->_goal_dr * cos(theta),
		     pose.Y() + that->_goal_dr * sin(theta));
	}
	glEnd();
      }
      glLineWidth(3);
      glPolygonMode(GL_FRONT, GL_LINE);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glTranslated(that->_goal_x, that->_goal_y, 0);
      gluDisk(wrap_glu_quadric_instance(), that->_goal_dr, that->_goal_dr,
	      36, 1);
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glLineWidth(1);
      break;

    default:
      cerr << "ERROR in MPGoal::Draw(): invalid mode " << _mode << "\n";
      exit(EXIT_FAILURE);
    }

  }

  
  mode DoAiming(const Frame & pose) {
    double dx(that->_goal_x - pose.X());
    double dy(that->_goal_y - pose.Y());
    pose.RotateFrom(dx, dy);
    double dtheta(atan2(dy, dx));
    //     cerr << "MPGoal::DoAiming():\n"
    // 	 << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    if(dtheta > M_PI / 2)
      dtheta = dtheta - M_PI;
    else if(dtheta < - M_PI / 2)
      dtheta = dtheta + M_PI;
    //    cerr << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    if(ForbiddenDTheta(pose.Theta(), dtheta)){
      if(dtheta > 0)
	dtheta = dtheta - M_PI;
      else
	dtheta = dtheta + M_PI;
      //       cerr << "  FORBIDDEN DTHETA\n"
      // 	   << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    }
    if(absval(dtheta) < _dtheta_home){
      //      cerr << "[[[ AIMING -> HOMING ]]]\n";
      //      that->SetQd(0, 0);
      return HOMING;
    }
    //     cerr << "  thetad = "
    // 	 << minval(maxval(dtheta * _kp_theta, - _thetad_max),
    // 				    _thetad_max) << "\n";
    that->SetSpeed(0, minval(maxval(dtheta * _kp_theta, - _thetad_max),
			     _thetad_max));
    return AIMING;
  }
  
  
  mode DoAdjusting(const Frame & pose) {
    double dtheta(mod2pi(that->_goal_theta - pose.Theta()));
    //     cerr << "MPGoal::DoAdjusting():\n"
    // 	 << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    if(absval(dtheta) <= that->_goal_dtheta){
      //       cerr << "[[[ ADJUSTING -> ATGOAL ]]]\n";
      //      that->DoSetSpeed(0, 0);
      return ATGOAL;
    }
    if(ForbiddenDTheta(pose.Theta(), dtheta)){
      if(dtheta > 0)
	dtheta = dtheta - 2 * M_PI;
      else
	dtheta = dtheta + 2 * M_PI;
      //       cerr << "  FORBIDDEN DTHETA\n"
      // 	   << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    }
    //     cerr << "  thetad = "
    // 	 << minval(maxval(dtheta * _kp_theta, - _thetad_max),
    // 		   _thetad_max) << "\n";
    that->SetSpeed(0, minval(maxval(dtheta * _kp_theta, - _thetad_max),
			     _thetad_max));
    return ADJUSTING;
  }
  
  
  mode DoHoming(const Frame & pose) {
    double dx(that->_goal_x - pose.X());
    double dy(that->_goal_y - pose.Y());
    double ds(sqrt(sqr(dx) + sqr(dy)));
    if(ds <= that->_goal_dr){
      //      cerr << "[[[ HOMING -> ADJUSTING ]]]\n";
      //      that->SetQd(0, 0);
      return ADJUSTING;
    }
    pose.RotateFrom(dx, dy);
    double dtheta(atan2(dy, dx));
    //     cerr << "MPGoal::DoHoming():\n"
    // 	 << "  ds = " << ds << "\n"
    // 	 << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    if(absval(dtheta) > M_PI / 2){
      ds = - ds;
      dtheta = mod2pi(dtheta - M_PI);
    }
    //     cerr << "MPGoal::DoHoming():\n"
    // 	 << "  ds = " << ds << "\n"
    // 	 << "  dtheta = " << 180 * dtheta / M_PI << "\n";
    if(absval(dtheta) > _dtheta_reaim){
      //      cerr << "[[[ HOMING -> AIMING ]]]\n";
      //      that->SetQd(0, 0);
      return AIMING;
    }
    //     cerr << "  sd = " << minval(maxval(ds * _kp_s, - _sd_max), _sd_max)
    // 	 << "\n";
    that->SetSpeed(minval(maxval(ds * _kp_s, - _sd_max), _sd_max), 0);
    return HOMING;
  }
};


class MPWait: public MPState {
public:
  MPWait(MotionManager * that): MPState(that) {}
  
  virtual MPState * Do() {
    that->DoSetQd(0, 0);
    return this;
  }
  
  virtual void Draw() const {}
};


class MPManual: public MPState {
public:
  MPManual(MotionManager * that): MPState(that) {}
  
  virtual MPState * Do() {
    return this;
  }
  
  virtual void Draw() const {}
};


class MPUnblock: public MPState {
public:
  MPUnblock(MotionManager * that): MPState(that) {}
  
  Timestamp _tout;
  
  virtual MPState * Do() {
    if(_tout > Timestamp::Now())
      return WaitInstance(that);
    return this;
  }
  
  virtual void Draw() const {}
};


MPGoal * MPState::GoalInstance(MotionManager * that) {
  static auto_ptr<MPGoal> instance;
  if(instance.get() == 0) instance = auto_ptr<MPGoal>(new MPGoal(that));
  return instance.get();
}


MPState * MPState::WaitInstance(MotionManager * that) {
  static auto_ptr<MPWait> instance;
  if(instance.get() == 0) instance = auto_ptr<MPWait>(new MPWait(that));
  return instance.get();
}


MPState * MPState::ManualInstance(MotionManager * that) {
  static auto_ptr<MPManual> instance;
  if(instance.get() == 0) instance = auto_ptr<MPManual>(new MPManual(that));
  return instance.get();
}


MPUnblock * MPState::UnblockInstance(MotionManager * that) {
  static auto_ptr<MPUnblock> instance;
  if(instance.get() == 0) instance = auto_ptr<MPUnblock>(new MPUnblock(that));
  return instance.get();
}


MotionManager::
MotionManager(Odometry & odometry, FModIPDCMOT & left, FModIPDCMOT & right,
	      double sdmax):
  _sdmax(sdmax),
  _thetadmax(2 * sdmax / odometry._wheelbase),
  _qdmax(sdmax / odometry._wheelradius),
  _state(MPState::WaitInstance(this)),
  _odometry(odometry),
  _left(left),
  _right(right),
  _goal_dr(0.1),
  _goal_dtheta(M_PI)
{
}


void MotionManager::
Update()
{
  _state = _state->Do();
}


MPState * MotionManager::
Wait()
{
  DoSetQd(0, 0);
  _state = MPState::WaitInstance(this);
  return _state;
}


MPState * MotionManager::
InitGoalState()
{
  const Frame pose(_odometry.GetCurrentPose());
  const double dx(_goal_x - pose.X());
  const double dy(_goal_y - pose.Y());
  const double dist(sqrt(sqr(dx) + sqr(dy)));
  if(dist <= _goal_dr){
    if(absval(mod2pi(_goal_theta - pose.Theta())) <= _goal_dtheta)
      MPState::GoalInstance(this)->_mode = MPGoal::ATGOAL;
    else
      MPState::GoalInstance(this)->_mode = MPGoal::ADJUSTING;
  }
  else{
    const double foo(absval(mod2pi(atan2(dy, dx) - pose.Theta())));
    if((foo <= MPGoal::_dtheta_home) || (foo >= M_PI - MPGoal::_dtheta_home))
      MPState::GoalInstance(this)->_mode = MPGoal::HOMING;
    else
      MPState::GoalInstance(this)->_mode = MPGoal::AIMING;
  }
  return MPState::GoalInstance(this);
}


MPState * MotionManager::
SetGoal(double x, double y, double theta)
{
  if((_state == MPState::GoalInstance(this))
     && (_goal_x == x) && (_goal_y == y) && (_goal_theta == theta))
    return _state;
  
  _goal_x = x;
  _goal_y = y;
  _goal_theta = theta;
  _state = InitGoalState();
  return _state;
}


MPState * MotionManager::
SetGoalPrecision(double dr, double dtheta)
{
  if((_state == MPState::GoalInstance(this))
     && (_goal_dr == dr) && (_goal_dtheta == dtheta))
    return _state;
  
  _goal_dr = dr;
  _goal_dtheta = dtheta;
  _state = InitGoalState();
  return _state;
}


MPState * MotionManager::
SetGoalAndPrecision(double x, double y, double theta, double dr, double dtheta)
{
  if((_state == MPState::GoalInstance(this))
     && (_goal_x == x) && (_goal_y == y) && (_goal_theta == theta)
     && (_goal_dr == dr) && (_goal_dtheta == dtheta))
    return _state;
  
  _goal_x = x;
  _goal_y = y;
  _goal_theta = theta;
  _goal_dr = dr;
  _goal_dtheta = dtheta;
  _state = InitGoalState();
  return _state;//->Do();
}


void MotionManager::
DoSetQd(double qdl, double qdr)
{
  qdl = maxval(minval(qdl, _qdmax), - _qdmax);
  qdr = maxval(minval(qdr, _qdmax), - _qdmax);
  
  int32_t vleft;
  int32_t vright;
  Odometry::Rad2Enc(qdl, qdr, vleft, vright);
  _left.SetSpeed(vleft);
  _right.SetSpeed(vright);
  
  if(vleft == - vright){
    if(vleft != 0){
      const bool fwds(qdl > 0);
      if((_anchor.IsValid() && (fwds != _anchor.GoingForwards()))
	 || ( ! _anchor.IsValid()))
	_anchor.Set(GetPose(), fwds, Timestamp::Now());
    }
  }
  else
    _anchor.Invalidate();
}


MPState * MotionManager::
SetQd(double qdl, double qdr)
{
  DoSetQd(qdl, qdr);
  _state = MPState::ManualInstance(this);
  return _state;
}


void MotionManager::
DoSetSpeed(double sd, double thetad)
{
  sd = maxval(minval(sd, _sdmax), - _sdmax);
  thetad = maxval(minval(thetad, _thetadmax), - _thetadmax);
  double qdl, qdr;
  Global2Actuator(sd, thetad, qdl, qdr);
  DoSetQd(qdl, qdr);
}


MPState * MotionManager::
SetSpeed(double sd, double thetad)
{
  DoSetSpeed(sd, thetad);
  _state = MPState::ManualInstance(this);
  return _state;
}


void MotionManager::
Global2Actuator(double sd, double thetad,
		double & qdl, double & qdr) const
{
  thetad *= 0.5 * _odometry._wheelbase; 
  qdr = (sd + thetad) / _odometry._wheelradius;
  qdl = (sd - thetad) / _odometry._wheelradius;
}


void MotionManager::
Actuator2Global(double qdl, double qdr,
		double & sd, double & thetad) const
{
  qdl *= _odometry._wheelradius;
  qdr *= _odometry._wheelradius;
  sd = 0.5 * (qdl + qdr);
  thetad = (qdr - qdl) / _odometry._wheelbase;
}


void MotionManager::
Draw() const
{
  if(_anchor.IsValid()){
    glColor3d(0.5, 0.5, 1);
    glLineWidth(3);
    _odometry.DrawFrame(_anchor.GetPose());
    glLineWidth(1);
  }
  _state->Draw();
}


const sfl::Frame & MotionManager::
GetPose() const
{
  return _odometry.GetCurrentPose();
}


const MotionManager::Anchor & MotionManager::
GetAnchor() const
{
  return _anchor;
}


void MotionManager::
InvalidateAnchor()
{
  _anchor.Invalidate();
}


MPState * MotionManager::
UnblockMotors()
{
  if(_state == MPState::UnblockInstance(this))
    return _state;
  
  double qdl(0);
  if(     _left.GetCommand() >=   FModIPDCMOT::max_command) qdl = -0.8;
  else if(_left.GetCommand() <= - FModIPDCMOT::max_command) qdl =  0.8;
  double qdr(0);
  if(     _right.GetCommand() >=   FModIPDCMOT::max_command) qdr = -0.8;
  else if(_right.GetCommand() <= - FModIPDCMOT::max_command) qdr =  0.8;
  DoSetSpeed(qdl, qdr);
  
  MPState::UnblockInstance(this)->_tout =
    Timestamp::Now() + Timestamp(1, 500000);
  _state = MPState::UnblockInstance(this);
  return _state;
}


double MotionManager::
GetSdMax() const
{
  return _sdmax;
}


double MotionManager::
GetThetadMax() const
{
  return _thetadmax;
}


double MotionManager::
GetQdMax() const
{
  return _qdmax;
}
