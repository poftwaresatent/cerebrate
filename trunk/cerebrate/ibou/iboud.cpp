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
#include <drivers/util.h>
#include <drivers/fmod_ipdcmot.h>
#include <sfl/numeric.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>


#define DEFAULT_CONFIG "/etc/iboud.conf"
#define DEFAULT_LOGFILE "/var/log/iboud"
#define DEFAULT_PIDFILE "/var/run/iboud.pid"


using namespace ibou;
using namespace sfl;
using namespace boost;
using namespace std;


typedef vector<shared_ptr<Action> > sequence_t;
typedef void (*triggerfunc_t)();


struct arg_s {
  arg_s(): config_fname(DEFAULT_CONFIG), log_fname(DEFAULT_LOGFILE),
	   pid_fname(DEFAULT_PIDFILE),
	   help(false), version(false), verbose(false), dryrun(false),
	   test(false) {}
  char *config_fname, *log_fname, *pid_fname;
  bool help, version, verbose, dryrun, test;
};

struct cfg_s {
  cfg_s(): host("lechts"), port(8010), kp(1.5), ki(0.01), kd(0), imax(2),
	   tspeed_deg(720), acc_deg(720), dec_deg(720), dzone_deg(2),
	   trigger1(0), trigger2(0), perform_homing(false),
	   homing_positive(true), homing_position(0) {}
  string host;
  uint32_t port;
  double kp, ki, kd, imax, tspeed_deg, acc_deg, dec_deg, dzone_deg;
  triggerfunc_t trigger1, trigger2;
  bool perform_homing, homing_positive;
  double homing_position;
};

struct watchdog_s {
  watchdog_s(): stop(false), error(false), thread(new pthread_t()),
		limit1(false), limit2(false), prevlimit1(false),
		prevlimit2(false), servoing(false),
		homing(false), prevhoming(false) {}
  ~watchdog_s() { delete thread; }
  bool stop, error;
  pthread_t * thread;
  bool limit1, limit2, prevlimit1, prevlimit2;
  bool servoing, prevservoing, homing, prevhoming;
};


static shared_ptr<arg_s> parse_args(int *argc, char ** argv);
static shared_ptr<cfg_s> parse_config(const string & cfname, istream & is,
				      sequence_t & sequence);
static fmod_s * alloc_motor(shared_ptr<cfg_s> cfg);
static bool config_motor(fmod_s * mot, double pos, shared_ptr<cfg_s> cfg);
static void handle(int signum);
static void cleanup();
static void usage_message(ostream & os);
static void log_message(const string & msg, bool omit_nl = false);
static void test_console();

static void start_watchdog();
static void * run_watchdog(void * foo);
static void stop_watchdog();

static void fork_shutdown();
static void fake_shutdown();
static void homing(struct fmod_s * mot, bool positive, double position);
static bool change_pos(struct fmod_s * mot, double pos, ostream & os);


static shared_ptr<arg_s> arg;
static shared_ptr<cfg_s> cfg;
static fmod_s * mot;
static shared_ptr<ifstream> config_is;
static shared_ptr<ofstream> log_os;
static sequence_t sequence;
static shared_ptr<watchdog_s> watchdog;


