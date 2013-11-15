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


#ifndef FMOD_IPDCMOT_HPP
#define FMOD_IPDCMOT_HPP


#include <stdint.h>


class FModIPDCMOT
{
protected:
  FModIPDCMOT(struct fmod_s * fs, unsigned int usec_cycle,
	      int32_t speed_increment);
  
public:
  static const int32_t max_command = 0x00007FFF;

  ~FModIPDCMOT();
  
  static FModIPDCMOT * Create(uint32_t portnum, const char * server,
			      unsigned int usec_cycle,
			      double kp, double ki, double kd,
			      int32_t tspeed, double imax,
			      int32_t acc);
  
  void ForceSpeed(int32_t speed);
  void SetSpeed(int32_t speed);
  
  bool GetOk() const;
  bool GetRunning() const;
  int32_t GetRealSpeed() const;
  int32_t GetWantedSpeed() const;
  int32_t GetPosition() const;
  int32_t GetCommand() const;

  bool UpdateCurrentWantedSpeed();
  bool UpdateRealSpeed();
  bool UpdatePosition();
  bool UpdateCommand();

private:
  void StartThread();
  void StopThread();
  
  
  struct FModIPDCMOT_wrap_s * _wrap;
  int32_t _real_speed;
  int32_t _final_wanted_speed;
  int32_t _current_wanted_speed;
  int32_t _position;
  int32_t _command;
  int32_t _speed_increment;
};

#endif // FMOD_IPDCMOT_HPP
