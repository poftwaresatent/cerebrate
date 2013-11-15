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


#ifndef FMOD_TCP_H
#define FMOD_TCP_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include <drivers/fmod_util.h>
  
  
  /** iodir 0 means output, 1 means input */
  int fmod_tcp_riodir(struct fmod_s * s, int ionum, uint8_t * iodir,
		      FILE * dbg);
  
  /** iodir 0 means output, 1 means input */
  int fmod_tcp_wiodir(struct fmod_s * s, int ionum, uint8_t iodir,
		      FILE * dbg);
  int fmod_tcp_rio(struct fmod_s * s, int ionum, uint8_t * io,
		   FILE * dbg);
  int fmod_tcp_wio(struct fmod_s * s, int ionum, uint8_t io,
		   FILE * dbg);
  int fmod_tcp_rad(struct fmod_s * s, int adnum, uint16_t * adval,
		   FILE * dbg);

  
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // FMOD_TCP_H
