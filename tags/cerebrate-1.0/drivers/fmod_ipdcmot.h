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


#ifndef FMOD_IPDCMOT_H
#define FMOD_IPDCMOT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include <drivers/fmod_util.h>
  

/** regulation mode: brake and stop motor */
#define FMOD_IPDCMOT_BRAKE     0x00

/** regulation mode: disconnect voltage */
#define FMOD_IPDCMOT_DRIVEOPEN 0x01

/** regulation mode: open loop */
#define FMOD_IPDCMOT_OPENLOOP  0x02

/** regulation mode: continue PWM w/o regulation */
#define FMOD_IPDCMOT_WAIT      0x03

/** regulation mode: speed control with acceleration etc */
#define FMOD_IPDCMOT_SPEEDCTRL 0x04

/** regulation mode: position control with ramps */
#define FMOD_IPDCMOT_POSCTRL   0x05
  
/** option: swap encoder pulse inputs */
#define FMOD_IPDCMOT_OPT_SWAPENC (1 << 0)

/** option: reverse direction */
#define FMOD_IPDCMOT_OPT_REVERSE (1 << 1)

/** option: interpolate measured velocity 10-fold */
#define FMOD_IPDCMOT_OPT_INTVEL  (1 << 2)

/** option: non-linear PID */
#define FMOD_IPDCMOT_OPT_NLPID   (1 << 3)

/** option: increase PWM resolution to 10bits (for 39kHz) */
#define FMOD_IPDCMOT_OPT_PWMRES  (1 << 4)

/** option: limit input should use pull-up resistors (instead of pull-down) */
#define FMOD_IPDCMOT_OPT_PULIM   (1 << 5)

/** option: save POSITION register to EEPROM on power-down */
#define FMOD_IPDCMOT_OPT_SAVEPOS (1 << 6)

/** option: integral term leakage */
#define FMOD_IPDCMOT_OPT_LEAK    (1 << 7)
  
/** warning: Enable pin not activated */
#define FMOD_IPDCMOT_WARN_DISABLE           (1 << 0)
#define FMOD_IPDCMOT_WARN_DISABLE_PREV      (1 << 1)
  
/** warning: Undervoltage on power pin */
#define FMOD_IPDCMOT_WARN_UNDERVOLTAGE      (1 << 2)
#define FMOD_IPDCMOT_WARN_UNDERVOLTAGE_PREV (1 << 3)
  
/** warning: Overvoltage on power pin */
#define FMOD_IPDCMOT_WARN_OVERVOLTAGE       (1 << 4)
#define FMOD_IPDCMOT_WARN_OVERVOLTAGE_PREV  (1 << 5)
  
/** warning: In POSCTRL, input +/- deadzone not reached yet */
#define FMOD_IPDCMOT_WARN_SERVOING          (1 << 6)
#define FMOD_IPDCMOT_WARN_SERVOING_PREV     (1 << 7)
  
/** warning: Command register is saturated */
#define FMOD_IPDCMOT_WARN_SATURATED         (1 << 8)
#define FMOD_IPDCMOT_WARN_SATURATED_PREV    (1 << 9)
  
/** warning: Homing procedure in progress */
#define FMOD_IPDCMOT_WARN_HOMING            (1 << 10)
#define FMOD_IPDCMOT_WARN_HOMING_PREV       (1 << 11)
  
/** warning: Limit 1 pin active */
#define FMOD_IPDCMOT_WARN_LIMIT1            (1 << 14)
#define FMOD_IPDCMOT_WARN_LIMIT1_PREV       (1 << 15)
  
/** warning: Limit 2 pin active */
#define FMOD_IPDCMOT_WARN_LIMIT2            (1 << 16)
#define FMOD_IPDCMOT_WARN_LIMIT2_PREV       (1 << 17)
  
/** limit switch setup: Activate limit detection */
#define FMOD_IPDCMOT_LIMIT_ENABLE (1 << 0)
  
/** limit switch setup: Active at high voltage (else active at low) */
#define FMOD_IPDCMOT_LIMIT_ACTHIGH (1 << 1)
  
/** limit switch setup: Trigger limit-regulation-mode when active */
#define FMOD_IPDCMOT_LIMIT_REGMODE (1 << 2)
  
/** limit switch setup: Trigger position change when active */
#define FMOD_IPDCMOT_LIMIT_POS (1 << 3)
  
