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


#include "Effects.hpp"
#include <util/Random.hpp>
#include <gfx/wrap_glu.hpp>
#include <gfx/Viewport.hpp>
#include <drivers/FModTCP.hpp>
#include <iostream>


using namespace std;


Effects::
Effects(FModTCP & io):
  _io(io),
  _enable_lightshow(false)
{
  DoSetShoulder(false, 0);
  DoSetEar(false, 0);
  DoSetLight(0, false, 0);
  DoSetLight(1, false, 0);
  DoSetLight(2, false, 0);
  DoSetLight(3, false, 0);
  DoSetLight(4, false, 0);
  DoSetLight(5, false, 0);
}


bool Effects::
DoSetShoulder(bool on, std::ostream * dbg)
{
  if( ! _io.SetBit(SHOULDER, on)){
    if(0 != dbg)
      (*dbg) << "ERROR in Effects::DoSetShoulder(): _io.SetBit() failed.\n";
    return false;
  }
  _bit[SHOULDER] = on;
  return true;
}


bool Effects::
DoSetEar(bool on, std::ostream * dbg)
{
  if( ! _io.SetBit(EAR, on)){
    if(0 != dbg)
      (*dbg) << "ERROR in Effects::DoSetEar(): _io.SetBit() failed.\n";
    return false;
  }
  _bit[EAR] = on;
  return true;
}


bool Effects::
DoSetLight(int num, bool on, std::ostream * dbg)
{
  if((num > 5) || (num < 0)){
    if(0 != dbg)
      (*dbg) << "ERROR in Effects::DoSetLight(): (num > 5) || (num < 0).\n";
    return false;
  }
  if( ! _io.SetBit(num, on)){
    if(0 != dbg)
      (*dbg) << "ERROR in Effects::DoSetLight(): _io.SetBit() failed.\n";
    return false;
  }
  _bit[num] = on;
  return true;
}


void Effects::
UpdateLightshow()
{
  if( ! _enable_lightshow)
    return;
  
  // SHOULDER
  if(_shoulder_on_tmax <= TMAX_THRESH){
    if(_bit[SHOULDER])
      DoSetShoulder(false, 0);
  }
  else if(_shoulder_off_tmax <= TMAX_THRESH){
    if( ! _bit[SHOULDER])
      DoSetShoulder(true, 0);
  }
  else{
    _shoulder_timeout.UpdateAbsolute();
    if(_shoulder_timeout.GetExpired()){
      if(_bit[SHOULDER]){
	DoSetShoulder(false, 0);
	_shoulder_timeout.Set(Random::Uniform(_shoulder_off_tmin,
					      _shoulder_off_tmax));
      }
      else{
	DoSetShoulder(true, 0);
	_shoulder_timeout.Set(Random::Uniform(_shoulder_on_tmin,
					      _shoulder_on_tmax));
      }
    }
  }
  
  // EAR
  if(_ear_on_tmax <= TMAX_THRESH){
    if(_bit[EAR])
      DoSetEar(false, 0);
  }
  else if(_ear_off_tmax <= TMAX_THRESH){
    if( ! _bit[EAR])
      DoSetEar(true, 0);
  }
  else{
    _ear_timeout.UpdateAbsolute();
    if(_ear_timeout.GetExpired()){
      if(_bit[EAR]){
	DoSetEar(false, 0);
	_ear_timeout.Set(Random::Uniform(_ear_off_tmin,
					 _ear_off_tmax));
      }
      else{
	DoSetEar(true, 0);
	_ear_timeout.Set(Random::Uniform(_ear_on_tmin,
					 _ear_on_tmax));
      }
    }
  }
  

  // LIGHT
  _light_timeout.UpdateAbsolute();
  if(_light_timeout.GetExpired()){
    while(true){
      const int num(Random::Uniform(0, 5));
      if(num != _active_light){
	DoSetLight(_active_light, false, 0);
	DoSetLight(num, true, 0);
	_active_light = num;
	break;
      }
    }
    _light_timeout.Set(Random::Uniform(_light_tmin, _light_tmax));
  }
}


bool Effects::
_SetShoulder(bool on, std::ostream * dbg)
{
  _enable_lightshow = false;
  return DoSetShoulder(on, dbg);
}


