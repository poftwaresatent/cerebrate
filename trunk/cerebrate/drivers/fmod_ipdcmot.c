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


#include "fmod_ipdcmot.h"


int fmod_ipdcmot_confspeed(struct fmod_s * s,
			   double kp,
			   double ki,
			   double kd,
			   int32_t tspeed,
			   double imax,
			   FILE * dbg)
{
  int result;
  int32_t inmax = 0x7FFFFFFF, inmin = - inmax - 1;
  int32_t acc = 2 * tspeed, dec = acc, in = 0;

  result = fmod_ipdcmot_wmode(s,FMOD_IPDCMOT_BRAKE);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wmode() failed.\n");
    return -1;
  }
  
  result = fmod_ipdcmot_wkp(s,kp);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wkp() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rkp(s,& kp);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rkp() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():\n  kp = %f\n", kp);

  result = fmod_ipdcmot_wki(s,ki);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wki() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rki(s,& ki);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rki() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  ki = %f\n", ki);

  result = fmod_ipdcmot_wkd(s,kd);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wkd() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rkd(s,& kd);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rkd() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  kd = %f\n", kd);

  result = fmod_ipdcmot_wtspeed(s,tspeed);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wtspeed() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rtspeed(s,& tspeed);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rtspeed() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  tspeed = %d\n", tspeed);

  result = fmod_ipdcmot_wimax(s,imax);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wimax() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rimax(s,& imax);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rimax() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  imax = %f\n", imax);

  result = fmod_ipdcmot_winmin(s,inmin);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_winmin() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rinmin(s,& inmin);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rinmin() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  inmin = %d\n", inmin);

  result = fmod_ipdcmot_winmax(s,inmax);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_winmax() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rinmax(s,& inmax);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rinmax() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  inmax = %d\n", inmax);

  result = fmod_ipdcmot_wacc(s,acc);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wacc() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_racc(s,& acc);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_racc() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  acc = %d\n", acc);

  result = fmod_ipdcmot_wdec(s,dec);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wdec() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rdec(s,& dec);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rdec() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  dec = %d\n", dec);

  result = fmod_ipdcmot_win(s,in);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_win() failed.\n");
    return -1;
  }
  result = fmod_ipdcmot_rin(s,& in);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_rin() failed.\n");
    return -1;
  }
  if(0 != dbg)
    fprintf(dbg, "  in = %d\n", in);

  result = fmod_ipdcmot_wmode(s, FMOD_IPDCMOT_SPEEDCTRL);
  if(result != FMOD_OK){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confspeed():"
	      " fmod_ipdcmot_wmode() failed.\n");
    return -9;
  }
  return 0;
}


int fmod_ipdcmot_confpos(struct fmod_s * s, double kp, double ki, double kd,
			 double imax, int32_t tspeed, int32_t acc, int32_t dec,
			 int32_t dzone, int32_t initpos, FILE * dbg)
{
  int res;
  res = fmod_ipdcmot_wmode(s, FMOD_IPDCMOT_BRAKE);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wmode() failed.\n");
    return 1;
  }
  res = fmod_ipdcmot_wkp(s, kp);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wkp() failed.\n");
    return 2;
  }
  res = fmod_ipdcmot_wki(s, ki);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wki() failed.\n");
    return 3;
  }
  res = fmod_ipdcmot_wkd(s, kd);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wkd() failed.\n");
    return 4;
  }
  res = fmod_ipdcmot_wimax(s, imax);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wimax() failed.\n");
    return 5;
  }
  res = fmod_ipdcmot_wtspeed(s, tspeed);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wtspeed() failed.\n");
    return 6;
  }
  res = fmod_ipdcmot_wacc(s, acc);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wacc() failed.\n");
    return 7;
  }
  res = fmod_ipdcmot_wdec(s, dec);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wdec() failed.\n");
    return 8;
  }
  res = fmod_ipdcmot_wdzone(s, dzone);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wdzone() failed.\n");
    return 9;
  }
  res = fmod_ipdcmot_wpos(s, initpos);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wpos() failed.\n");
    return 10;
  }
  res = fmod_ipdcmot_win(s, initpos);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg, "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_win() failed.\n");
    return 11;
  }
  res = fmod_ipdcmot_wmode(s, FMOD_IPDCMOT_POSCTRL);
  if(FMOD_OK != res){
    if(0 != dbg)
      fprintf(dbg,
	      "DEBUG fmod_ipdcmot_confpos():"
	      " fmod_ipdcmot_wmode() failed.\n");
    return 12;
  }
  return 0;
}


