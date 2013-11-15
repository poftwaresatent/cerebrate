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


#include "Blink.hpp"
#include <sfl/pdebug.hpp>
#include <drivers/util.h>
#include <drivers/fmod_tcp.h>
#include <sstream>
#include <map>
#include <errno.h>
#include <math.h>

#define PDEBUG PDEBUG_ERR
#define PVDEBUG PDEBUG_OFF


using namespace boost;
using namespace std;


namespace local {
  
  typedef enum { RUNNING, WAIT, QUIT } state_t;
  
  static void * run(void * arg);
  
  struct poster {
    pthread_t thread_id;
    pthread_mutex_t mutex;
    state_t state;
    Blink * blink;
    
    poster(Blink * _blink, bool & ok)
      : state(WAIT), blink(_blink)
    {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
      pthread_mutex_init(&mutex, &attr);
      pthread_mutexattr_destroy(&attr);
      switch(pthread_create(&thread_id, 0, run, this)){
      case EAGAIN:
	if(0 != blink->m_os)
	  (*blink->m_os) << "insufficient resources for thread.\n";
	ok = false;
	break;
      case EINVAL:
	if(0 != blink->m_os)
	  (*blink->m_os) << "BUG [invalid attr for pthread_create()]\n";
	ok = false;
	break;
      default:
	ok = true;
      }
    }
    
    ~poster() {
      state = QUIT;
      char * result;
      pthread_join(thread_id, reinterpret_cast<void**>(&result));
      if(result != 0)
	if(0 != blink->m_os)
	  (*blink->m_os) << "WARNING thread result \"" << result << "\"\n";
      pthread_mutex_destroy(&mutex);
    }
  };
  
  struct channel {
    channel() {}
    channel(int _ionum, size_t _bitnum)
      : ionum(_ionum), bitnum(_bitnum), mask(1 << _bitnum) {}
    int ionum;
    size_t bitnum;
    uint8_t mask;
  };
  
  struct pattern {
    typedef map<int, uint8_t> map_t;
    pattern() {}
    explicit pattern(const map_t & _mask): mask(_mask) {}
    map_t mask;
  };

  struct shutdown {
    shutdown(int _ionum, uint8_t _mask, bool _invert)
      : ionum(_ionum), mask(_mask), invert(_invert) {}
    const int ionum;
    const uint8_t mask;
    const bool invert;
  };
  
}


using namespace local;


Blink::
Blink()
  : m_os(0),
    m_index(0),
    m_host("192.168.0.5"),
    m_port(8010),
    m_true_char('*'),
    m_false_char('.'),
    m_fd(-1),
    m_fs(0),
    m_dryrun(false),
    m_cleanup_state(false)
{
}


Blink::
~Blink()
{
  if(0 > m_fd){
    PVDEBUG("nothing to clean up.\n");
    return;
  }
  if(0 == m_fs){
    PVDEBUG("no fmod_s to clean up.\n");
    return;
  }
  if(( ! m_dryrun) && ( ! m_sequence.empty())){
    uint8_t mask(0);
    if(m_cleanup_state)
      mask = 0xFF;
    const pattern & pp(m_sequence[0]);
    for(pattern::map_t::const_iterator ip(pp.mask.begin());
	ip != pp.mask.end(); ++ip){
      const int res(fmod_tcp_wio(m_fs, ip->first, mask, 0));
      if((FMOD_OK != res) && (0 != m_os))
	PDEBUG("warning: fmod_tcp_wio() error %d on ionum %d.\n",
	       res, ip->first);
    }
  }
  if(tcp_close(m_fd) != 0)
    PDEBUG("warning: tcp_close() failed.\n");
  fmod_delete(m_fs);
  PVDEBUG("all OK\n");
}