int main(int argc, char ** argv)
{
  if(atexit(cleanup)){
    perror("iboud: atexit() failed");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGINT, handle) == SIG_ERR){
    perror("iboud: signal(SIGINT) failed");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGHUP, handle) == SIG_ERR){
    perror("iboud: signal(SIGHUP) failed");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGTERM, handle) == SIG_ERR){
    perror("iboud: signal(SIGTERM) failed");
    exit(EXIT_FAILURE);
  }
  
  arg = parse_args(&argc, argv);
  if( ! arg){
    usage_message(cerr);
    exit(1);
  }
  if(arg->help){
    usage_message(cout);
    exit(0);
  }
  if(arg->verbose)
    cout << "config file name: " << arg->config_fname << "\n"
	 << "log file name: " << arg->log_fname << "\n"
	 << "PID file name: " << arg->pid_fname << "\n"
	 << "help: " << (arg->help ? "TRUE\n" : "FALSE\n")
	 << "version: " << (arg->version ? "TRUE\n" : "FALSE\n")
	 << "verbose: " << (arg->verbose ? "TRUE\n" : "FALSE\n")
	 << "dryrun: " << (arg->dryrun ? "TRUE\n" : "FALSE\n")
	 << "test: " << (arg->test ? "TRUE\n" : "FALSE\n");
  if(arg->version){
    cout << "iboud version $Id:$\n";
    exit(0);
  }
  
  config_is.reset(new ifstream(arg->config_fname));
  if(!config_is || !(*config_is)){
    cerr << "couldn't open config file " << arg->config_fname << "\n";
    exit(2);
  }
  log_os.reset(new ofstream(arg->log_fname, ios_base::app));
  if(!log_os || !(*log_os)){
    cerr << "couldn't open log file " << arg->log_fname << "\n";
    exit(3);
  }
  
  cfg = parse_config(arg->config_fname, *config_is, sequence);
  if( ! cfg){
    log_message("error parsing configuration");
    exit(4);
  }
  
  { // PID file
    ofstream pid_os(arg->pid_fname);
    ostringstream os;
    if( ! pid_os){
      os << "error creating PID file \"" << arg->pid_fname << "\": "
	 << strerror(errno);
      log_message(os.str());
      exit(5);
    }
    else{
      pid_os << getpid() << "\n";
      if( ! pid_os){
	os << "error writing to PID file \"" << arg->pid_fname << "\": "
	   << strerror(errno);
	log_message(os.str());
	exit(6);
      }
      os << "wrote PID file " << arg->pid_fname;
      if(arg->verbose)
	cout << os.str() << "\n";
      log_message(os.str());
    }
  }
  
  if( ! arg->dryrun){
    mot = alloc_motor(cfg);
    if( ! mot){
      log_message("error allocating motor");
      exit(5);
    }
    if( ! config_motor(mot, 0, cfg)){
      log_message("error configuring motor");
      exit(6);
    }
    start_watchdog();
    if(cfg->perform_homing && ( ! arg->test)){
      homing(mot, cfg->homing_positive, cfg->homing_position);
      if( ! config_motor(mot, cfg->homing_position, cfg)){
	log_message("error configuring motor after homing");
	exit(7);
      }
    }
  }
  else if(arg->verbose)
    log_message("dry run mode, don't initialize motor, no watchdog");
  
  if(arg->test)
    test_console();
  
  if(sequence.empty()){
    log_message("error: no actions defined");
    exit(8);
  }
  if(arg->verbose){
    ostringstream os;
    os << "actions:\n";
    for(size_t ii(0); ii < sequence.size(); ++ii)
      os << "  " << *sequence[ii] << "\n";
    log_message(os.str(), true);
  }
  
  sequence_t::iterator is(sequence.begin());
  ostream * dbg_os(0);
  if(arg->verbose)
    dbg_os = &cout;
  while(1){
    const char * msg((*is)->Do(mot, dbg_os));
    if(0 != msg){
      ostringstream os;
      os << "error on action \"" << **is << "\": " << msg << "\n";
      log_message(os.str());
      exit(9);
    }
    ++is;
    if(sequence.end() == is)
      is = sequence.begin();
    if(watchdog){
      usleep(50000);
      while(watchdog->servoing)
	usleep(100000);
    }
  }
}


shared_ptr<arg_s> parse_args(int *argc, char ** argv)
{
  const struct option longopts[] = {
    {"config-file", required_argument, 0, 'c'},
    {"log-file",    required_argument, 0, 'l'},
    {"pid-file",    required_argument, 0, 'p'},
    {"help",        no_argument,       0, 'h'},
    {"version",     no_argument,       0, 'v'},
    {"verbose",     no_argument,       0, 'd'},
    {"dryrun",      no_argument,       0, 'r'},
    {"test",        no_argument,       0, 't'},
    {0,             0,                 0, 0}
  };
  const char *shortopts("c:l:p:hvdrt");
  int ch;
  shared_ptr<arg_s> result(new arg_s);
  while(-1 != (ch = getopt_long(*argc, argv, shortopts, longopts, 0))){
    switch(ch){
    case 'c':
      result->config_fname = optarg;
      break;
    case 'l':
      result->log_fname = optarg;
      break;
    case 'p':
      result->pid_fname = optarg;
      break;
    case 'h':
      result->help = true;
      break;
    case 'v':
      result->version = true;
      break;
    case 'd':
      result->verbose = true;
      break;
    case 'r':
      result->dryrun = true;
      break;
    case 't':
      result->test = true;
      break;
    case '?':
    default:
      usage_message(cerr);
      exit(10);
    }
  }
  *argc -= optind;
  argv += optind;
  return result;
}


