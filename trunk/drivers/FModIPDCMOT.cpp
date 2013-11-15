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


#include "FModIPDCMOT.hpp"
#include <drivers/fmod_util.h>
#include <drivers/fmod_ipdcmot.h>
#include <drivers/util.h>
#include <sfl/numeric.hpp>
#include <pthread.h>
#include <errno.h>
#include <iostream>		// dbg


struct FModIPDCMOT_wrap_s {
  FModIPDCMOT * that;
  struct fmod_s * fs;
  unsigned int usec_cycle;
  bool running;
  bool stop;
  bool ok;
  pthread_t * thread;
};


FModIPDCMOT::
FModIPDCMOT(struct fmod_s * fs, unsigned int usec_cycle,
	    int32_t speed_increment):
  _speed_increment(speed_increment)
{
  _wrap = new FModIPDCMOT_wrap_s();
  _wrap->that = this;
  _wrap->fs = fs;
  _wrap->usec_cycle = usec_cycle;
  _wrap->running = false;
  _wrap->stop = false;
  _wrap->ok = false;
  _wrap->thread = new pthread_t();

  ForceSpeed(0);
  StartThread();
}


FModIPDCMOT::
~FModIPDCMOT()
{
  fmod_ipdcmot_wmode(_wrap->fs, FMOD_IPDCMOT_BRAKE);

  StopThread();
  
  tcp_close(_wrap->fs->fd);
  fmod_delete(_wrap->fs);
  delete _wrap->thread;
  delete _wrap;
}


FModIPDCMOT * FModIPDCMOT::
Create(uint32_t portnum, const char * server,
       unsigned int usec_cycle,
       double kp, double ki, double kd,
       int32_t tspeed, double imax,
       int32_t acc)
{
  int fd(tcp_open(8010, server));
  if(fd < 0)
    return 0;
  
  struct fmod_s * fs(fmod_new(fd));
  int result(fmod_ipdcmot_confspeed(fs, kp, ki, kd, tspeed, imax, 0));
  if(FMOD_OK != result){
    fmod_delete(fs);
    tcp_close(fd);
    return 0;
  }
  
  int32_t speed_inc((int32_t) rint(acc * 1000000.0 / usec_cycle));
  if(0 == speed_inc)
    speed_inc = 1;
  if(0 > speed_inc)
    speed_inc = - speed_inc;
  return new FModIPDCMOT(fs, usec_cycle, speed_inc);
}


void FModIPDCMOT::
SetSpeed(int32_t speed)
{
  _final_wanted_speed = speed;
}


void FModIPDCMOT::
ForceSpeed(int32_t speed)
{
  _final_wanted_speed = speed;
  _current_wanted_speed = speed - 1;
}


int32_t FModIPDCMOT::
GetRealSpeed()
  const
{
  return _real_speed;
}


int32_t FModIPDCMOT::
GetWantedSpeed()
  const
{
  return _final_wanted_speed;
}


int32_t FModIPDCMOT::
GetPosition()
  const
{
  return _position;
}


int32_t FModIPDCMOT::
GetCommand()
  const
{
  return _command;
}


bool FModIPDCMOT::
UpdateRealSpeed()
{
  return (FMOD_OK == fmod_ipdcmot_rspeed(_wrap->fs, & _real_speed));
}


bool FModIPDCMOT::
UpdatePosition()
{
  return (FMOD_OK == fmod_ipdcmot_rpos(_wrap->fs, & _position));
}


bool FModIPDCMOT::
UpdateCommand()
{
  return (FMOD_OK == fmod_ipdcmot_rcom(_wrap->fs, & _command));
}


bool FModIPDCMOT::
GetOk()
  const
{
  return _wrap->ok;
}


bool FModIPDCMOT::
GetRunning()
  const
{
  return _wrap->running;
}


extern "C" {
  static void * thread_run(struct FModIPDCMOT_wrap_s * wrap)
  {
    wrap->stop = false;
    while( ! wrap->stop){
      wrap->ok = wrap->that->UpdateCurrentWantedSpeed();
      wrap->ok = wrap->that->UpdatePosition();
      wrap->ok = wrap->that->UpdateCommand();
      wrap->ok &= wrap->that->UpdateRealSpeed();
      usleep(wrap->usec_cycle);
    }
    wrap->stop = false;
    return wrap;
  }
}


void FModIPDCMOT::
StartThread()
{
  if(0 == pthread_create(_wrap->thread, 0,
			 (void*(*)(void*)) thread_run, _wrap))
    _wrap->running = true;
}


void FModIPDCMOT::
StopThread()
{
  _wrap->stop = true;
  usleep(100000);
  while(_wrap->stop){
    std::cerr << ".";
    usleep(100000);
  }
  _wrap->running = false;
  
  if(0 != pthread_join( * _wrap->thread, 0))
    std::cerr << "pthread_join() failed: " << strerror(errno) << "\n";
}


bool FModIPDCMOT::
UpdateCurrentWantedSpeed()
{
  if(_final_wanted_speed == _current_wanted_speed)
    return true;
  
  const int32_t delta(_final_wanted_speed - _current_wanted_speed);
  if(sfl::absval(delta) <= _speed_increment)
    _current_wanted_speed = _final_wanted_speed;
  else{
    if(delta > 0)
      _current_wanted_speed += _speed_increment;
    else
      _current_wanted_speed -= _speed_increment;
  }
  
  return FMOD_OK == fmod_ipdcmot_win(_wrap->fs, _current_wanted_speed);
}
