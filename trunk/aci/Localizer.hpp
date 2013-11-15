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


#ifndef LOCALIZER_HPP
#define LOCALIZER_HPP


#include <aci/Timestamp.hpp>
#include <aci/Scanalyzer.hpp>


class Odometry;
class MotionManager;

namespace sfl {
  class Frame;
}


class Localizer
{
public:
  Localizer(Odometry & odometry,
	    MotionManager & motion_manager,
	    Scanalyzer & scanalyzer);
  
  void Update();

  const Scanalysis & GetScanalysis() const;
  int GetRobotLabel() const;
  const Timestamp & GetTMatch() const;

  void Draw() const;
  void DrawPrediction(const sfl::Frame & pose) const;

  bool SaveScan(const std::string & fname) const;
  bool SaveBackground(const std::string & fname) const;
  bool LoadBackground(const std::string & fname);
  
  // refactoring for Cactus
  const CircleLSQ * FindBest() {
    UpdateScan();
    return _FindBest();
  }
  
private:
  const CircleLSQ * _FindBest();
  const CircleLSQ * _FindRobot(double extract_thresh);

  bool UpdateScan();
  bool UpdateMatch();
  void RelabelRobot();

  const double _cactus_radius;// = 0.14;
  const double _dr_thresh;// = 0.02;
  const double _deadzone;// = 0.5;

  Odometry & _odometry;
  MotionManager & _motion_manager;
  Scanalyzer & _scanalyzer;
  
  Scanalysis _scanalysis;
  const CircleLSQ * _robot_circle;
  Timestamp _sick_success;
};

#endif // LOCALIZER_HPP
