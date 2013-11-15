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


#ifndef EFFECTS_HPP
#define EFFECTS_HPP


#include <aci/Timeout.hpp>
#include <iosfwd>


class FModTCP;
class Viewport;


class Effects
{
public:
  Effects(FModTCP & io);
  
  void UpdateLightshow();
  
  void SetLightshow(double shoulder_on_tmin, double shoulder_on_tmax,
		    double shoulder_off_tmin, double shoulder_off_tmax,
		    double ear_on_tmin, double ear_on_tmax,
		    double ear_off_tmin, double ear_off_tmax,
		    double light_tmin, double light_tmax);
  bool _SetShoulder(bool on, std::ostream * dbg);
  bool _SetEar(bool on, std::ostream * dbg);
  bool _SetLight(int num, bool on, std::ostream * dbg);

  void Draw();
  void ConfigureViewport(Viewport & vp) const;

  
private:
  void DoSetLightshow(double shoulder_on_tmin, double shoulder_on_tmax,
		      double shoulder_off_tmin, double shoulder_off_tmax,
		      double ear_on_tmin, double ear_on_tmax,
		      double ear_off_tmin, double ear_off_tmax,
		      double light_tmin, double light_tmax,
		      bool re_init);
  bool DoSetShoulder(bool on, std::ostream * dbg);
  bool DoSetEar(bool on, std::ostream * dbg);
  bool DoSetLight(int num, bool on, std::ostream * dbg);

  static const int SHOULDER = 6;
  static const int EAR = 7;
  static const double TMAX_THRESH = 0.01;
  
  FModTCP & _io;
  bool _bit[8];
  bool _enable_lightshow;
  double _shoulder_on_tmin, _shoulder_on_tmax;
  double _shoulder_off_tmin, _shoulder_off_tmax;
  double _ear_on_tmin, _ear_on_tmax;
  double _ear_off_tmin, _ear_off_tmax;
  double _light_tmin, _light_tmax;
  int _active_light;
  Timeout _shoulder_timeout;
  Timeout _ear_timeout;
  Timeout _light_timeout;
};

#endif // EFFECTS_HPP
