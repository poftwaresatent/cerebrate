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



#ifndef BLINK_HPP
#define BLINK_HPP


#include <boost/scoped_ptr.hpp>
#include <iosfwd>
#include <vector>
#include <map>
#include <string>
#include <stdint.h>


namespace local {
  struct poster;
  struct channel;
  struct pattern;
  struct shutdown;
}


class Blink
{
public:
  Blink();
  ~Blink();
  
  bool ParseConfig(std::istream & is, std::ostream & os);
  void DumpConfig(std::ostream & os) const;
  
  const char * StartThread(bool dryrun, std::ostream * os);
  void StopThread();
  
  bool Step();
  double GetPeriodMS() const { return m_current_period_ms; }
  
  /** \return -1 if no pause for current step */
  double GetPause() const;
  
  enum shutdown_t { DISABLED, KEEP_RUNNING, SHUTDOWN, ERROR };
  shutdown_t QueryShutdown(bool force_read, bool dryrun) const;
  
  std::ostream * m_os;
  
private:
  typedef std::map<size_t, local::channel> channel_t;
  
  size_t m_index;
  std::string m_host;
  uint32_t m_port;
  double m_current_period_ms;
  channel_t m_channel;
  std::vector<local::pattern> m_sequence;
  mutable std::map<size_t, double> m_pause_ms;
  mutable std::map<size_t, double> m_period_ms;
  char m_true_char;
  char m_false_char;
  boost::scoped_ptr<local::poster> m_poster;
  int m_fd;
  struct fmod_s * m_fs;
  bool m_dryrun, m_cleanup_state;
  boost::scoped_ptr<local::shutdown> m_shutdown;
};

#endif // BLINK_HPP
