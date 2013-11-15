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


#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP


#include <aci/Timeout.hpp>


class Scanalysis;
class MotionManager;
class Effects;

namespace sfl {
  class Frame;
}


class Behavior
{
public:
  typedef enum { WAIT, AIM, ATTACK } mode;
  
  Behavior(double home_x, double home_y,
	   double green, double red);
  
  void Update(const Scanalysis & scanalysis, const sfl::Frame & pose,
	      double deadzone);
  void FakeUpdate(double target_x, double target_y);
  void Perform(const sfl::Frame & pose,
	       MotionManager & mm, Effects & effects);

  void DrawRegions() const;
  void DrawTarget() const;

  
private:
  void FindTarget(const Scanalysis & scanalysis, const sfl::Frame & pose,
		  double deadzone);
  void DoUpdate();
  void DoWait(MotionManager & mm, Effects & effects);
  void DoAim(const sfl::Frame & pose,
	     MotionManager & mm, Effects & effects);
  void DoAttack(MotionManager & mm, Effects & effects);
  
  const double _home_x;
  const double _home_y;
  const double _green;
  const double _red;
  mode _mode;
  double _potential_target_dist; // use -1 to signal "no target"
  double _potential_target_x;
  double _potential_target_y;
  double _active_target_dist;	// use -1 to signal "no target"
  double _active_target_x;
  double _active_target_y;
  Timeout _target_timeout;
};

#endif // BEHAVIOR_HPP