int fmod_ipdcmot_rmode(struct fmod_s * s,
		       uint8_t * mode)
{
  return fmod_rreg(s, 0x1234, 0x20, mode, 1, 0);
}


int fmod_ipdcmot_wmode(struct fmod_s * s, uint8_t mode)
{
  return fmod_wreg(s, 0x1234, 0x20, & mode, 1, 0);
}


int fmod_ipdcmot_ropt(struct fmod_s * s, uint32_t * opt)
{
  return fmod_rreg32(s, 0x1234, 0x2C, opt, 0);
}


int fmod_ipdcmot_rwarn(struct fmod_s * s, uint32_t * warn)
{
  return fmod_rreg32(s, 0x1234, 0x08, warn, 0);
}


int fmod_ipdcmot_wopt(struct fmod_s * s, uint32_t opt)
{
  return fmod_wreg32(s, 0x1234, 0x2C, opt, 0);
}


int fmod_ipdcmot_win(struct fmod_s * s, int32_t input)
{
  return fmod_wreg32(s, 0x1234, 0x21, input, 0);
}


int fmod_ipdcmot_rin(struct fmod_s * s, int32_t * input)
{
  return fmod_rreg32(s, 0x1234, 0x21, input, 0);
}


int fmod_ipdcmot_rinmin(struct fmod_s * s, int32_t * mininput)
{
  return fmod_rreg32(s, 0x1234, 0x24, mininput, 0);
}


int fmod_ipdcmot_rinmax(struct fmod_s * s, int32_t * maxinput)
{
  return fmod_rreg32(s, 0x1234, 0x25, maxinput, 0);
}


int fmod_ipdcmot_winoff(struct fmod_s * s, int32_t offset)
{
  return fmod_wreg32(s, 0x1234, 0x22, offset, 0);
}


int fmod_ipdcmot_winmin(struct fmod_s * s, int32_t mininput)
{
  return fmod_wreg32(s, 0x1234, 0x24, mininput, 0);
}


int fmod_ipdcmot_winmax(struct fmod_s * s, int32_t maxinput)
{
  return fmod_wreg32(s, 0x1234, 0x25, maxinput, 0);
}


int fmod_ipdcmot_wpos(struct fmod_s * s, int32_t position)
{
  return fmod_wreg32(s, 0x1234, 0x26, position, 0);
}


int fmod_ipdcmot_rpos(struct fmod_s * s, int32_t * position)
{
  return fmod_rreg32(s, 0x1234, 0x26, position, 0);
}


int fmod_ipdcmot_rspeed(struct fmod_s * s, int32_t * speed)
{
  return fmod_rreg32(s, 0x1234, 0x28, speed, 0);
}


int fmod_ipdcmot_wkp_raw(struct fmod_s * s, int32_t kp)
{
  return fmod_wreg32(s, 0x1234, 0x33, kp, 0);
}


int fmod_ipdcmot_wki_raw(struct fmod_s * s, int32_t ki)
{
  return fmod_wreg32(s, 0x1234, 0x34, ki, 0);
}


int fmod_ipdcmot_wkd_raw(struct fmod_s * s, int32_t kd)
{
  return fmod_wreg32(s, 0x1234, 0x35, kd, 0);
}


int fmod_ipdcmot_wacc(struct fmod_s * s, int32_t acceleration)
{
  return fmod_wreg32(s, 0x1234, 0x40, acceleration, 0);
}


int fmod_ipdcmot_wdec(struct fmod_s * s, int32_t deceleration)
{
  return fmod_wreg32(s, 0x1234, 0x41, deceleration, 0);
}


int fmod_ipdcmot_wkp(struct fmod_s * s,
		     double kp)
{
  return fmod_ipdcmot_wkp_raw(s, fmod_f2i(kp));
}


int fmod_ipdcmot_wki(struct fmod_s * s,
		     double ki)
{
  return fmod_ipdcmot_wki_raw(s, fmod_f2i(ki));
}


int fmod_ipdcmot_wkd(struct fmod_s * s,
		     double kd)
{
  return fmod_ipdcmot_wkd_raw(s, fmod_f2i(kd));
}