void handle(int signum)
{
  if(arg){
    ostringstream os;
    os << "caught signal " << signum;
    log_message(os.str());
  }
  exit(-signum);
}


void cleanup()
{
  if(arg)
    log_message("cleaning up");
  if(watchdog)
    stop_watchdog();
  if(mot){
    const int res(fmod_ipdcmot_wmode(mot, FMOD_IPDCMOT_BRAKE));
    if(FMOD_OK != res){
      ostringstream os;
      os << "warning: fmod_ipdcmot_wmode(BRAKE) failed: " << fmod_errstr(res);
      log_message(os.str());
    }
    tcp_close(mot->fd);
    fmod_delete(mot);
  }
}


void usage_message(ostream & os)
{
  os << "iboud [-hvdr] [-c config-file] [-l log-file] [-p pid-file]\n"
     << "  -c --config-file filename   specify configuration (default "
     << DEFAULT_CONFIG << ")\n"
     << "  -l --log-file    filename   specify log file (default "
     << DEFAULT_LOGFILE << ")\n"
     << "  -p --pid-file    filename   specify PID file (default "
     << DEFAULT_PIDFILE << ")\n"
     << "  -h --help                   print usage message\n"
     << "  -v --version                print version\n"
     << "  -d --verbose                enable debug messages\n"
     << "  -r --dryrun                 disable communication with IPDCMOT\n"
     << "  -t --test                   test (console) mode\n";
}


void log_message(const string & msg, bool omit_nl)
{
  if(( ! log_os) || ( ! *log_os))
    return;
  
  const time_t now(time(0));
  if(-1 == now)
    (*log_os) << "(time error " << strerror(errno) << ")";
  else{				//     0         1         2
    char tbuf[26];		//     0123456789012345678901234.5.
    ctime_r(&now, tbuf);	// ex "Thu Nov 24 18:22:48 1986\n\0"
    tbuf[24] = '\0';
    (*log_os) << tbuf;
  }
  (*log_os) << " [" << getpid() << "] " << msg;
  if( ! omit_nl)
    (*log_os) << "\n";
  (*log_os) << flush;
}


