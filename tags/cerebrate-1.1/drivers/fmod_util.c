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


#include "util.h"
#include "fmod_util.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <netinet/in.h>


//#define FMOD_UTIL_DEBUG_CRC
#undef FMOD_UTIL_DEBUG_CRC


void fmod_crc(uint8_t * packet,
	      uint16_t len,
	      uint16_t * crc,
	      FILE * dbg)
{
  uint32_t acc = 0;
  uint16_t crc0, crc1, i;
  
#ifndef FMOD_UTIL_DEBUG_CRC
  dbg = 0;
#endif // FMOD_UTIL_DEBUG_CRC
  
  if(dbg == 0){
    for(i = 0; i < len / 2; ++i)
      acc += ntohs(((uint16_t *) packet)[i]) ^ 0x0000FFFF;
    if(len % 2 != 0)
      acc += (packet[len - 1] << 8) ^ 0x0000FFFF;
    crc0 = acc >> 16;
    crc1 = acc & 0x0000FFFF;
    acc = crc0 + crc1;
    if(acc & 0xFFFF0000)
      ++acc;
    * crc = htons(acc & 0x0000FFFF);
  }
  else{
    fprintf(dbg, "DEBUG fmod_crc():\n  msg:\n    ");
    for(i = 0; i < len; ++i)
      fprintf(dbg, "%02X ", packet[i]);
    fprintf(dbg, "\n  acc:\n");
    
    for(i = 0; i < len / 2; ++i){
      crc0 = ntohs(((uint16_t *) packet)[i]);
      acc += crc0 ^ 0x0000FFFF;
      fprintf(dbg, "    0x%04X -> 0x%08X\n", crc0, acc);
    }
    if(len % 2 != 0){
      //crc0 = ntohs(packet[len - 1] << 8);
      crc0 = packet[len - 1] << 8;
      acc += crc0 ^ 0x0000FFFF;
      fprintf(dbg, "    0x%04X -> 0x%08X\n", crc0, acc);
    }
    
    crc0 = acc >> 16;
    crc1 = acc & 0x0000FFFF;
    acc = crc0 + crc1;
    fprintf(dbg, "  res:\n    0x%04X + 0x%04X = 0x%08X", crc0, crc1, acc);
    if(acc & 0xFFFF0000){
      ++acc;
      fprintf(dbg, " (carry)\n");
    }
    else
      fprintf(dbg, " (no carry)\n");
    
    * crc = htons(acc & 0x0000FFFF);
    fprintf(dbg, "    crc = 0x%04X\n", * crc);
  }
}


int fmod_ckcrc(uint8_t * packet,
	       uint16_t len,
	       FILE * dbg)
{
  uint16_t wanted_crc, received_crc;
  
  fmod_crc(packet, len, & wanted_crc, dbg);
  received_crc = ntohs((packet[len] << 8) | (packet[len + 1]));
  if(wanted_crc != received_crc){
    if(dbg != 0)
      fprintf(dbg,
	      "DEBUG fmod_ckcrc():\n"
	      "  wanted crc   = 0x%04X\n"
	      "  received crc  = 0x%04X\n",
	      wanted_crc, received_crc);
    return FMOD_ECRC;
  }

  return FMOD_OK;
}


const char * fmod_errstr(int error)
{
  static char * str[7] = {
    "FMOD_OK (success)",
    "FMOD_EIMP (function not implemented)",
    "FMOD_ESYS (syscall failed)",
    "FMOD_EPARAM (parameter mismatch)",
    "FMOD_ECRC (checksum mismatch)",
    "FMOD_ELOCK (pthread_mutex error)",
    "(invalid)"};
  
  if(error < -5)
    return str[6];
  return str[-error];
}


