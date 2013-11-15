/* 
 * Copyright (C) 2004
 * Swiss Federal Institute of Technology, Lausanne. All rights reserved.
 * 
 * Developed at the Autonomous Systems Lab.
 * Visit our homepage at http://asl.epfl.ch/
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


#ifndef SUNFLOWER_POLYGON_HPP
#define SUNFLOWER_POLYGON_HPP


#include <sfl/Line.hpp>
#include <sfl/Point.hpp>
#include <sfl/numeric.hpp>
#include <vector>
#include <iosfwd>
#include <stdexcept>


namespace sfl {


  /**
     A simple polygon implementation that allows calculating the
     convex hull, and some more straightforward functions.
  */
  class Polygon
  {
  public:
    typedef std::vector<Point * > pointlist_t;
    
    /**
       Construct an empty polygon.
    */    
    Polygon();

    /**
       Create a copy of an existing polygon.
    */
    Polygon(const Polygon & p);
    
    ~Polygon();

    /**
       Adds the specified point to the polygon.
    */
    void AddPoint(double x, double y);

    /**
       Performs Jarvis' March on the polygon and thus constructs and
       returns the convex hull. Needed prior to using
       Polygon::Contains() and Polygon::CreateGrownPolygon().

       \note For polygons with less than 2 points, it simply returns a
       copy.
    */
    Polygon * CreateConvexHull() const;

    /**
       Calculates the bounding box of the polygon.
    */
    void BoundingBox(double & x0, double & y0, double & x1, double & y1)
      const;

    /**
       \note Only correct for ccw convex hulls constructed by ConvexHull().
    */
    bool Contains(double x, double y) const;

    /** \note Only correct for ccw convex hulls constructed by ConvexHull() */
    Polygon * CreateGrownPolygon(double padding) const;

    /**
       Determines the largest distance from the origin to any point of
       the polygon.
    */
    double CalculateRadius() const;

    inline unsigned int GetNLines() const;
    
    /**
       Returns a copy of the line between the corners (index) and
       (index + 1). The last point is connected with the first one.
    */
    Line GetLine(int index) const throw(std::range_error);

    const Point * GetPoint(unsigned int index) const;
    
    /**
       Writes the corners in human readable format on the provided ostream.
    */
    friend std::ostream & operator<<(std::ostream & os, const Polygon & p);

    
  protected:
    pointlist_t _point;
  };

  
  unsigned int Polygon::
  GetNLines()
    const
  {
    return _point.size();
  }

}

#endif // SUNFLOWER_POLYGON_HPP
