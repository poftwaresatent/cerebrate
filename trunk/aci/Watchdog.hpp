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


#ifndef WATCHDOG_HPP
#define WATCHDOG_HPP


#include <aci/Timeout.hpp>
#include <stdint.h>


class Scanalyzer;
class FModIPDCMOT;
class Localizer;
class Viewport;


class Watchdog
{
public:
  typedef enum { OK, ILL, RECOVERING, DEAD } state_t;
  
  
  Watchdog(const Localizer & localizer,
	   const Timestamp & localizer_timeout,
	   const Scanalyzer & scanalyzer,
	   const Timestamp & sick_ok_timeout,
	   const Timestamp & sick_recover_timeout,
	   FModIPDCMOT & left, FModIPDCMOT & right//,
	   //double slowdown_threshold
	   );
  
  void Update();
  
  state_t GetSickState() const;
  state_t GetLocalizerState() const;
  void RestartSick();
  bool MotorsOk() const;
  
  void Draw() const;
  void ConfigureViewport(Viewport & vp) const;
  
  
private:
  const Localizer & _localizer;
  Timeout _localizer_timeout;
  state_t _localizer_state;
  const Scanalyzer & _scanalyzer;
  Timeout _sick_ok_timeout;
  Timeout _sick_recover_timeout;
  state_t _sick_state;
  FModIPDCMOT & _left;
  FModIPDCMOT & _right;
  int32_t _leftcom, _rightcom;
  bool _motors_ok;
};

#endif // WATCHDOG_HPP
