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


#include "fmod_tcp.h"


int fmod_tcp_riodir(struct fmod_s * s,
		    int ionum,
		    uint8_t * iodir,
		    FILE * dbg)
{
  if(0 != dbg)
    fprintf(dbg, "DEBUG fmod_tcp_riodir()\n");

  if(ionum == 0)
    return fmod_rreg(s, 0x1234, 0x20, iodir, 1, dbg);
  if(ionum == 1)
    return fmod_rreg(s, 0x1234, 0x27, iodir, 1, dbg);
  if(ionum == 2)
    return fmod_rreg(s, 0x1234, 0x29, iodir, 1, dbg);
  return FMOD_EPARAM;
}


int fmod_tcp_wiodir(struct fmod_s * s,
		    int ionum,
		    uint8_t iodir,
		    FILE * dbg)
{
  if(0 != dbg)
    fprintf(dbg, "DEBUG fmod_tcp_wiodir()\n");

  if(ionum == 0)
    return fmod_wreg(s, 0x1234, 0x20, & iodir, 1, dbg);
  if(ionum == 1)
    return fmod_wreg(s, 0x1234, 0x27, & iodir, 1, dbg);
  if(ionum == 2)
    return fmod_wreg(s, 0x1234, 0x29, & iodir, 1, dbg);
  return FMOD_EPARAM;
}

  
int fmod_tcp_rio(struct fmod_s * s,
		 int ionum,
		 uint8_t * io,
		 FILE * dbg)
{
  if(0 != dbg)
    fprintf(dbg, "DEBUG fmod_tcp_rio()\n");

  if(ionum == 0)
    return fmod_rreg(s, 0x1234, 0x21, io, 1, dbg);
  if(ionum == 1)
    return fmod_rreg(s, 0x1234, 0x28, io, 1, dbg);
  if(ionum == 2)
    return fmod_rreg(s, 0x1234, 0x2A, io, 1, dbg);
  return FMOD_EPARAM;
}


int fmod_tcp_wio(struct fmod_s * s,
		 int ionum,
		 uint8_t io,
		 FILE * dbg)
{
  if(0 != dbg)
    fprintf(dbg, "DEBUG fmod_tcp_wio()\n");

  if(ionum == 0)
    return fmod_wreg(s, 0x1234, 0x21, & io, 1, dbg);
  if(ionum == 1)
    return fmod_wreg(s, 0x1234, 0x28, & io, 1, dbg);
  if(ionum == 2)
    return fmod_wreg(s, 0x1234, 0x2A, & io, 1, dbg);
  return FMOD_EPARAM;  
}


int fmod_tcp_rad(struct fmod_s * s,
		 int adnum,
		 uint16_t * adval,
		 FILE * dbg)
{
  uint8_t val[2] = {0, 0};
  int result = FMOD_EPARAM;
  
  if(0 != dbg)
    fprintf(dbg, "DEBUG fmod_tcp_rad()\n");

  if(adnum == 0)
    result = fmod_rreg(s, 0x1234, 0x22, val, 2, dbg);
  if(adnum == 1)
    result = fmod_rreg(s, 0x1234, 0x23, val, 2, dbg);
  if(adnum == 2)
    result = fmod_rreg(s, 0x1234, 0x24, val, 2, dbg);
  if(adnum == 3)
    result = fmod_rreg(s, 0x1234, 0x25, val, 2, dbg);
  if(adnum == 4)
    result = fmod_rreg(s, 0x1234, 0x26, val, 2, dbg);
  
  * adval = (val[0] << 8) | val[1];
  return result;
}