int fmod_ipdcmot_rkp(struct fmod_s * s,
		     double * kp)
{
  int32_t foo;
  int result = fmod_rreg32(s, 0x1234, 0x33, & foo, 0);
  if(result != FMOD_OK)
    return result;
  * kp = fmod_i2f(foo);
  return FMOD_OK;
}


int fmod_ipdcmot_rki(struct fmod_s * s,
		     double * ki)
{
  int32_t foo;
  int result = fmod_rreg32(s, 0x1234, 0x34, & foo, 0);
  if(result != FMOD_OK)
    return result;
  * ki = fmod_i2f(foo);
  return FMOD_OK;
}


int fmod_ipdcmot_rkd(struct fmod_s * s,
		     double * kd)
{
  int32_t foo;
  int result = fmod_rreg32(s, 0x1234, 0x35, & foo, 0);
  if(result != FMOD_OK)
    return result;
  * kd = fmod_i2f(foo);
  return FMOD_OK;
}


int fmod_ipdcmot_racc(struct fmod_s * s, int32_t * acceleration)
{
  return fmod_rreg32(s, 0x1234, 0x40, acceleration, 0);
}


int fmod_ipdcmot_rdec(struct fmod_s * s, int32_t * acceleration)
{
  return fmod_rreg32(s, 0x1234, 0x41, acceleration, 0);
}


int fmod_ipdcmot_rtspeed(struct fmod_s * s, int32_t * top_speed)
{
  return fmod_rreg32(s, 0x1234, 0x42, top_speed, 0);
}


int fmod_ipdcmot_rdzone(struct fmod_s * s, int32_t * dead_zone)
{
  return fmod_rreg32(s, 0x1234, 0x43, dead_zone, 0);
}


int fmod_ipdcmot_wtspeed(struct fmod_s * s, int32_t top_speed)
{
  return fmod_wreg32(s, 0x1234, 0x42, top_speed, 0);
}


int fmod_ipdcmot_wdzone(struct fmod_s * s, int32_t dead_zone)
{
  return fmod_wreg32(s, 0x1234, 0x43, dead_zone, 0);
}


int fmod_ipdcmot_rimax(struct fmod_s * s, double * maxcurrent)
{
  int32_t foo;
  int result = fmod_rreg32(s, 0x1234, 0x2A, & foo, 0);
  if(result != FMOD_OK)
    return result;
  * maxcurrent = fmod_i2f(foo);
  return FMOD_OK;
}


int fmod_ipdcmot_rcom(struct fmod_s * s, int32_t * command)
{
  int result = fmod_rreg32(s, 0x1234, 0x32, command, 0);
  if(result != FMOD_OK)
    return result;
  return FMOD_OK;
}


int fmod_ipdcmot_wimax(struct fmod_s * s, double maxcurrent)
{
  return fmod_ipdcmot_wimax_raw(s, fmod_f2i(maxcurrent));
}


int fmod_ipdcmot_wimax_raw(struct fmod_s * s, int32_t maxcurrent)
{
  return fmod_wreg32(s, 0x1234, 0x2A, maxcurrent, 0);
}


int fmod_ipdcmot_wlim1setup(struct fmod_s * s, uint32_t conf)
{
  return fmod_wreg32(s, 0x1234, 0x50, conf, 0);
}


int fmod_ipdcmot_wlim1mode(struct fmod_s * s, uint8_t mode)
{
  return fmod_wreg(s, 0x1234, 0x51, & mode, 1, 0);
}


int fmod_ipdcmot_wlim1pos(struct fmod_s * s, int32_t pos)
{
  return fmod_wreg32(s, 0x1234, 0x52, pos, 0);
}


int fmod_ipdcmot_wlim1xin(struct fmod_s * s, int32_t xin)
{
  return fmod_wreg32(s, 0x1234, 0x53, xin, 0);
}


int fmod_ipdcmot_wlim2setup(struct fmod_s * s, uint32_t conf)
{
  return fmod_wreg32(s, 0x1234, 0x58, conf, 0);
}


int fmod_ipdcmot_wlim2mode(struct fmod_s * s, uint8_t mode)
{
  return fmod_wreg(s, 0x1234, 0x59, & mode, 1, 0);
}


int fmod_ipdcmot_wlim2pos(struct fmod_s * s, int32_t pos)
{
  return fmod_wreg32(s, 0x1234, 0x5A, pos, 0);
}


int fmod_ipdcmot_wlim2xin(struct fmod_s * s, int32_t xin)
{
  return fmod_wreg32(s, 0x1234, 0x5B, xin, 0);
}
