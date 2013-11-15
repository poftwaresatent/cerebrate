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


#include "Action.hpp"
#include <util/Random.hpp>
#include <sfl/numeric.hpp>
#include <drivers/fmod_ipdcmot.h>
#include <boost/current_function.hpp>
#include <iostream>
#include <sstream>


using namespace sfl;
using namespace boost;
using namespace std;


namespace ibou {
  
  
  // constant to convert degrees to ticks
  //    *   4          quadrature
  //    * 512          tick per rev
  //    *  43          gear reduction
  //    / 360          rev per deg
  int32_t deg_to_tick(double deg)
  { return static_cast<int32_t>(rint(deg * 4.0 * 512.0 * 43.0 / 360.0)); }
  
  double tick_to_deg(int32_t tick)
  { return tick / 4.0 / 512.0 / 43.0 * 360.0; }
  
  
  class Value {
  public:
    virtual ~Value() {}
    virtual double Get() = 0;
    virtual void Dump(ostream & os) const = 0;
    
    friend ostream & operator << (ostream & os, const Value & val)
    { val.Dump(os); return os; }
  };
  
  
  class DetVal: public Value {
  public:
    DetVal(double val): m_val(val) {}
    double Get() { return m_val; }
    void Dump(ostream & os) const { os << m_val; }
    
    double m_val;
  };
  
  
  class RandVal: public Value {
  public:
    RandVal(double vmin, double vmax): m_vmin(minval(vmin, vmax)),
				       m_vmax(maxval(vmin, vmax)) {}
    double Get() { return Random::Uniform(m_vmin, m_vmax); }
    void Dump(ostream & os) const
    { os << "rand(" << m_vmin << ", " << m_vmax << ")"; }
    
    double m_vmin, m_vmax;
  };
  
  
  class Move: public Action {
  public:
    Move(shared_ptr<Value> speed_deg, shared_ptr<Value> pos_deg):
      m_speed_deg(speed_deg), m_pos_deg(pos_deg) {}
    
    void Dump(ostream & os) const
    { os << "move(" << *m_speed_deg << ", " << *m_pos_deg << ")"; }
    
    const char * Do(struct fmod_s * motor, ostream * dbg) {
      const double pos(m_pos_deg->Get());
      const double speed(m_speed_deg->Get());
      if(0 == motor)
	cout << "DRYRUN move: pos " << pos << " speed " << speed << "\n";
      else{
	if(0 != dbg)
	  (*dbg) << "move: pos " << pos << " speed " << speed << "\n";
	int res;
	res = fmod_ipdcmot_wmode(motor, FMOD_IPDCMOT_WAIT);
	if(FMOD_OK != res) return "error writing WAIT mode";
	res = fmod_ipdcmot_win(motor, deg_to_tick(pos));
	if(FMOD_OK != res) return "error writing INPUT register";
	res =
	  fmod_ipdcmot_wtspeed(motor, deg_to_tick(speed));
	if(FMOD_OK != res) return "error writing TOPSPEED register";
	res = fmod_ipdcmot_wmode(motor, FMOD_IPDCMOT_POSCTRL);
	if(FMOD_OK != res) return "error writing POSCTRL mode";
      }
      return 0;
    }
    
    shared_ptr<Value> m_speed_deg, m_pos_deg;
  };
  
  
  class Pause: public Action {
  public:
    Pause(shared_ptr<Value> minutes, shared_ptr<Value> seconds):
      m_minutes(minutes), m_seconds(seconds) {}
    
    void Dump(ostream & os) const
    { os << "pause(" << *m_minutes << ", " << *m_seconds << ")"; }
    
    const char * Do(struct fmod_s * motor, ostream * dbg){
      if(0 == motor)
	cout << "DRYRUN pause: min " << m_minutes->Get()
	     << " sec " << m_seconds->Get() << "\n";
      else{
	const unsigned int
	  us(static_cast<unsigned int>
	     (ceil(1000000 * (60 * m_minutes->Get() + m_seconds->Get()))));
	if(0 != dbg) (*dbg) << "  sleep " << us << "us\n";
	usleep(us);
      }
      return 0;
    }
    
    shared_ptr<Value> m_minutes, m_seconds;
  };
  
  
  shared_ptr<Value> parse_value(istream & is, ostream & os)
  {
    shared_ptr<Value> result;
    string token;
    if( ! (is >> token)){
      os << "error reading value token";
      return result;
    }
    if(token == "rand"){
      double vmin, vmax;
      if( ! (is >> vmin >> vmax)){
	os << "error reading vmin and/or vmax";
	return result;
      }
      result.reset(new RandVal(vmin, vmax));
    }
    else{
      istringstream iss(token);
      double val;
      if( ! (iss >> val)){
	os << "error reading value from \"" << token << "\"";
	return result;
      }
      result.reset(new DetVal(val));
    }
    return result;
  }
  
  
  shared_ptr<Action> parse_action(const string & line, ostream & os)
  {
    shared_ptr<Action> result;
    istringstream is(line);
    string token;
    if( ! (is >> token)){
      os << "error reading action token";
      return result;
    }
    if(token == "move"){
      ostringstream oss;
      shared_ptr<Value> speed(parse_value(is, oss));
      if( ! speed){
	os << "value error on speed: " << oss.str();
	return result;
      }
      shared_ptr<Value> position(parse_value(is, oss));
      if( ! position){
	os << "value error on position: " << oss.str();
	return result;
      }
      result.reset(new Move(speed, position));
    }
    else if(token == "pause"){
      ostringstream oss;
      shared_ptr<Value> minutes(parse_value(is, oss));
      if( ! minutes){
	os << "value error on minutes: " << oss.str();
	return result;
      }
      shared_ptr<Value> seconds(parse_value(is, oss));
      if( ! seconds){
	os << "value error on seconds: " << oss.str();
	return result;
      }
      result.reset(new Pause(minutes, seconds));
    }
    else{
      os << "invalid token: " << token;
      return result;
    }
    return result;
  }
  
}
