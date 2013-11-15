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


#ifndef CIRCLE_LSQ_HPP
#define CIRCLE_LSQ_HPP


class Scanalysis;


class CircleLSQ
{
private:
  CircleLSQ(double xc, double yc, double radius, int npoints, int label,
	    double maxdist);
  
public:
  CircleLSQ(const CircleLSQ & original);

  static CircleLSQ * Create(const Scanalysis & scanalysis,
			    int startindex, int endindex,
			    int label);
  
  double CalculateDistance(double x, double y) const;
  
  void Draw(bool filled) const;
  
  static double CalculateSxn(double n, const Scanalysis & scanalysis,
			     int startindex, int endindex);
  static double CalculateSym(double m, const Scanalysis & scanalysis,
			     int startindex, int endindex);
  static double CalculateSxnym(double n, double m,
			       const Scanalysis & scanalysis,
			       int startindex, int endindex);
  
  const double xc, yc, radius;
  const int npoints;
  const int label;
  const double maxdist;
};

#endif // CIRCLE_LSQ_HPP