bool Blink::
ParseConfig(istream & is, ostream & os)
{
  m_channel.clear();
  m_sequence.clear();
  m_pause_ms.clear();
  m_period_ms.clear();
  string textline;
  size_t linenum(0);
  while(getline(is, textline)){
    ++linenum;
    istringstream tls(textline);
    string token;
    tls >> token;
    if(( ! tls) || ('#' == token[0]))
      continue;
    
    if(token == "host"){
      tls >> m_host;
      if( ! tls){
	os << linenum << ": Couldn't parse host from \"" << tls.str()
	   << "\"\n";
	return false;
      }
    }
    
    else if(token == "port"){
      tls >> m_port;
      if( ! tls){
	os << linenum << ": Couldn't parse port from \"" << tls.str()
	   << "\"\n";
	return false;
      }
    }
    
    else if(token == "bit"){
      size_t data_bitnum;
      int ionum;
      size_t bitnum;
      tls >> data_bitnum >> ionum >> bitnum;
      if( ! tls){
	os << linenum << ": Couldn't parse bit from \"" << tls.str() << "\"\n";
	return false;
      }
      if(7 < bitnum){
	os << linenum << ": Couldn't parse bit from\"" << tls.str()
	   << "\" (need 0<=bitnum<=7)\n";
	return false;
      }
      if(m_shutdown && (m_shutdown->ionum == ionum)
	 && (m_shutdown->mask == (1 << bitnum))){
	os << linenum << ": bit \"" << tls.str()
	   << "\" conflicts with shutdown\n";
	return false;
      }
      m_channel[data_bitnum] = channel(ionum, bitnum);
    }
    
    else if(token == "shutdown"){
      int ionum;
      size_t bitnum;
      string invert_str;
      tls >> ionum >> bitnum >> invert_str;
      if( ! tls){
	os << linenum << ": Couldn't parse shutdown from\"" << tls.str()
	   << "\"\n";
	return false;
      }
      bool invert(false);
      if("true" == invert_str)
	invert = true;
      else if("false" != invert_str){
	os << linenum << ": Couldn't parse shutdown from\"" << tls.str()
	   << "\" (expected true or false)\n";
	return false;
      }
      if(7 < bitnum){
	os << linenum << ": Couldn't parse shutdown from\"" << tls.str()
	   << "\" (need 0<=bitnum<=7)\n";
	return false;
      }
      if(m_shutdown)
	os << linenum << ": Warning: overwriting prior shutdown definition\n";
      for(channel_t::const_iterator ic(m_channel.begin());
	  ic != m_channel.end(); ++ic)
	if((ic->second.ionum == ionum) && (ic->second.bitnum == bitnum)){
	  os << linenum << ": shutdown \"" << tls.str()
	     << "\" conflicts with bit " << ic->first << "\n";
	  return false;
	}
      const uint8_t mask(1 << bitnum);
      m_shutdown.reset(new shutdown(ionum, mask, invert));
    }
    
    else if(token == "true"){
      tls >> m_true_char;
      if( ! tls){
	os << linenum << ": Couldn't parse true_char from \"" << tls.str()
	   << "\"\n";
	return false;
      }
    }
    
    else if(token == "false"){
      tls >> m_false_char;
      if( ! tls){
	os << linenum << ": Couldn't parse false_char from \"" << tls.str()
	   << "\"\n";
	return false;
      }
    }
    
    else if(token == "cleanup"){
      string foo;
      tls >> foo;
      if( ! tls){
	os << linenum << ": Couldn't parse cleanup from \"" << tls.str()
	   << "\"\n";
	return false;
      }
      if(foo == "true")
	m_cleanup_state = true;
      else if(foo == "false")
	m_cleanup_state = false;
      else{
	os << linenum << ": cleanup must be true or false in \"" << tls.str()
	   << "\"\n";
	return false;
      }
    }
    
    else if(token == "data")
      break;
    
    else{
      os << linenum << ": Unhandled line in config file \"" << tls.str()
	 << "\"\n";
      return false;
    }
  }
  
  if(m_true_char == m_false_char){
    os << linenum << ": true_char and false_char are both '" << m_true_char
       << "'.\n";
    return false;
  }
  
  for(size_t ii(0); ii < m_channel.size(); ++ii)
    if(m_channel.find(ii) == m_channel.end()){
      os << linenum << ": bit number " << ii << " not mapped\n";
      return false;
    }
  
  while(getline(is, textline)){
    ++linenum;
    istringstream tls(textline);
    string token;
    tls >> token;
    if(( ! tls) || ('#' == token[0]))
      continue;
    
    if(token == "pause_ms"){
      double pms;
      tls >> pms;
      if(!tls){
	os << linenum << ": Couldn't parse pause_ms from \"" << tls.str()
	   << "\"\n";
	return false;
      }
      m_pause_ms[m_sequence.size()] = pms;
      PVDEBUG("pause_ms index %lu ms %g\n", m_sequence.size(), pms);
    }
    
    else if(token == "period_ms"){
      double ms;
      tls >> ms;
      if( ! tls){
	os << linenum << ": Couldn't parse period_ms from \"" << tls.str()
	   << "\"\n";
	return false;
      }
      m_period_ms[m_sequence.size()] = ms;
    }
    
    else{
      pattern::map_t mask;
      for(size_t ii(0); ii < m_channel.size(); ++ii){
	if(mask.find(m_channel[ii].ionum) == mask.end())
	  mask[m_channel[ii].ionum] = 0;
	if(textline[ii] == m_true_char){
	  mask[m_channel[ii].ionum] |= m_channel[ii].mask;
	}
	else if(textline[ii] != m_false_char){
	  os << linenum << ": Illegal character '" << textline[ii]
	     << "' in data line:  " << textline << "\n";
	  return false;
	}
      }
      m_sequence.push_back(pattern(mask));
      PVDEBUG("data index %lu string %s\n",
	     m_sequence.size(), textline.c_str());
    }
  }
  
  if(m_sequence.empty()){
    os << "Insufficient data section.\n";
    return false;
  }
  
  if(m_period_ms.empty()){
    os << "Insufficient period_ms information.\n";
    return false;
  }
  
  return true;
}


