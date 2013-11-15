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


#include "CircleLSQ.hpp"
#include "gfx/wrap_glu.hpp"
#include "Scanalyzer.hpp"
#include <sfl/numeric.hpp>
#include <iostream>		// dbg


using namespace sfl;


CircleLSQ::
CircleLSQ(double _xc, double _yc, double _radius, int _npoints, int _label,
	  double _maxdist):
  xc(_xc), yc(_yc), radius(_radius), npoints(_npoints), label(_label),
  maxdist(_maxdist)
{
}


CircleLSQ::
CircleLSQ(const CircleLSQ & original):
  xc(original.xc), yc(original.yc), radius(original.radius),
  npoints(original.npoints), label(original.label),
  maxdist(original.maxdist)
{
}


CircleLSQ * CircleLSQ::
Create(const Scanalysis & scanalysis, int startindex, int endindex, int label)
{
  const int I(endindex - startindex + 1);
  if(I < 3)
    return 0;
  
  const double Sx( CalculateSxn(1, scanalysis, startindex, endindex));
  const double Sx2(CalculateSxn(2, scanalysis, startindex, endindex));
  const double Sx3(CalculateSxn(3, scanalysis, startindex, endindex));
  const double S2x(sqr(Sx));
  const double S2x2(sqr(Sx2));
  
  const double Sy( CalculateSym(1, scanalysis, startindex, endindex));
  const double Sy2(CalculateSym(2, scanalysis, startindex, endindex));
  const double Sy3(CalculateSym(3, scanalysis, startindex, endindex));
  const double S2y(sqr(Sy));
  const double S2y2(sqr(Sy2));

  const double Sxy( CalculateSxnym(1, 1, scanalysis, startindex, endindex));
  const double Sx2y(CalculateSxnym(2, 1, scanalysis, startindex, endindex));
  const double Sxy2(CalculateSxnym(1, 2, scanalysis, startindex, endindex));
  const double S2xy(sqr(Sxy));
  
  const double
    Nx(I*Sx2y*Sxy - Sx*Sx2y*Sy - Sx2*Sxy*Sy + Sx3*S2y + Sxy2*S2y + Sx*Sx2*Sy2
       - I*Sx3*Sy2 - I*Sxy2*Sy2 - Sxy*Sy*Sy2 + Sx*S2y2 + I*Sxy*Sy3
       - Sx*Sy*Sy3);
  const double
    Ny(S2x*Sx2y - I*Sx2*Sx2y - Sx*Sx2*Sxy + I*Sx3*Sxy + I*Sxy*Sxy2 + S2x2*Sy
       - Sx*Sx3*Sy - Sx*Sxy2*Sy - Sx*Sxy*Sy2 + Sx2*Sy*Sy2 + S2x*Sy3
       - I*Sx2*Sy3);
  const double
    Dxy(2*I*S2xy - 4*Sx*Sxy*Sy + 2*Sx2*S2y + 2*S2x*Sy2 - 2*I*Sx2*Sy2);
  
  const double xc(Nx / Dxy);
  const double yc(Ny / Dxy);
  const double
    radius(sqrt(xc*xc + yc*yc + (Sx2 + Sy2 - 2*Sx*xc - 2*Sy*yc) / I));
  
  double maxdist(0);
  for(int i(startindex); i <= endindex; ++i){
    const double dist(absval(sqrt(sqr(scanalysis.x[i] - xc)
				  + sqr(scanalysis.y[i] - yc)) - radius));
    if(dist > maxdist)
      maxdist = dist;
  }
  
  return new CircleLSQ(xc, yc, radius, I, label, maxdist);
}


double CircleLSQ::
CalculateDistance(double x, double y) const
{
  return sqrt(sqr(x - xc) + sqr(y - yc)) - radius;
}


double CircleLSQ::
CalculateSxn(double n, const Scanalysis & scanalysis,
	     int startindex, int endindex)
{
  double result(0);
  for(int i(startindex); i <= endindex; ++i)
    result += pow(scanalysis.x[i], n);
  return result;
}


double CircleLSQ::
CalculateSym(double m, const Scanalysis & scanalysis,
	     int startindex, int endindex)
{
  double result(0);
  for(int i(startindex); i <= endindex; ++i)
    result += pow(scanalysis.y[i], m);
  return result;
}


double CircleLSQ::
CalculateSxnym(double n, double m,
	       const Scanalysis & scanalysis,
	       int startindex, int endindex)
{
  double result(0);
  for(int i(startindex); i <= endindex; ++i)
    result += pow(scanalysis.x[i], n) * pow(scanalysis.y[i], m);
  return result;
}


void CircleLSQ::
Draw(bool filled)
  const
{
#undef HACKED_CIRCLE_DRAWING
#ifdef HACKED_CIRCLE_DRAWING
  double dr;
  if(filled)
    dr = radius / 10;
  else
    dr = radius;
  glPolygonMode(GL_FRONT, GL_LINE);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(xc, yc, 0);
  for(double r(radius); r >= 0.5 * radius; r -= dr)
    gluDisk(wrap_glu_quadric_instance(), r, r, 36, 1);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
#else // HACKED_CIRCLE_DRAWING
  if(filled)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  else
    glPolygonMode(GL_FRONT, GL_LINE);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(xc, yc, 0);
  if(filled)
    gluDisk(wrap_glu_quadric_instance(), 0, radius, 36, 1);
  else  
    gluDisk(wrap_glu_quadric_instance(), radius, radius, 36, 1);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
#endif // HACKED_CIRCLE_DRAWING
}