/**
   \todo Could also check READ_ANSWER, transaction ID, and number of
   bytes in answer.
*/
int fmod_rreg(struct fmod_s * s,
	      uint16_t trid,
	      uint8_t reg,
	      uint8_t * val,
	      uint16_t len,
	      FILE * dbg)
{
  uint8_t packet[9 + len];
  uint16_t crc;
  
  packet[0] = 0x00;
  packet[1] = 0x21;
  packet[2] = trid >> 8;
  packet[3] = trid & 0x00FF;
  packet[4] = 0x00;
  packet[5] = 0x01;
  packet[6] = reg;
  fmod_crc(packet, 7, & crc, dbg);
  memcpy(packet + 7, & crc, 2);
  
  //////////////////////////////////////////////////
  // START SYNC
  {
    int retval = FMOD_OK;
    if(0 != pthread_mutex_lock(& s->mutex))
      retval = FMOD_ELOCK;
    else if(0 != buffer_write(s->fd, packet, 9, dbg)){
      pthread_mutex_unlock(& s->mutex);
      retval = FMOD_ESYS;
    }
    else if(buffer_read(s->fd, packet, 9 + len, dbg) != 0){
      pthread_mutex_unlock(& s->mutex);
      retval = FMOD_ESYS;
    }
    if(0 != pthread_mutex_unlock(& s->mutex))
      retval = FMOD_ELOCK;
    if(FMOD_OK != retval)
      return retval;
  }
  // END SYNC
  //////////////////////////////////////////////////
  
//   fmod_crc(packet, 7 + len, & crc, dbg);
//   received_crc = ntohs((packet[7 + len] << 8) | (packet[8 + len]));
//   if(crc != received_crc){
//       {
// 	if(dbg != 0)
// 	  fprintf(dbg,
// 		  "DEBUG fmod_rreg():\n"
// 		  "  calculated crc = 0x%04X\n"
// 		  "  received crc   = 0x%04X\n",
// 		  crc, received_crc);
// 	return FMOD_ECRC;
//       }
//   }
  if(FMOD_OK != fmod_ckcrc(packet, len + 7, dbg))
    return FMOD_ECRC;
  
  if(len + 1 != ((packet[4] << 8) | packet[5])){
    if(dbg != 0)
      fprintf(dbg,
	      "DEBUG fmod_rreg():\n"
	      "  expected length = 0x%04X\n"
	      "  received length = 0x%04X\n",
	      len + 1, (packet[4] << 8) | packet[5]);
    return FMOD_EPARAM;
  }
  memcpy(val, packet + 7, len);
  
  return FMOD_OK;
}


/**
   \todo Could also check WRITE_ANSWER and transaction ID.
*/
int fmod_wreg(struct fmod_s * s,
	      uint16_t trid,
	      uint8_t reg,
	      const uint8_t * val,
	      uint16_t len,
	      FILE * dbg)
{
  uint8_t packet[9 + len];
  uint16_t crc;
  
  //////////////////////////////////////////////////
  // send write request
  
  packet[0] = 0x00;
  packet[1] = 0x22;
  packet[2] = trid >> 8;
  packet[3] = trid & 0x00FF;
  packet[4] = (1 + len) >> 8;
  packet[5] = (1 + len) & 0x00FF;
  packet[6] = reg;
  memcpy(packet + 7, val, len);	  // value
  fmod_crc(packet, 7 + len, & crc, dbg);
  memcpy(packet + 7 + len, & crc, 2);
  
  //////////////////////////////////////////////////
  // START SYNC
  {
    int retval = FMOD_OK;
    if(0 != pthread_mutex_lock(& s->mutex))
      retval = FMOD_ELOCK;
    else if(0 != buffer_write(s->fd, packet, 9 + len, dbg)){
      pthread_mutex_unlock(& s->mutex);
      retval = FMOD_ESYS;
    }
    else if(buffer_read(s->fd, packet, 8, dbg) != 0){
      pthread_mutex_unlock(& s->mutex);
      retval = FMOD_ESYS;
    }
    if(0 != pthread_mutex_unlock(& s->mutex))
      retval = FMOD_ELOCK;
    if(FMOD_OK != retval)
      return retval;
  }
  // END SYNC
  //////////////////////////////////////////////////
  
//   fmod_crc(packet, 6, & crc, dbg);
//   received_crc = ntohs((packet[6] << 8) | (packet[7]));
//   if(crc != received_crc){
//       {
// 	if(dbg != 0)
// 	  fprintf(dbg,
// 		  "DEBUG fmod_wreg():\n"
// 		  "  calculated crc = 0x%04X\n"
// 		  "  received crc   = 0x%04X\n",
// 		  crc, received_crc);
// 	return FMOD_ECRC;
//       }
//   }
  if(FMOD_OK != fmod_ckcrc(packet, 6, dbg))
    return FMOD_ECRC;
  
  return FMOD_OK;
}


int fmod_rmac(struct fmod_s * s,
	      uint8_t * arr6)
{
  return fmod_rreg(s, 0x1234, 0x11, arr6, 6, 0);
}


