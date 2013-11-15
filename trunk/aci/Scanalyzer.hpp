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


#ifndef SCANALYZER_HPP
#define SCANALYZER_HPP


#include <util/Timestamp.hpp>
#include <iosfwd>
#include <vector>
#include <memory>
#include <string>


class CircleLSQ;
class Viewport;

namespace sfl {
  class Polygon;
}


class Scanalysis
{
public:
  typedef enum {
    INVALID,
    BACKGROUND,
    OBJECT
  } category_t;
  
  Scanalysis();
  Scanalysis(const Scanalysis & original);
  Scanalysis & operator = (const Scanalysis & original);
  ~Scanalysis();
  
  void Draw() const;

  static const int scansize = 361;
  
  double x[scansize], y[scansize], rho[scansize], bgrho[scansize];
  category_t category[scansize];
  int label[scansize];
  std::vector<size_t> startindex;
  std::vector<size_t> endindex;
  std::vector<CircleLSQ *> circle;
  Timestamp t0, t1;
};


class Scanalyzer
{
public:
  Scanalyzer(double cluster_thresh, double rhomax,
	     const std::string & comport,
	     unsigned long baudrate,
	     unsigned int usec_cycle);
  Scanalyzer(double cluster_thresh, double rhomax,
	     const std::string & bgmap_filename,
	     const std::string & comport,
	     unsigned long baudrate,
	     unsigned int usec_cycle);
  ~Scanalyzer();
  
  /** \returns true if new data has arrived */
  bool Update(std::ostream * dbg);
  const Scanalysis & GetScanalysis() const;
  const Timestamp & GetCurrentStamp() const;
  bool LoadBackground(std::string fname);
  bool RestartSick() const;
  void GetSickStats(double & tmin_sec, double & tmean_sec,
		    double & tmax_sec) const;
  
  void Draw() const;
  void ConfigureViewport(Viewport & vp) const;
  
private:
  bool InitSick(FILE * sick_dbg);
  void Init(double cluster_thresh, double rhomax);
  void Cluster(std::ostream * dbg);
  void Extract();
  
  static const int scansize = Scanalysis::scansize;
  
  const std::string _comport;
  const unsigned long _baudrate;
  const unsigned int _usec_cycle;
  struct sick_poster_s * _sick_poster;
  const double _cluster_thresh, _rhomax;
  const double _cosphi[scansize], _sinphi[scansize];
  double _bgx[scansize], _bgy[scansize];
  Scanalysis _analysis;
  std::auto_ptr<sfl::Polygon> _valid_zone;
};

#endif // SCANALYZER_HPP