void Blink::
DumpConfig(ostream & os) const
{
  os << "  length:     " << m_sequence.size() << "\n"
     << "  width:      " << m_channel.size() << "\n"
     << "  host:       " << m_host << "\n"
     << "  true_char:  " << m_true_char << "\n"
     << "  false_char: " << m_false_char << "\n"
     << "  channel:\n";
  for(map<size_t, channel>::const_iterator ic(m_channel.begin());
      ic != m_channel.end(); ++ic)
    os << "    [" << ic->first << "]: " << ic->second.ionum << " / "
       << ic->second.bitnum << " (" << static_cast<int>(ic->second.mask)
       << ")\n";
  os << "  sequence:\n";
  for(size_t ii(0); ii < m_sequence.size(); ++ii){
    if(m_pause_ms.find(ii) != m_pause_ms.end())
      os << "    pause_ms " << m_pause_ms[ii] << "\n";
    if(m_period_ms.find(ii) != m_period_ms.end())
      os << "    period_ms " << m_period_ms[ii] << "\n";
    os << "    [" << ii << "]:";
    const pattern & pp(m_sequence[ii]);
    for(pattern::map_t::const_iterator ip(pp.mask.begin());
	ip != pp.mask.end(); ++ip){
      os << " ";
      for(ssize_t ib(7); ib >= 0; --ib)
	if(ip->second & (1 << ib))
	  os << m_true_char;
	else
	  os << m_false_char;
    }
    os << "\n";
  }
}


double Blink::
GetPause() const
{
  if(m_pause_ms.find(m_index) == m_pause_ms.end())
    return -1;
  return m_pause_ms[m_index];
}


bool Blink::
Step()
{
  if(m_poster){
    PVDEBUG("lock...\n");
    if(EINVAL == pthread_mutex_lock(&m_poster->mutex)){
      if(0 != m_os)
	(*m_os) << "Blink::Step(): pthread_mutex_lock() error\n";
      return false;
    }
  }
  
  if(m_index >= m_sequence.size())
    m_index = 0;
  if(0 != m_os)
    (*m_os) << "step [" << m_index << "]:";
  if(m_period_ms.find(m_index) != m_period_ms.end()){
    m_current_period_ms = m_period_ms[m_index];
    PVDEBUG("\nnew period is %g\n", m_current_period_ms);
  }
  const pattern & pp(m_sequence[m_index]);
  for(pattern::map_t::const_iterator ip(pp.mask.begin());
      ip != pp.mask.end(); ++ip){
    if(0 != m_os){
      (*m_os) << " ";
      for(ssize_t ib(7); ib >= 0; --ib)
	if(ip->second & (1 << ib))
	  (*m_os) << m_true_char;
	else
	  (*m_os) << m_false_char;
    }
    if( ! m_dryrun){
      const int res(fmod_tcp_wio(m_fs, ip->first, ip->second, 0));
      if((FMOD_OK != res) && (0 != m_os))
	(*m_os) << "\nfmod_tcp_wio() error \"" << fmod_errstr(res)
		<< "\" on ionum " << ip->first << "\n";
    }
  }
  if(0 != m_os)
    (*m_os) << "\n";
  ++m_index;
  
  if(m_poster){
    PVDEBUG("unlock...\n");
    if(EINVAL == pthread_mutex_unlock(&m_poster->mutex)){
      if(0 != m_os)
	(*m_os) << "Blink::Step(): pthread_mutex_unlock() error\n";
      return false;
    }
  }
  return true;
}


