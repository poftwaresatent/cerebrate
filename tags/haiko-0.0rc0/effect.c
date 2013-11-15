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


#include "effect.h"
/* #include <stdlib.h> */
/* #include <stdio.h> */
#include <math.h>
#include <string.h>


double effect_point_distance(double const * from, double const * to,
			     double * delta)
{
  int ii;
  double dist = 0;
  if (NULL == delta)
    for (ii = 0; ii < 3; ii++)
      dist += pow(to[ii] - from[ii], 2);
  else
    for (ii = 0; ii < 3; ii++) {
      delta[ii] = to[ii] - from[ii];
      dist += pow(delta[ii], 2);
    }
  return sqrt(dist);
}


static double inner(double const * aa, double const * bb)
{
  double result = 0;
  int ii;
  for (ii = 0; ii < 3; ii++)
    result += aa[ii] * bb[ii];
  return result;
}


double effect_line_distance(double const * from,
			    double const * to_point,
			    double const * to_direction,
			    double * delta)
{
  if (NULL == delta) {
    int ii;
    double dd[3];
    double hyp, ank;
    hyp = 0;
    for (ii = 0; ii < 3; ii++) {
      dd[ii] = to_point[ii] - from[ii];
      hyp += pow(dd[ii], 2);
    }
    ank = inner(to_direction, dd);
    return sqrt(hyp - pow(ank, 2));
  }
  else {
    int ii;
    double len, dist;
    for (ii = 0; ii < 3; ii++)
      delta[ii] = to_point[ii] - from[ii];
    len = inner(to_direction, delta);
    dist = 0;
    for (ii = 0; ii < 3; ii++) {
      delta[ii] -= len * to_direction[ii];
      dist += pow(delta[ii], 2);
    }
    return sqrt(dist);
  }
}


double effect_plane_distance(double const * from,
			     double const * to_point,
			     double const * to_normal,
			     double * delta)
{
  int ii;
  double dd[3];
  double dist;
  for (ii = 0; ii < 3; ii++)
    dd[ii] = to_point[ii] - from[ii];
  dist = inner(to_normal, dd);
  if (NULL != delta)
    for (ii = 0; ii < 3; ii++)
      delta[ii] = dist * to_normal[ii];
  return dist;
}


double effect_distance(double const * from,
		       effect_distref_t const * to,
		       double * delta)
{
  switch (to->type) {
  case EFFECT_DISTREF_POINT:
    return effect_point_distance(from, to->point, delta);
  case EFFECT_DISTREF_LINE:
    return effect_line_distance(from, to->point, to->unit, delta);
  case EFFECT_DISTREF_PLANE:
    return effect_plane_distance(from, to->point, to->unit, delta);
  }
  fprintf(stderr,
	  "ERROR in effect_distance(): invalid effect_distref::type %d\n",
	  to->type);
  return -1;
}


double effect_sawtooth(double duration, double period,
		       double outside_value,
		       double dist, double instant)
{
  dist = fmod(dist - instant, period);
  if (dist < 0)
    dist += period;
  if ((dist >= 0) && (dist <= duration))
    return 2 * dist / duration - 1;
  return outside_value;
}


double effect_amplitude_bump(effect_t * ww, double dist, double instant)
{
  double aa = effect_sawtooth(ww->duration, ww->period, 1, dist, instant);
  return ww->maxgrow * (1 - pow(fabs(aa), ww->power)) + ww->minval;
}


double effect_amplitude_spike(effect_t * ww, double dist, double instant)
{
  double aa = effect_sawtooth(ww->duration, ww->period, 1, dist, instant);
  return ww->maxgrow * pow(1 - fabs(aa), ww->power) + ww->minval;
}


void effect_balloon_init(struct effect_s * ww, voxel_t * first)
{
  for (/**/; NULL != first; first = first->next)
    first->balloondist = effect_distance(first->pos, &ww->distance, NULL);
}


void effect_balloon_update(struct effect_s * ww, voxel_t * first)
{
  if (NULL == ww->amplitude)
    return;
  for (/**/; NULL != first; first = first->next)
    first->length = ww->amplitude(ww, first->balloondist, ww->phase);
  ww->phase += ww->speed;
}


void effect_warp_init(struct effect_s * ww, voxel_t * first)
{
  for (/**/; NULL != first; first = first->next) {
    double norm = effect_distance(first->pos, &ww->direction, first->warp);
    first->warpdist = effect_distance(first->pos, &ww->distance, NULL);
    if (fabs(first->warpdist) > 1e-6) {
      int ii;
      for (ii = 0; ii < 3; ii++)
	first->warp[ii] /= norm;
    }
  }
}


void effect_warp_update(struct effect_s * ww, voxel_t * first)
{
  if (NULL == ww->amplitude)
    return;
  for (/**/; NULL != first; first = first->next) {
    int ii;
    double len = ww->amplitude(ww, first->warpdist, ww->phase);
    for (ii = 0; ii < 3; ii++)
      first->off[ii] = len * first->warp[ii];
  }
  ww->phase += ww->speed;
}


void effect_configure_balloon(effect_t * ww)
{
  ww->init = effect_balloon_init;
  ww->update = effect_balloon_update;
}


void effect_configure_warp(effect_t * ww)
{
  ww->init = effect_warp_init;
  ww->update = effect_warp_update;
}
