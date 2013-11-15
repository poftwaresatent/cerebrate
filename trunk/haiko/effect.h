/* 
 * Copyright (C) 2008 Roland Philippsen <roland.philippsen@gmx.net>
 * 
 * BSD-style license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of
 *    contributors to this software may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR THE CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef HAIKO_EFFECT_H
#define HAIKO_EFFECT_H


#include "voxel.h"
#include <stdio.h>


typedef enum {
  EFFECT_DISTREF_POINT,
  EFFECT_DISTREF_LINE,
  EFFECT_DISTREF_PLANE
} effect_distref_type_t;


typedef struct effect_distref_s {
  effect_distref_type_t type;
  double point[3];
  double unit[3];		/* either direction, normal, or ignored */
} effect_distref_t;


typedef struct effect_s {
  effect_distref_t distance;
  effect_distref_t direction;
  double duration, period, speed, minval, maxgrow, power;
  double phase;			/* += speed at each update()  */
  double (*amplitude)(struct effect_s * ww, double dist, double instant);
  void (*init)(struct effect_s * ww, voxel_t * first);
  void (*update)(struct effect_s * ww, voxel_t * first);
} effect_t;


int effect_parse_file(FILE * configfile,
		      effect_t * balloon,
		      effect_t * warp,
		      int verbose);

void effect_configure_balloon(effect_t * ww);

void effect_configure_warp(effect_t * ww);


double effect_distance(double const * from,
		       effect_distref_t const * to,
		       /* optional */
		       double * delta);

double effect_point_distance(double const * from,
			     double const * to,
			     /* optional */
			     double * delta);

double effect_line_distance(double const * from,
			    double const * to_point,
			    /* expected to be of unit length */
			    double const * to_direction,
			    /* optional */
			    double * delta);

double effect_plane_distance(double const * from,
			     double const * to_point,
			     /* expected to be of unit length */
			     double const * to_normal,
			     /* optional */
			     double * delta);


double effect_sawtooth(double duration, double period,
		       double outside_value,
		       double normdist, double instant);

double effect_amplitude_bump(effect_t * ww, double dist, double instant);

double effect_amplitude_spike(effect_t * ww, double dist, double instant);

void effect_balloon_init(effect_t * ww, voxel_t * first);

void effect_balloon_update(effect_t * ww, voxel_t * first);

void effect_warp_init(effect_t * ww, voxel_t * first);

void effect_warp_update(effect_t * ww, voxel_t * first);


#endif /* HAIKO_EFFECT_H */