int fmod_ripaddr(struct fmod_s * s,
		 uint8_t * arr4,
		 FILE * dbg)
{
  return fmod_rreg(s, 0x1234, 0x12, arr4, 4, dbg);
}


int fmod_wipaddr(struct fmod_s * s,
		 const uint8_t * arr4,
		 FILE * dbg)
{
  return fmod_wreg(s, 0x1234, 0x12, arr4, 4, dbg);
}


int fmod_rnetmask(struct fmod_s * s,
		  uint8_t * arr4)
{
  return fmod_rreg(s, 0x1234, 0x13, arr4, 4, 0);
}


int fmod_wnetmask(struct fmod_s * s,
		  const uint8_t * arr4)
{
  return fmod_wreg(s, 0x1234, 0x13, arr4, 4, 0);
}


int fmod_rtcptout(struct fmod_s * s,
		  uint8_t * seconds)
{
  return fmod_rreg(s, 0x1234, 0x14, seconds, 1, 0);
}


int fmod_wtcptout(struct fmod_s * s,
		  uint8_t seconds)
{
  return fmod_wreg(s, 0x1234, 0x14, & seconds, 1, 0);
}


int fmod_saveusr(struct fmod_s * s)
{
  return fmod_wreg(s, 0x1234, 0x03, 0, 0, 0);
}


int fmod_restoreusr(struct fmod_s * s)
{
  return fmod_wreg(s, 0x1234, 0x04, 0, 0, 0);
}


int fmod_restorefct(struct fmod_s * s)
{
  return fmod_wreg(s, 0x1234, 0x05, 0, 0, 0);
}


int fmod_rreg32(struct fmod_s * s,
		uint16_t trid,
		uint8_t reg,
		int32_t * val,
		FILE * dbg)
{
  uint8_t val8[4];
  int result = fmod_rreg(s, trid, reg, val8, 4, dbg);
  if(result != FMOD_OK)
    return result;

  * val
    = (((uint32_t) val8[0]) << 24)
    | (((uint32_t) val8[1]) << 16)
    | (((uint32_t) val8[2]) <<  8)
    |  ((uint32_t) val8[3]);
  
  if(dbg != 0)
    fprintf(dbg,
	    "DEBUG fmod_rreg32():\n"
	    "  val8[] = 0x%02X%02X%02X%02X\n"
	    "         = 0x%08X\n"
	    "         + 0x%08X\n"
	    "         + 0x%08X\n"
	    "         + 0x%08X\n"
	    "  val    = 0x%08X\n",
	    val8[0], val8[1], val8[2], val8[3],
	    ((uint32_t) val8[0]) << 24,
	    ((uint32_t) val8[1]) << 16,
	    ((uint32_t) val8[2]) <<  8,
	    ((uint32_t) val8[3]),
	    * val);
  
  return FMOD_OK;
}


int fmod_wreg32(struct fmod_s * s,
		uint16_t trid,
		uint8_t reg,
		int32_t val,
		FILE * dbg)
{
  uint8_t val8[4];
  val8[0] =  ((uint32_t) val) >> 24;
  val8[1] = (((uint32_t) val) >> 16) & 0x0000FF;
  val8[2] = (((uint32_t) val) >>  8) & 0x0000FF;
  val8[3] =  ((uint32_t) val)        & 0x0000FF;
  return fmod_wreg(s, trid, reg, val8, 4, dbg);
}


int32_t fmod_f2i(double k)
{
  k *= 65536;
  if(k >= 0x7FFFFFFF)
    return 0x7FFFFFFF;
  if(k <= (int32_t) 0x80000000)
    return (int32_t) 0x80000000;
  return rint(k);
}


double fmod_i2f(int32_t k)
{
  return k / 65536.0;
}


struct fmod_s * fmod_new(int fd)
{
  struct fmod_s * fs = malloc(sizeof(* fs));
  if(0 == fs)
    return 0;
  fs->fd = fd;
  if(0 != pthread_mutex_init(& fs->mutex, 0)){
    perror("pthread_mutex_init");
    free(fs);
    return 0;
  }
  return fs;
}


void fmod_delete(struct fmod_s * s)
{
  if(0 != pthread_mutex_destroy(& s->mutex))
    perror("WARNING pthread_mutex_destroy");
  free(s);
}
