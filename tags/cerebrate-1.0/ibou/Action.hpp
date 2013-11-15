/* 
 * Copyright (C) 2006 Roland Philippsen <roland dot philippsen at gmx dot net>
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


#include <boost/shared_ptr.hpp>
#include <string>
#include <iosfwd>


struct fmod_s;


namespace ibou {
  
  int32_t deg_to_tick(double deg);  
  double tick_to_deg(int32_t tick);
  
  class Action {
  public:
    virtual ~Action() {}
    virtual void Dump(std::ostream & os) const = 0;
    virtual const char * Do(struct fmod_s * motor, std::ostream * dbg) = 0;
    
    friend std::ostream & operator << (std::ostream & os, const Action & act)
    { act.Dump(os); return os; }
  };
  
  boost::shared_ptr<Action>
  parse_action(const std::string & line, std::ostream & os);
  
}
