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


#ifndef MOTION_MANAGER_HPP
#define MOTION_MANAGER_HPP


#include <aci/Timestamp.hpp>
#include <sfl/Frame.hpp>
#include <iosfwd>
#include <memory>


class Odometry;
class FModIPDCMOT;
class MPState;


class MotionManager
{
public:
  class Anchor {
  public:
    Anchor(): _stamp(Timestamp::Last()) {}
    void Set(const sfl::Frame & pose, bool forwards,
	     const Timestamp & stamp) {
      _pose = pose;
      _forwards = forwards;
      _stamp = stamp;
    }
    void Invalidate() { _stamp = Timestamp::Last(); }
    bool IsValid() const { return _stamp != Timestamp::Last(); }
    bool GoingForwards() const { return _forwards; }
    const sfl::Frame & GetPose() const { return _pose; }
    const Timestamp & GetStamp() const { return _stamp; }
  private:
    sfl::Frame _pose;
    bool _forwards;
    Timestamp _stamp;
  };
  
  
  MotionManager(Odometry & odometry, FModIPDCMOT & left, FModIPDCMOT & right,
		double sdmax);

  
  void Update();
  void Draw() const;
  
  MPState * Wait();
  MPState * SetGoal(double x, double y, double theta);
  MPState * SetGoalAndPrecision(double x, double y, double theta,
				double dr, double dtheta);
  MPState * SetGoalPrecision(double dr, double dtheta);
  MPState * SetQd(double qdl, double qdr);
  MPState * SetSpeed(double sd, double thetad);
  MPState * UnblockMotors();
  
  double GetSdMax() const;
  double GetThetadMax() const;
  double GetQdMax() const;

  const sfl::Frame & GetPose() const;
  const Anchor & GetAnchor() const;
  void InvalidateAnchor();
  
  void Global2Actuator(double sd, double thetad,
		       double & qdl, double & qdr) const;
  void Actuator2Global(double qdl, double qdr,
		       double & sd, double & thetad) const;
  
private:
  friend class MPGoal;
  friend class MPWait;
  
  
  void DoSetQd(double qdl, double qdr);
  void DoSetSpeed(double sd, double thetad);
  MPState * InitGoalState();
  
  
  const double _sdmax;
  const double _thetadmax;
  const double _qdmax;
  
  MPState * _state;
  Odometry & _odometry;
  FModIPDCMOT & _left;
  FModIPDCMOT & _right;
  double _goal_x, _goal_y, _goal_theta, _goal_dr, _goal_dtheta;
  Anchor _anchor;
};

#endif // MOTION_MANAGER_HPP
