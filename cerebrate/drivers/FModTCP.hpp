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


#ifndef FMOD_TCP_HPP
#define FMOD_TCP_HPP


#include <stdint.h>
#include <stdio.h>


class FModTCP
{
protected:
  FModTCP(struct fmod_s * fs, unsigned int usec_cycle,
	  unsigned int max_errcount);
  
public:
  ~FModTCP();
  
  static FModTCP * Create(uint32_t portnum, const char * server,
			  unsigned int usec_cycle, unsigned int max_errcount);
  
  bool SetBit(int num, bool on);
  bool SetState(uint8_t state);
  
  uint8_t GetState() const;
  bool GetOk() const;
  bool GetRunning() const;

private:
  void StartThread();
  void StopThread();
  
  struct FModTCP_wrap_s * _wrap;
  uint8_t _iostate;
  FILE * _dbg;
};

#endif // FMOD_TCP_HPP
