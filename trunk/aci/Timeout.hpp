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


#ifndef TIMEOUT_HPP
#define TIMEOUT_HPP


#include <util/Timestamp.hpp>


class Timeout
{
public:
  Timeout(const Timestamp & duration = Timestamp::Last());
  
  void UpdateRelative(const Timestamp & delta);
  void UpdateAbsolute();

  void Set(double seconds);
  inline const Timestamp & GetDelta() const;
  inline double GetDeltaSec() const;
  inline bool GetExpired() const;
  inline double GetFraction() const;

  void Draw(double x0, double x1) const;

private:
  Timestamp _duration;
  double _duration_sec;
  Timestamp _delta;
  double _delta_sec;
  bool _expired;
  double _fraction;
  Timestamp _absolute_t0;
};


const Timestamp & Timeout::
GetDelta() const
{
  return _delta;
}


double Timeout::
GetDeltaSec() const
{
  return _delta_sec;
}


bool Timeout::
GetExpired() const
{
  return _expired;
}


double Timeout::
GetFraction() const
{
  return _fraction;
}

#endif // TIMEOUT_HPP