bool Effects::
_SetEar(bool on, std::ostream * dbg)
{
  _enable_lightshow = false;
  return DoSetEar(on, dbg);
}


bool Effects::
_SetLight(int num, bool on, std::ostream * dbg)
{
  _enable_lightshow = false;
  return DoSetLight(num, on, dbg);
}


void Effects::
SetLightshow(double shoulder_on_tmin, double shoulder_on_tmax,
	     double shoulder_off_tmin, double shoulder_off_tmax,
	     double ear_on_tmin, double ear_on_tmax,
	     double ear_off_tmin, double ear_off_tmax,
	     double light_tmin, double light_tmax)
{
  DoSetLightshow(shoulder_on_tmin, shoulder_on_tmax,
		 ear_on_tmin, ear_on_tmax,
		 shoulder_off_tmin, shoulder_off_tmax,
		 ear_off_tmin, ear_off_tmax,
		 light_tmin, light_tmax,
		 ! _enable_lightshow);
  _enable_lightshow = true;
}


void Effects::
DoSetLightshow(double shoulder_on_tmin, double shoulder_on_tmax,
	       double shoulder_off_tmin, double shoulder_off_tmax,
	       double ear_on_tmin, double ear_on_tmax,
	       double ear_off_tmin, double ear_off_tmax,
	       double light_tmin, double light_tmax,
	       bool re_init)
{
  if(re_init
     || (_shoulder_on_tmin != shoulder_on_tmin)
     || (_shoulder_on_tmax != shoulder_on_tmax)
     || (_shoulder_off_tmin != shoulder_off_tmin)
     || (_shoulder_off_tmax != shoulder_off_tmax)){
    _shoulder_on_tmin = shoulder_on_tmin;
    _shoulder_on_tmax = shoulder_on_tmax;
    _shoulder_off_tmin = shoulder_off_tmin;
    _shoulder_off_tmax = shoulder_off_tmax;
    _shoulder_timeout.Set(0);
  }

  if(re_init
     || (_ear_on_tmin != ear_on_tmin)
     || (_ear_on_tmax != ear_on_tmax)
     || (_ear_off_tmin != ear_off_tmin)
     || (_ear_off_tmax != ear_off_tmax)){
    _ear_on_tmin = ear_on_tmin;
    _ear_on_tmax = ear_on_tmax;
    _ear_off_tmin = ear_off_tmin;
    _ear_off_tmax = ear_off_tmax;
    _ear_timeout.Set(0);
  }
  
  if(re_init
     || (_light_tmin != light_tmin)
     || (_light_tmax != light_tmax)){
    _light_tmin = light_tmin;
    _light_tmax = light_tmax;
    _light_timeout.Set(0);
    if(_active_light >= 0){
      DoSetLight(_active_light, false, 0);
      _active_light = -1;
    }
  }
}


void Effects::
Draw()
{  
  if(_enable_lightshow){
    if(_shoulder_on_tmax <= TMAX_THRESH){
      glColor3d(1, 0, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glRectd(SHOULDER, 0, SHOULDER + 1, 1);
    }
    else if(_shoulder_off_tmax <= TMAX_THRESH){
      glColor3d(0, 1, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glRectd(SHOULDER, 0, SHOULDER + 1, 1);
    }
    else
      _shoulder_timeout.Draw(SHOULDER, SHOULDER + 1);
    
    if(_ear_on_tmax <= TMAX_THRESH){
      glColor3d(1, 0, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glRectd(EAR, 0, EAR + 1, 1);
    }
    else if(_ear_off_tmax <= TMAX_THRESH){
      glColor3d(0, 1, 0);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glRectd(EAR, 0, EAR + 1, 1);
    }
    else
      _ear_timeout.Draw(EAR, EAR + 1);
    
    if(_active_light >= 0)
      _light_timeout.Draw(_active_light, _active_light + 1);
  }
  
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  for(size_t i(0); i < 8; ++i){
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslated(i + 0.5, 0.5, 0);
    if(_bit[i])
      glColor3d(1, 1, 1);
    else
      glColor3d(0.5, 0.5, 0.5);
    gluDisk(wrap_glu_quadric_instance(), 0, 0.4, 36, 1);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
}


void Effects::
ConfigureViewport(Viewport & vp) const
{
  vp.Remap(Subwindow::logical_bbox_t(0, 0, 9, 1));
}
