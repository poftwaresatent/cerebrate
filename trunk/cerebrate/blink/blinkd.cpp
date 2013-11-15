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
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>


#define DEFAULT_CONFIG "/etc/blinkd.conf"
#define DEFAULT_LOGFILE "/var/log/blinkd"
#define DEFAULT_PIDFILE "/var/run/blinkd.pid"


using namespace boost;
using namespace std;


struct arg_s {
  arg_s(): config_fname(DEFAULT_CONFIG), log_fname(DEFAULT_LOGFILE),
	   pid_fname(DEFAULT_PIDFILE),
	   help(false), version(false), verbose(false), dryrun(false) {}
  char *config_fname, *log_fname, *pid_fname;
  bool help, version, verbose, dryrun;
};


static shared_ptr<arg_s> parse_args(int *argc, char ** argv);
static void handle(int signum);
static void cleanup();
static void usage_message(ostream & os);
static void log_message(const string & msg, bool omit_nl = false);
static void fork_shutdown();


static shared_ptr<Blink> blink;
static shared_ptr<arg_s> arg;
static shared_ptr<ifstream> config_is;
static shared_ptr<ofstream> log_os;


int main(int argc, char ** argv)
{
  if(atexit(cleanup)){
    perror("blinkd: atexit() failed");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGINT, handle) == SIG_ERR){
    perror("blinkd: signal(SIGINT) failed");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGHUP, handle) == SIG_ERR){
    perror("blinkd: signal(SIGHUP) failed");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGTERM, handle) == SIG_ERR){
    perror("blinkd: signal(SIGTERM) failed");
    exit(EXIT_FAILURE);
  }
  
  arg = parse_args(&argc, argv);
  if(!arg){
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
	 << "verbose: " << (arg->verbose ? "TRUE\n" : "FALSE\n");
  if(arg->version){
    cout << "blinkd version 0.0\n";
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
  
  blink.reset(new Blink());  
  ostringstream os;
  if(!blink->ParseConfig(*config_is, os)){
    cerr << "error parsing config file \"" << arg->config_fname << "\":\n"
	 << os.str();
    exit(4);
  }
  
  if(arg->verbose)
    blink->DumpConfig(cout);
  {
    ostringstream os;
    os << "read configuration file \"" << arg->config_fname << "\"";
    log_message(os.str());
    if(arg->verbose && (log_os && (*log_os)))
      blink->DumpConfig(*log_os);
  }
  
  { 
    ofstream pid_os(arg->pid_fname);
    ostringstream os;
    if( ! pid_os){
      os << "error creating PID file \"" << arg->pid_fname << "\": "
	 << strerror(errno);
      cerr << os.str() << "\n";
      log_message(os.str());
      exit(5);
    }
    else{
      pid_os << getpid() << "\n";
      if( ! pid_os){
	os << "error writing to PID file \"" << arg->pid_fname << "\": "
	   << strerror(errno);
	cerr << os.str() << "\n";
	log_message(os.str());
	exit(6);
      }
      os << "wrote PID file " << arg->pid_fname;
      if(arg->verbose)
	cout << os.str() << "\n";
      log_message(os.str());
    }
  }
  
  ostream * bos(0);
  if(arg->verbose)
    bos = &cout;
  const char * res(blink->StartThread(arg->dryrun, bos));
  if(0 != res){
    ostringstream os;
    os << "blink->StartThread() failed: " << res;
    cerr << os.str() << "\n";
    log_message(os.str());
    exit(7);
  }
  
  if(arg->verbose)
    cout << "thread started\n";
  log_message("thread started");
  while(true){
    const Blink::shutdown_t status(blink->QueryShutdown(true, arg->dryrun));
    if(Blink::SHUTDOWN == status){
      static bool shutdown_done(false);
      if( ! shutdown_done){
	if(arg->verbose)
	  cout << "Blink::SHUTDOWN\n";
	fork_shutdown();
	shutdown_done = true;
      }
    }
    else if(Blink::ERROR == status){
      if(arg->verbose)
	cout << "error in Blink::QueryShutdown()\n";
      log_message("error in Blink::QueryShutdown()");
      exit(EXIT_FAILURE);
    }
    else if((Blink::DISABLED != status) && (Blink::KEEP_RUNNING != status)){
      if(arg->verbose)
	cout << "bug: invalid Blink::QueryShutdown() result\n";
      log_message("bug: invalid Blink::QueryShutdown() result");
      exit(EXIT_FAILURE);
    }
    //     else if(arg->verbose)
    //       cout << "Hello from main()!\n";
    usleep(200000);
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
    {0,             0,                 0, 0}
  };
  const char *shortopts("c:l:p:hvdr");
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
    case '?':
    default:
      usage_message(cerr);
      exit(8);
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
    if(arg->verbose)
      cout << os.str() << "\n";
    log_message(os.str());
  }
  exit(-signum);
}


void cleanup()
{
  if(arg){
    if(arg->verbose)
      cout << "cleaning up\n";
    log_message("cleaning up");
  }
}


void usage_message(ostream & os)
{
  os << "blinkd [-hvdr] [-c config-file] [-l log-file] [-p pid-file]\n"
     << "  -c --config-file filename   specify configuration (default "
     << DEFAULT_CONFIG << ")\n"
     << "  -l --log-file    filename   specify log file (default "
     << DEFAULT_LOGFILE << ")\n"
     << "  -p --pid-file    filename   specify PID file (default "
     << DEFAULT_PIDFILE << ")\n"
     << "  -h --help                   print usage message\n"
     << "  -v --version                print version\n"
     << "  -d --verbose                enable debug messages\n"
     << "  -r --dryrun                 disable communication with FMOD\n";
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
