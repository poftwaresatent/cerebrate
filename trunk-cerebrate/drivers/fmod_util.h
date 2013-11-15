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


#ifndef FMOD_UTIL_H
#define FMOD_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include <stdint.h>
#include <stdio.h>
#include <pthread.h>


/** success */
#define FMOD_OK        0

/** error: function not implemented */
#define FMOD_EIMP     -1

/** error: syscall failed (e.g. read() or write()) */
#define FMOD_ESYS     -2

/** error: parameter out of range or mismatch */
#define FMOD_EPARAM   -3

/** error: checksum mismatch */
#define FMOD_ECRC     -4

/** error: mutex lock or unlock failed */
#define FMOD_ELOCK    -5
  
  
  struct fmod_s {
    int fd;
    pthread_mutex_t mutex;
  };
  
  
  struct fmod_s * fmod_new(int fd);
  void fmod_delete(struct fmod_s * s);
  
  const char * fmod_errstr(int error);
  int32_t fmod_f2i(double k);
  double fmod_i2f(int32_t k);
  
  int fmod_rreg(struct fmod_s * s, uint16_t trid, uint8_t reg,
		uint8_t * val, uint16_t len, FILE * dbg);

  int fmod_wreg(struct fmod_s * s, uint16_t trid, uint8_t reg,
		const uint8_t * val, uint16_t len, FILE * dbg);

  int fmod_rreg32(struct fmod_s * s, uint16_t trid, uint8_t reg,
		  int32_t * val, FILE * dbg);
  
  int fmod_wreg32(struct fmod_s * s, uint16_t trid, uint8_t reg,
		  const int32_t val, FILE * dbg);

  void fmod_crc(uint8_t * packet, uint16_t len, uint16_t * crc, FILE * dbg);
  int fmod_ckcrc(uint8_t * packet, uint16_t len, FILE * dbg);
  
  int fmod_saveusr(struct fmod_s * s);
  int fmod_restoreusr(struct fmod_s * s);
  int fmod_restorefct(struct fmod_s * s);

  int fmod_rmac(struct fmod_s * s, uint8_t * arr6);
  int fmod_ripaddr(struct fmod_s * s, uint8_t * arr4, FILE * dbg);
  int fmod_wipaddr(struct fmod_s * s, const uint8_t * arr4, FILE * dbg);
  int fmod_rnetmask(struct fmod_s * s, uint8_t * arr4);
  int fmod_wnetmask(struct fmod_s * s, const uint8_t * arr4);
  int fmod_rtcptout(struct fmod_s * s, uint8_t * seconds);
  int fmod_wtcptout(struct fmod_s * s, const uint8_t seconds);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // FMOD_UTIL_H