const char * Blink::
StartThread(bool dryrun, ostream * os)
{
  m_dryrun = dryrun;
  m_os = os;
  
  if( ! dryrun){
    if(0 > m_fd)
      m_fd = tcp_open(m_port, m_host.c_str());
    if(0 > m_fd){
      if(0 != os)
	(*os) << "tcp_open() error \"" << strerror(errno) << "\"\n";
      return "tcp_open() error";
    }
    if(0 == m_fs)
      m_fs = fmod_new(m_fd);
    for(int ionum(0); ionum < 3; ++ionum){
      int res;
      if((m_shutdown) && (m_shutdown->ionum == ionum))
	res = fmod_tcp_wiodir(m_fs, ionum, m_shutdown->mask, 0);
      else
	res = fmod_tcp_wiodir(m_fs, ionum, 0, 0);
      if(FMOD_OK != res){
	if(0 != os)
	  (*os) << "fmod_tcp_wiodir() error \"" << fmod_errstr(res)
		<< "\" on ionum " << ionum << "\n";
	return "fmod_tcp_wiodir() error";
      }
    }
  }
  
  bool ok;
  m_poster.reset(new poster(this, ok));
  if(!ok)
    return "error creating poster";
  m_poster->state = RUNNING;
  return 0;
}


Blink::shutdown_t Blink::
QueryShutdown(bool force_read, bool dryrun) const
{
  if(dryrun)
    return KEEP_RUNNING;
  
  if( ! m_shutdown){
    PVDEBUG("no m_shutdown\n");
    if(force_read){
      uint8_t mask;
      const int res(fmod_tcp_rio(m_fs, 0, &mask, 0));
      if(FMOD_OK != res){
	if(0 != m_os)
	  (*m_os) << "fmod_tcp_rio() error \"" << fmod_errstr(res)
		  << "\" on ionum 0\n";
	return ERROR;
      }
    }
    return DISABLED;
  }
  
  if(m_poster){
    PVDEBUG("lock...\n");
    if(EINVAL == pthread_mutex_lock(&m_poster->mutex)){
      if(0 != m_os)
	(*m_os) << "Blink::QueryShutdown(): pthread_mutex_lock() error\n";
      return ERROR;
    }
  }
  
  shutdown_t retval(KEEP_RUNNING);
  uint8_t mask(0);
  const int res(fmod_tcp_rio(m_fs, m_shutdown->ionum, &mask, 0));
  if(FMOD_OK != res){
    if(0 != m_os)
      (*m_os) << "fmod_tcp_rio() error \"" << fmod_errstr(res)
	      << "\" on ionum " << m_shutdown->ionum << "\n";
    retval = ERROR;
  }
  if(m_shutdown->invert)
    mask = ~ mask;
  
  PVDEBUG("mask=0x%02x   shutdown_mask=0x%02x  AND=0x%02x\n",
	  mask, m_shutdown->mask, mask & m_shutdown->mask);
  
  bool active(false);
  if(ERROR != retval)
    active = mask & m_shutdown->mask;
  if(active)
    retval = SHUTDOWN;
  
  if(m_poster){
    PVDEBUG("unlock...\n");
    if(EINVAL == pthread_mutex_unlock(&m_poster->mutex)){
      if(0 != m_os)
	(*m_os) << "Blink::Step(): pthread_mutex_unlock() error\n";
      if(retval != SHUTDOWN)
	retval = ERROR;
    }
  }
  
  if(retval == DISABLED)
    PVDEBUG("retval=DISABLED\n");
  else if(retval == KEEP_RUNNING)
    PVDEBUG("retval=KEEP_RUNNING\n");
  else if(retval == SHUTDOWN)
    PVDEBUG("retval=SHUTDOWN\n");
  else if(retval == ERROR)
    PVDEBUG("retval=ERROR\n");
  
  return retval;
}


void Blink::
StopThread()
{
  m_poster.reset();
}


void * local::
run(void * arg)
{
  poster * pst(reinterpret_cast<poster*>(arg));
  Blink * blink(pst->blink);
  bool quit(false);
  char * result(0);
  while(!quit){
    switch(pst->state){
    case WAIT:
      PVDEBUG("WAIT\n");
      break;
    case QUIT:
      PVDEBUG("QUIT\n");
      quit = true;
      break;
    case RUNNING:
      PVDEBUG("RUNNING\n");
      {
	const double pause_ms(blink->GetPause());
	if(pause_ms > 0)
	  usleep(static_cast<unsigned int>(ceil(pause_ms * 1000)));
      }
      if( ! blink->Step()){
	quit = true;
	result = "Blink::Step() failed";
      }
      break;
    default:
      if(0 != blink->m_os)
	(*blink->m_os) << "invalid poster->state " << pst->state << "\n";
      quit = true;
      result = "invalid poster->state";
    }
    if( ! quit)
      usleep(static_cast<unsigned int>(ceil(blink->GetPeriodMS() * 1000)));
  }
  return result;
}
