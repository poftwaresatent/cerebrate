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


#include "FModTCP.hpp"
#include <drivers/fmod_util.h>
#include <drivers/fmod_tcp.h>
#include <drivers/util.h>
#include <pthread.h>
#include <errno.h>
#include <iostream>


struct FModTCP_wrap_s {
  FModTCP * that;
  struct fmod_s * fs;
  unsigned int usec_cycle;
  unsigned int errcount, max_errcount;
  bool running;
  bool stop;
  bool ok;
  pthread_t * thread;
  FILE * dbg;
};


FModTCP::
FModTCP(struct fmod_s * fs, unsigned int usec_cycle, unsigned int max_errcount)
{
  _wrap = new FModTCP_wrap_s();
  _wrap->that = this;
  _wrap->fs = fs;
  _wrap->usec_cycle = usec_cycle;
  _wrap->max_errcount = max_errcount;
  _wrap->running = false;
  _wrap->stop = false;
  _wrap->ok = false;
  _wrap->thread = new pthread_t();
  _wrap->dbg = _dbg;

  fmod_tcp_wio(_wrap->fs, 0, 0, 0);

  StartThread();
}


FModTCP::
~FModTCP()
{
  fmod_tcp_wio(_wrap->fs, 0, 0, 0);
  
  StopThread();
  
  tcp_close(_wrap->fs->fd);
  fmod_delete(_wrap->fs);
  delete _wrap->thread;
  delete _wrap;
}


FModTCP * FModTCP::
Create(uint32_t portnum, const char * server,
       unsigned int usec_cycle, unsigned int max_errcount)
{
  int fd(tcp_open(8010, server));
  if(fd < 0)
    return 0;
  
  struct fmod_s * fs(fmod_new(fd));
  int result(fmod_tcp_wiodir(fs, 0, 0, 0));
  if(FMOD_OK != result){
    fmod_delete(fs);
    tcp_close(fd);
    return 0;
  }
  
  return new FModTCP(fs, usec_cycle, max_errcount);
}


bool FModTCP::
SetBit(int num, bool on)
{
  uint8_t state(_iostate);
  if(on) state |= (1 << num);
  else   state &= (1 << num) ^ 0xFF;
  return SetState(state);
}


bool FModTCP::
SetState(uint8_t state)
{
  const bool ok(FMOD_OK == fmod_tcp_wio(_wrap->fs, 0, state, _dbg));
  if(!ok)
    return false;
  _iostate = state;
  return true;
}


uint8_t FModTCP::
GetState()
  const
{
  return _iostate;
}


bool FModTCP::
GetOk()
  const
{
  return _wrap->ok;
}


bool FModTCP::
GetRunning()
  const
{
  return _wrap->running;
}


extern "C" {
  static void * thread_run(struct FModTCP_wrap_s * wrap)
  {
    wrap->errcount = 0;
    wrap->stop = false;
    while( ! wrap->stop){
      uint8_t io;
      if(FMOD_OK != fmod_tcp_rio(wrap->fs, 0, & io, wrap->dbg))
	wrap->ok = false;
      else
	wrap->ok = true;
      if(io != wrap->that->GetState()){
	if(FMOD_OK != fmod_tcp_wio(wrap->fs, 0, wrap->that->GetState(),
				   wrap->dbg))
	  ++wrap->errcount;
	else
	  wrap->errcount = 0;	
	if(wrap->errcount > wrap->max_errcount)
	  wrap->ok = false;
      }
      usleep(wrap->usec_cycle);
    }
    wrap->stop = false;
    return wrap;
  }
}


void FModTCP::
StartThread()
{
  if(0 == pthread_create(_wrap->thread, 0,
			 (void*(*)(void*)) thread_run, _wrap))
    _wrap->running = true;
}


void FModTCP::
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
