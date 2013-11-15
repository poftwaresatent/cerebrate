// g++ -o tfork -Wall tfork.cpp

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
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <signal.h>
// #include <sys/types.h>
// #include <unistd.h>


using namespace boost;
using namespace std;


static void cleanup();
static void handle(int signum);
static void log_message(const string & msg);


int main(int argc, char ** argv)
{
  log_message("started");
  
  if(atexit(cleanup)){
    perror("atexit()");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGINT, handle) == SIG_ERR){
    perror("signal(SIGINT)");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGHUP, handle) == SIG_ERR){
    perror("signal(SIGHUP)");
    exit(EXIT_FAILURE);
  }
  if(signal(SIGTERM, handle) == SIG_ERR){
    perror("signal(SIGTERM)");
    exit(EXIT_FAILURE);
  }
  
  log_message("initialized");
  
  const pid_t cpid(fork());
  if(cpid == -1){
    ostringstream os;
    os << "fork() FAILED: " << strerror(errno);
    log_message(os.str());
    exit(EXIT_FAILURE);
  }
  if(cpid == 0){
    for(int ii(3); ii > 0; ii--){
      ostringstream os;
      os << "shutdown in " << ii << " seconds";
      log_message(os.str());
      usleep(1000000);
    }
    char * xpath("shutdown");
    char * xargv[] = { "shutdown", "-r", "now", "\"just testing\"", 0 };
    execvp(xpath, xargv);
    ostringstream os;
    os << "execvp() FAILED: " << strerror(errno);
    log_message(os.str());
    exit(EXIT_FAILURE);
  }
  
  while(true){
    log_message("still alive");
    usleep(1000000);
  }
}


void cleanup()
{
  log_message("cleaning up");
}


void log_message(const string & msg)
{
  ofstream os("tfork.log", ios::app);
  const time_t now(time(0));
  if(-1 == now)
    os << "(time error " << strerror(errno) << ")";
  else{				//     0         1         2
    char tbuf[26];		//     0123456789012345678901234.5.
    ctime_r(&now, tbuf);	// ex "Thu Nov 24 18:22:48 1986\n\0"
    tbuf[24] = '\0';
    os << tbuf;
  }
  os << " [" << getpid() << "] " << msg << "\n" << flush;
  os.close();
}


void handle(int signum)
{
  ostringstream os;
  os << "caught signal " << signum;
  log_message(os.str());
  exit(-signum);
}
