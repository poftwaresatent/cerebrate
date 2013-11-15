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


#ifndef CACTUS_HPP
#define CACTUS_HPP


#include <aci/Timestamp.hpp>
#include <vector>
#include <memory>


class FModIPDCMOT;
class FModTCP;
class MotionManager;
class Odometry;
class Scanalyzer;
class Localizer;
class Behavior;
class Watchdog;
class Mousehandler;
class Effects;
class Viewport;


class Cactus
{
public:
  typedef enum { MANUAL_SPEED, MANUAL_GOAL, MANUAL_TARGET, AUTO } state_t;
  
  Cactus();
  ~Cactus();
  
  void Update();

  bool InitOdometryXY(double & anchor_x, double & anchor_y,
		      std::ostream * dbg);
  bool InitOdometryTheta(double anchor_x, double anchor_y, std::ostream * dbg);
  bool AutoLocalize(std::ostream * dbg);
  
  void SetState(state_t state);
  void SetTarget(double x, double y);
  void SetGoal(double x, double y, double theta);
  void SetGoalPrecision(double dr, double dtheta);
  void SetQd(double qdl, double qdr);
  void SetSpeed(double sd, double thetad);
  void SetEnableAutoLocalize(bool enable);

  bool _SetShoulder(bool on, std::ostream * dbg);
  bool _SetEar(bool on, std::ostream * dbg);
  bool _SetLight(int num, bool on, std::ostream * dbg);
  
  void GetPose(double & x, double & y, double & theta) const;
  Mousehandler * GetMouseGoal();
  Mousehandler * GetMouseTarget();
  Mousehandler * GetMouseAuto();
  
  /** \return false if the program should exit */
  bool Command(std::istream & is, std::ostream & os, std::ostream * dbg);

  void Draw() const;
  void DrawStatus() const;
  void DrawEffects() const;
  void ConfigureStatusViewport(Viewport & vp) const;
  void ConfigureEffectsViewport(Viewport & vp) const;
  void ConfigureArenaViewport(Viewport & vp) const;
  
  
private:
  void UpdateWatchdog();

  static const double WHEELBASE = 0.345;
  static const double WHEELRADIUS = 0.088;
  
  state_t _state;
  bool _enable_auto_localize;
  double _fake_target_x, _fake_target_y;
  
  std::auto_ptr<FModIPDCMOT> _left, _right;
  std::auto_ptr<FModTCP> _io;
  std::auto_ptr<MotionManager> _motion_manager; 
  std::auto_ptr<Odometry> _odometry;
  std::auto_ptr<Scanalyzer> _scanalyzer;
  std::auto_ptr<Localizer> _localizer;
  std::auto_ptr<Behavior> _behavior;
  std::auto_ptr<Watchdog> _watchdog;
  std::auto_ptr<Mousehandler> _mousegoal;
  std::auto_ptr<Mousehandler> _mousetarget;
  std::auto_ptr<Mousehandler> _mouseauto;
  std::auto_ptr<Effects> _effects;
};

#endif // CACTUS_HPP