shared_ptr<cfg_s> parse_config(const string & cfname, istream & is,
			       sequence_t & sequence)
{
  shared_ptr<cfg_s> cfg(new cfg_s());
  sequence.clear();
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
      tls >> cfg->host;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse host from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "port"){
      tls >> cfg->port;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse port from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "kp"){
      tls >> cfg->kp;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse kp from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "ki"){
      tls >> cfg->ki;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse ki from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "kd"){
      tls >> cfg->kd;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse kd from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "imax"){
      tls >> cfg->imax;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse imax from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "tspeed_deg"){
      tls >> cfg->tspeed_deg;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse tspeed_deg from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "acc_deg"){
      tls >> cfg->acc_deg;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse acc_deg from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "dec_deg"){
      tls >> cfg->dec_deg;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse dec_deg from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "dzone_deg"){
      tls >> cfg->dzone_deg;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse dzone_deg from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "limit"){
      size_t num;
      string action;
      tls >> num >> action;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse limit from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
      if(num > 2){
	ostringstream os;
	os << cfname << ":" << linenum << ": Limit \""
	   << textline << "\" need number = 1 or 2 (got " << num << ")";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
      if("shutdown" == action){
	if(1 == num) cfg->trigger1 = fork_shutdown;
	else         cfg->trigger2 = fork_shutdown;
      }
      else if("fake_shutdown" == action){
	if(1 == num) cfg->trigger1 = fake_shutdown;
	else         cfg->trigger2 = fake_shutdown;
      }
      else{
	ostringstream os;
	os << cfname << ":" << linenum << ": Limit \""
	   << textline << "\" invalid trigger name \"" << action << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
    }
    
    else if(token == "homing"){
      string direction;
      tls >> direction >> cfg->homing_position;
      if( ! tls){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse homing from \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
      if("positive" == direction)
	cfg->homing_positive = true;
      else if("negative" == direction)
	cfg->homing_positive = false;
      else{
	ostringstream os;
	os << cfname << ":" << linenum << ": Homing \""
	   << textline << "\"";
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
      cfg->perform_homing = true;
    }
    
    else{
      ostringstream oss;
      shared_ptr<Action> act(parse_action(textline, oss));
      if( ! act){
	ostringstream os;
	os << cfname << ":" << linenum << ": Couldn't parse action from \""
	   << textline << "\": " << oss.str();
	log_message(os.str());
	return shared_ptr<cfg_s>();
      }
      sequence.push_back(act);
    }
  }
  return cfg;
}


fmod_s * alloc_motor(shared_ptr<cfg_s> cfg)
{
  if(0 != mot){
    tcp_close(mot->fd);
    fmod_delete(mot);
  }
  const int fd(tcp_open(cfg->port, cfg->host.c_str()));
  if(fd < 0){
    ostringstream os;
    os << "tcp_open() error \"" << strerror(errno) << "\"";
    log_message(os.str());
    return 0;
  }
  return fmod_new(fd);
}


bool config_motor(fmod_s * mot, double pos, shared_ptr<cfg_s> cfg)
{
  int res;
  res = fmod_ipdcmot_wkp(mot, cfg->kp);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wkp() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wki(mot, cfg->ki);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wki() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wkd(mot, cfg->kd);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wkd() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wimax(mot, cfg->imax);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wimax() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wtspeed(mot, deg_to_tick(cfg->tspeed_deg));
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wtspeed() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wacc(mot, deg_to_tick(cfg->acc_deg));
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wacc() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wdec(mot, deg_to_tick(cfg->dec_deg));
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wdec() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wdzone(mot, deg_to_tick(cfg->dzone_deg));
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wdzone() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wpos(mot, deg_to_tick(pos));
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wpos() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_win(mot, deg_to_tick(pos));
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_win() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wmode(mot, FMOD_IPDCMOT_POSCTRL);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wmode() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wlim1setup(mot,
				FMOD_IPDCMOT_LIMIT_ENABLE |
				FMOD_IPDCMOT_LIMIT_INUNUSED);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wlim1setup() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  res = fmod_ipdcmot_wlim2setup(mot,
				FMOD_IPDCMOT_LIMIT_ENABLE |
				FMOD_IPDCMOT_LIMIT_INUNUSED);
  if(FMOD_OK != res){
    ostringstream os;
    os << "fmod_ipdcmot_wlim2setup() error \"" << fmod_errstr(res) << "\"";
    log_message(os.str());
    return false;
  }
  return true;
}


void test_console()
{
  string cmdline;
  shared_ptr<istringstream> cmdline_is;
  string cmd("help");
  while(true){
    if(cmd == "quit")
      exit(EXIT_SUCCESS);
    else if(cmd == "help")
      cout << "  quit                 exit iboud\n"
	   << "  help                 this message\n"
	   << "  home <fwd|bwd> <pos> homing sequence\n"
	   << "  rpos               show current position [deg]\n"
	   << "  wpos <value>       set current position [deg]\n"
	   << "  <action string>    parse and execute action\n";
    else if(cmd == "rpos"){
      int32_t pos;
      const int res(fmod_ipdcmot_rpos(mot, &pos));
      if(FMOD_OK != res)
	cout << "fmod_ipdcmot_rpos(): " << fmod_errstr(res) << "\n";
      else
	cout << "tick_to_deg(" << pos << ") = " << tick_to_deg(pos) << "\n"
	     << "position: " << tick_to_deg(pos) << " [deg]\n";
    }
    else if(cmd == "home"){
      string dir;
      double position;
      if( ! ((*cmdline_is) >> dir >> position))
	cout << "error reading direction and position from command line\n";
      else{
	if(("fwd" != dir) && ("bwd" != dir))
	  cout << "error: direction must be \"fwd\" or \"bwd\"\n";
	else{
	  cout << "starting homing (can take a while)\n";
	  if("fwd" == dir)
	    homing(mot, true, position);
	  else
	    homing(mot, false, position);
	  cout << "homing finshed, reconfiguring\n";
	  if( ! config_motor(mot, deg_to_tick(position), cfg))
	    cout << "error configuring motors\n";
	  else
	    cout << "OK\n";
	}
      }
    }
    else if(cmd == "wpos"){
      double pos;
      if( ! ((*cmdline_is) >> pos))
	cout << "error reading position from command line\n";
      else if(change_pos(mot, pos, cout))
	cout << "OK\n";
    }
    else{
      shared_ptr<Action> action(parse_action(cmdline, cout));
      if(!action)
	cout << "\nsyntax error\n";
      else{
	const char * msg(action->Do(mot, &cout));
	if(0 != msg)
	  cout << "error on action \"" << *action << "\": " << msg << "\n";
	else
	  cout << "OK\n";
      }
    }
    cout << "iboud test> ";
    if( ! getline(cin, cmdline)){
      cout << "ERROR reading command\n";
      exit(EXIT_FAILURE);
    }
    cmdline_is.reset(new istringstream(cmdline));
    (*cmdline_is) >> cmd;
  }
}


void start_watchdog()
{
  if(watchdog)
    return;
  watchdog.reset(new watchdog_s());
  if(0 != pthread_create(watchdog->thread, 0, run_watchdog, 0)){
    ostringstream os;
    os << "pthread_create() failed: " << strerror(errno);
    log_message(os.str());
    watchdog.reset();
    exit(EXIT_FAILURE);
  }
  log_message("watchdog started");
}


void * run_watchdog(void * foo)
{
  if( ! watchdog){
    log_message("error: run_watchdog() called without watchdog");
    exit(EXIT_FAILURE);
  }
  if( ! arg){
    log_message("error: run_watchdog() called without arg");
    exit(EXIT_FAILURE);
  }
  if( ! cfg){
    log_message("error: run_watchdog() called without cfg");
    exit(EXIT_FAILURE);
  }
  if(0 == mot){
    log_message("error: run_watchdog() called without motor");
    exit(EXIT_FAILURE);
  }
  
  uint32_t warn;
  int res(fmod_ipdcmot_rwarn(mot, & warn));
  if(FMOD_OK != res){
    ostringstream os;
    os << "watchdog: fmod_ipdcmot_rwarn() failed: " << fmod_errstr(res);
    log_message(os.str());
    watchdog->error = true;
    exit(EXIT_FAILURE);
  }
  const bool invert_limit1(warn & FMOD_IPDCMOT_WARN_LIMIT1);
  const bool invert_limit2(warn & FMOD_IPDCMOT_WARN_LIMIT2);
  
  while( ! watchdog->stop){
    res = fmod_ipdcmot_rwarn(mot, & warn);
    if(FMOD_OK != res){
      ostringstream os;
      os << "watchdog: fmod_ipdcmot_rwarn() failed: " << fmod_errstr(res);
      log_message(os.str());
      watchdog->error = true;
      exit(EXIT_FAILURE);
    }
    if(invert_limit1)
      watchdog->limit1 = ! (warn & FMOD_IPDCMOT_WARN_LIMIT1);
    else
      watchdog->limit1 = warn & FMOD_IPDCMOT_WARN_LIMIT1;
    if(invert_limit2)
      watchdog->limit2 = ! (warn & FMOD_IPDCMOT_WARN_LIMIT2);
    else
      watchdog->limit2 = warn & FMOD_IPDCMOT_WARN_LIMIT2;
    watchdog->servoing = warn & FMOD_IPDCMOT_WARN_SERVOING;
    watchdog->homing = warn & FMOD_IPDCMOT_WARN_HOMING;
    
    if(watchdog->limit1 && ( ! watchdog->prevlimit1) && (0 != cfg->trigger1))
      cfg->trigger1();
    if(watchdog->limit2 && ( ! watchdog->prevlimit2) && (0 != cfg->trigger2))
      cfg->trigger2();
    
    if(arg->verbose){
      if(watchdog->limit1 && ( ! watchdog->prevlimit1))
	log_message("limit1 ON");
      if(( ! watchdog->limit1) && watchdog->prevlimit1)
	log_message("limit1 OFF");
      if(watchdog->limit2 && ( ! watchdog->prevlimit2))
	log_message("limit2 ON");
      if(( ! watchdog->limit2) && watchdog->prevlimit2)
	log_message("limit2 OFF");
      if(watchdog->servoing && ( ! watchdog->prevservoing))
	log_message("servoing ON");
      if(( ! watchdog->servoing) && watchdog->prevservoing)
	log_message("servoing OFF");
      if(watchdog->homing && ( ! watchdog->prevhoming))
	log_message("homing ON");
      if(( ! watchdog->homing) && watchdog->prevhoming)
	log_message("homing OFF");
    }
    
    watchdog->prevlimit1 = watchdog->limit1;
    watchdog->prevlimit2 = watchdog->limit2;
    watchdog->prevservoing = watchdog->servoing;
    watchdog->prevhoming = watchdog->homing;
    
    usleep(50000);
  }
  watchdog->stop = false;
  return 0;
}


void stop_watchdog()
{
  if( ! watchdog)
    return;
  if(watchdog->error){
    if(0 != pthread_cancel(*watchdog->thread)){
      ostringstream os;
      os << "pthread_cancel() failed: " << strerror(errno);
      log_message(os.str());
    }
  }
  else{
    watchdog->stop = true;
    for(int ii(0); ii < 50; ++ii){
      if( ! watchdog->stop)
	break;
      usleep(100000);
    }
    if(watchdog->stop && (0 != pthread_cancel(*watchdog->thread))){
      ostringstream os;
      os << "pthread_cancel() failed: " << strerror(errno);
      log_message(os.str());
    }
  }
  if(0 != pthread_join(*watchdog->thread, 0)){
    ostringstream os;
    os << "pthread_join() failed: " << strerror(errno);
    log_message(os.str());
  }
  watchdog.reset();
  log_message("watchdog stopped");
}


void fork_shutdown()
{
  log_message("fork_shutdown()");
  const pid_t cpid(fork());
  if(cpid == -1){
    ostringstream os;
    os << "fork() FAILED: " << strerror(errno);
    log_message(os.str());
    //    exit(EXIT_FAILURE);
  }
  if(cpid == 0){
    char * xpath("shutdown");
    char * xargv[] = { "shutdown", "-h", "now", 0 };
    execvp(xpath, xargv);
    ostringstream os;
    os << "execvp() FAILED: " << strerror(errno);
    log_message(os.str());
    //    exit(EXIT_FAILURE);
  }
}


void fake_shutdown()
{
  log_message("fake_shutdown()");
}


void homing(struct fmod_s * mot, bool positive, double position)
{
  if( ! watchdog){
    log_message("homing impossible without watchdog!\n");
    exit(EXIT_FAILURE);
  }

  log_message("start homing");
  int status;
  if(positive)
    status = fmod_wreg32(mot, 1234, 0x48, 0x00000b09, 0);
  else
    status = fmod_wreg32(mot, 1234, 0x48, 0x00000b08, 0);
  if(FMOD_OK != status){
    ostringstream os;
    os << "homing: fmod_wreg32() failed: " << fmod_errstr(status);
    log_message(os.str());
    exit(EXIT_FAILURE);
  }
  uint8_t paranoid(0);
  status = fmod_wreg(mot, 1234, 0x49, &paranoid, 0, 0);
  if(FMOD_OK != status){
    ostringstream os;
    os << "homing: fmod_wreg() failed: " << fmod_errstr(status);
    log_message(os.str());
    exit(EXIT_FAILURE);
  }
  usleep(100000);
  while(watchdog->homing)
    usleep(200000);
  log_message("end homing");
}


bool change_pos(struct fmod_s * mot, double pos, ostream & os)
{
  uint8_t mode;
  int res(fmod_ipdcmot_rmode(mot, &mode));
  if(FMOD_OK != res){
    os << "fmod_ipdcmot_rmode(): " << fmod_errstr(res) << "\n";
    return false;
  }
  res = fmod_ipdcmot_wmode(mot, FMOD_IPDCMOT_WAIT);
  if(FMOD_OK != res){
    cout << "fmod_ipdcmot_wmode(): " << fmod_errstr(res) << "\n";
    return false;
  }
  res = fmod_ipdcmot_wpos(mot, deg_to_tick(pos));
  if(FMOD_OK != res){
    cout << "fmod_ipdcmot_wpos(): " << fmod_errstr(res) << "\n";
    return false;
  }
  res = fmod_ipdcmot_win(mot, deg_to_tick(pos));
  if(FMOD_OK != res){
    os << "fmod_ipdcmot_win(): " << fmod_errstr(res) << "\n";
    return false;
  }
  res = fmod_ipdcmot_wmode(mot, mode);
  if(FMOD_OK != res){
    cout << "fmod_ipdcmot_wmode(): " << fmod_errstr(res) << "\n";
    return false;
  }
  return true;
}