/** limit switch setup: Do not trigger input change */
#define FMOD_IPDCMOT_LIMIT_INUNUSED  ((0 << 6) | (0 << 5))
/** limit switch setup: Trigger limitXYinput to INPUT */
#define FMOD_IPDCMOT_LIMIT_INPUT     ((0 << 6) | (1 << 5))
/** limit switch setup: Trigger limitXYinput to INPUTOFFSET */
#define FMOD_IPDCMOT_LIMIT_INOFFSET  ((1 << 6) | (0 << 5))
/** limit switch setup: Trigger limitXYinput to INPUTOFFSETMEASURED */
#define FMOD_IPDCMOT_LIMIT_INOFFMEAS ((1 << 6) | (1 << 5))

  
  int fmod_ipdcmot_confspeed(struct fmod_s * s,
			     double kp, double ki, double kd,
			     int32_t tspeed, double imax,
			     FILE * dbg);
  int fmod_ipdcmot_confpos(struct fmod_s * s, double kp, double ki, double kd,
			   double imax, int32_t tspeed, int32_t acc,
			   int32_t dec, int32_t dzone, int32_t initpos,
			   FILE * dbg);
    
  int fmod_ipdcmot_rmode(struct fmod_s * s, uint8_t * mode);
  int fmod_ipdcmot_wmode(struct fmod_s * s, uint8_t mode);
  
  int fmod_ipdcmot_ropt(struct fmod_s * s, uint32_t * opt);
  int fmod_ipdcmot_wopt(struct fmod_s * s, uint32_t opt);
  
  int fmod_ipdcmot_rwarn(struct fmod_s * s, uint32_t * warn);
  
  int fmod_ipdcmot_wlim1setup(struct fmod_s * s, uint32_t conf);
  int fmod_ipdcmot_wlim1mode(struct fmod_s * s, uint8_t mode);
  int fmod_ipdcmot_wlim1pos(struct fmod_s * s, int32_t pos);
  int fmod_ipdcmot_wlim1xin(struct fmod_s * s, int32_t xin);

  int fmod_ipdcmot_wlim2setup(struct fmod_s * s, uint32_t conf);
  int fmod_ipdcmot_wlim2mode(struct fmod_s * s, uint8_t mode);
  int fmod_ipdcmot_wlim2pos(struct fmod_s * s, int32_t pos);
  int fmod_ipdcmot_wlim2xin(struct fmod_s * s, int32_t xin);
  
  int fmod_ipdcmot_win(struct fmod_s * s, int32_t input);
  int fmod_ipdcmot_winoff(struct fmod_s * s, int32_t offset);
  int fmod_ipdcmot_winmin(struct fmod_s * s, int32_t mininput);
  int fmod_ipdcmot_winmax(struct fmod_s * s, int32_t maxinput);
  int fmod_ipdcmot_wimax(struct fmod_s * s, double maxcurrent);
  int fmod_ipdcmot_wimax_raw(struct fmod_s * s, int32_t maxcurrent);
  
  int fmod_ipdcmot_wkp(struct fmod_s * s, double kp);
  int fmod_ipdcmot_wki(struct fmod_s * s, double ki);
  int fmod_ipdcmot_wkd(struct fmod_s * s, double kd);
  int fmod_ipdcmot_wkp_raw(struct fmod_s * s, int32_t kp);
  int fmod_ipdcmot_wki_raw(struct fmod_s * s, int32_t ki);
  int fmod_ipdcmot_wkd_raw(struct fmod_s * s, int32_t kd);
  int fmod_ipdcmot_wacc(struct fmod_s * s, int32_t acceleration);
  int fmod_ipdcmot_wdec(struct fmod_s * s, int32_t deceleration);
  int fmod_ipdcmot_wtspeed(struct fmod_s * s, int32_t top_speed);
  int fmod_ipdcmot_wdzone(struct fmod_s * s, int32_t dead_zone);  
  int fmod_ipdcmot_wpos(struct fmod_s * s, int32_t position);
  
  int fmod_ipdcmot_rin(struct fmod_s * s, int32_t * input);
  int fmod_ipdcmot_rinmin(struct fmod_s * s, int32_t * mininput);
  int fmod_ipdcmot_rinmax(struct fmod_s * s, int32_t * maxinput);
  int fmod_ipdcmot_rpos(struct fmod_s * s, int32_t * position);
  int fmod_ipdcmot_rspeed(struct fmod_s * s, int32_t * speed);
  int fmod_ipdcmot_rimax(struct fmod_s * s, double * maxcurrent);
  
  // registre 0x32: sur les 4 bytes, seulement les 2 de poids faible
  // sont envoyés au PWM -> detection de roues bloquees facile si la
  // valeur retourner n'entre pas en 16 bits!
  //
  // on peut aussi utiliser le registre de warnings...
  int fmod_ipdcmot_rcom(struct fmod_s * s, int32_t * command);
  
  int fmod_ipdcmot_rkp(struct fmod_s * s, double * kp);
  int fmod_ipdcmot_rki(struct fmod_s * s, double * ki);
  int fmod_ipdcmot_rkd(struct fmod_s * s, double * kd);
  int fmod_ipdcmot_racc(struct fmod_s * s, int32_t * acceleration);
  int fmod_ipdcmot_rdec(struct fmod_s * s, int32_t * deceleration);
  int fmod_ipdcmot_rtspeed(struct fmod_s * s, int32_t * top_speed);
  int fmod_ipdcmot_rdzone(struct fmod_s * s, int32_t * dead_zone);

  
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // FMOD_IPDCMOT_H
