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


#ifndef ODOMETRY_HPP
#define ODOMETRY_HPP


#include <util/Timestamp.hpp>
#include <sfl/Frame.hpp>
#include <map>
#include <stdint.h>


class FModIPDCMOT;
class MotionManager;


class Odometry
{
public:
  class posechange {
  public:
    posechange(const Timestamp & _stamp):
      stamp(_stamp), is_match(false) {}
    posechange(const Timestamp & _stamp,
	       const sfl::Frame & _delta,
	       const sfl::Frame & _global,
	       bool _is_match):
      stamp(_stamp), delta(_delta), global(_global), is_match(_is_match) {}
    const Timestamp stamp;
    const sfl::Frame delta;
    sfl::Frame global;
    const bool is_match;
  };
  
  class observation {
  public:
    observation(//const Timestamp & _stamp,
		double _x, double _y,
		bool _known_theta, double _theta):
      //stamp(_stamp),
      x(_x), y(_y), known_theta(_known_theta), theta(_theta) {}
    //const Timestamp stamp;
    const double x;
    const double y;
    const bool known_theta;
    const double theta;
  };
  
  
  Odometry(FModIPDCMOT & left, FModIPDCMOT & right,
	   double wheelbase, double wheelradius);
  
  
  void Init(const Timestamp & t, double x, double y, double theta,
	    MotionManager * mm);
  void Update();
  void Correct(const Timestamp & t, const observation & observation);
  
  /**
     \return Reference of current (most recent) pose in world frame.
  */
  const sfl::Frame & GetCurrentPose() const;
  
  /**
     \return Reference of current (most recent) pose change.
  */
  const posechange & GetCurrent() const;
  
  /**
     \return Reference of most recent matched pose change.
  */
  const posechange & GetMatched() const;
  const sfl::Frame & GetMatchedPose() const;

  void Draw() const;
  void DrawFrame(const sfl::Frame & frame) const;
  
  static void Rad2Enc(double ql_rad, double qr_rad,
		      int32_t & ql_enc, int32_t & qr_enc);
  static void Enc2Rad(int32_t ql_enc, int32_t qr_enc,
		      double & ql_rad, double & qr_rad);


  const double _wheelbase;
  const double _wheelradius;
  
private:
  typedef std::map<Timestamp, posechange, Timestamp::less> history;
  typedef history::iterator ichange;

  void AppendPose(const Timestamp & t, double dx, double dy, double dtheta);
  static ichange FindClosest(const Timestamp & t, history & hist);
  
  history _ancient_history;
  history _recent_history;
  
  FModIPDCMOT & _left;
  FModIPDCMOT & _right;
  int32_t _ql, _qr;
};

#endif // ODOMETRY_HPP
