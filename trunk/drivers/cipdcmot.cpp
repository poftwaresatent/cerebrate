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


#include "util.h"
#include "fmod_ipdcmot.h"
#include <iostream>
#include <sstream>
#include <string>
#include <errno.h>
#include <math.h>


using namespace std;


struct keepalive_s {
  struct fmod_s * fs;
  unsigned int seconds;
  FILE * out;
  int stop;
  pthread_t * thread;
};
  

static void cleanup();
static struct keepalive_s * keepalive_new(struct fmod_s * fs,
					  unsigned int seconds,
					  FILE * out);
static void keepalive_delete(struct keepalive_s * ka);
static void * keepalive_run(struct keepalive_s * ka);
static void keepalive_start(struct keepalive_s * ka);
static void keepalive_stop(struct keepalive_s * ka);

static int32_t deg_to_tick(double deg)
{ return static_cast<int32_t>(rint(deg * 4.0 * 512.0 * 43.0 / 360.0)); }


static struct keepalive_s * ka(0);
static struct fmod_s * fs(0);
static int fd(0);
static FILE * dbg = stderr;


int main(int argc,
	 char ** argv)
{
  set_cleanup(cleanup);
  
  string chname;
  if(argc > 1)
    chname = argv[1];
  else
    chname = "lechts";
  
  fd = tcp_open(8010, chname.c_str());
  if(fd < 0)
    return 1;
  fs = fmod_new(fd);
  
  string cmd("help");
  while(true){
    int result;
    int32_t some_int32;
    uint8_t some_uint8;
    double some_double;
    
    if(cmd == "quit")
      break;
    
    else if(cmd == "help")
      cout << "Available commands:\n"
	   << "  quit              exit this program\n"
	   << "  help              write this message\n"
	   << "  confspeed         configure cactus motor\n"
	   << "  confpos           configure ibou motor\n"
	   << "  startka           START keepalive thread\n"
	   << "  stopka            STOP keepalive thread\n"
	   << "  rpos              read POSITION\n"
	   << "  rmode             read MODE\n"
	   << "  brake             enter BRAKE mode\n"
	   << "  dopen             enter DRIVE OPEN mode\n"
	   << "  oloop             enter OPEN LOOP mode\n"
	   << "  wait              enter WAIT mode\n"
	   << "  speed             enter SPEED CONTROL mode\n"
	   << "  pos               enter POSITION CONTROL mode\n"
	   << "  win     <int32>   write INPUT\n"
	   << "  winoff  <int32>   write input OFFSET\n"
	   << "  winmin  <int32>   write MINIMUM input\n"
	   << "  winmax  <int32>   write MAXIMUM input\n"
	   << "  wkp     <double>  write PROPORTIONAL constant\n"
	   << "  wki     <double>  write INTEGRATION constant\n"
	   << "  wkd     <double>  write DERIVATION constant\n"
	   << "  wimax   <double>  write maximum CURRENT\n"
	   << "  wacc    <int32>   write ACCELERATION\n"
	   << "  wdec    <int32>   write DECELERATION\n"
	   << "  wtspeed <int32>   write TOP SPEED\n"
	   << "  rin               read INPUT\n"
	   << "  rinmin            read MINIMUM input\n"
	   << "  rinmax            read MAXIMUM input\n"
	   << "  rpos              read POSITION\n"
	   << "  rspeed            read SPEED\n"
	   << "  rimax             read maximum CURRENT\n"
	   << "  rkp               read PROPORTIONAL constant\n"
	   << "  rki               read INTEGRATION constant\n"
	   << "  rkd               read DERIVATION constant\n"
	   << "  racc              read ACCELERATION\n"
	   << "  rdec              read DECELERATION\n"
	   << "  rtspeed           read TOP SPEED\n";
    
    else if(cmd == "confspeed"){
      double kp = 1.5, ki = 0.01, kd = 0.0, imax = 2.0;
      int32_t tspeed = 100000;
      result = fmod_ipdcmot_confspeed(fs, kp, ki, kd, tspeed, imax, dbg);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_confspeed(): "
	     << fmod_errstr(result) << "\n";
	return 1;
      }
      cout << "Fmod-IPDCMOT configured for cactus.\n";
    }
    
    else if(cmd == "confpos"){
      const double kp(1.5);
      const double ki(0.01);
      const double kd(0.0);
      const double imax(2.0);
      const int32_t tspeed(deg_to_tick(720));
      const int32_t acc(deg_to_tick(720));
      const int32_t dec(deg_to_tick(720));
      const int32_t dzone(deg_to_tick(1));
      const int32_t initpos(0);
      result = fmod_ipdcmot_confpos(fs, kp, ki, kd, imax, tspeed, acc, dec,
				    dzone, initpos, dbg);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_confpos(): "
	     << fmod_errstr(result) << "\n";
	return 1;
      }
      cout << "Fmod-IPDCMOT configured for ibou.\n";
    }
    
    else if(cmd == "startka"){
      uint8_t seconds;
      int result(fmod_rtcptout(fs, & seconds));
      if(FMOD_OK != result){
	cerr << "ERROR: fmod_rtcptout(): " << fmod_errstr(result) << "\n";
	break;
      }
      if(0 == ka)
	ka = keepalive_new(fs, seconds / 4, dbg);
      keepalive_start(ka);
    }
    
    else if(cmd == "stopka"){
      if(0 != ka)
	keepalive_stop(ka);
    }
    
    else if(cmd == "rpos"){
      result = fmod_ipdcmot_rpos(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rpos(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      printf("position: 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rmode"){
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      switch(some_uint8){
      case FMOD_IPDCMOT_BRAKE:     cout << "BRAKE mode\n"; break;
      case FMOD_IPDCMOT_DRIVEOPEN: cout << "DRIVEOPEN mode\n"; break;
      case FMOD_IPDCMOT_OPENLOOP:  cout << "OPENLOOP mode\n"; break;
      case FMOD_IPDCMOT_WAIT:      cout << "WAIT mode\n"; break;
      case FMOD_IPDCMOT_SPEEDCTRL: cout << "SPEEDCTRL mode\n"; break;
      case FMOD_IPDCMOT_POSCTRL:   cout << "POSCTRL mode\n"; break;
      default: printf("invalid mode 0x%02X\n", some_uint8);
      }
    }
    
    else if(cmd == "brake"){
      result = fmod_ipdcmot_wmode(fs, FMOD_IPDCMOT_BRAKE);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_wmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      if(FMOD_IPDCMOT_BRAKE == some_uint8)
	cout << "BRAKE mode.\n";
      else
	cout << "Failed to enter BRAKE mode.\n";
    }
    
    else if(cmd == "dopen"){
      result = fmod_ipdcmot_wmode(fs, FMOD_IPDCMOT_DRIVEOPEN);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_wmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      if(FMOD_IPDCMOT_DRIVEOPEN == some_uint8)
	cout << "DRIVEOPEN mode.\n";
      else
	cout << "Failed to enter DRIVEOPEN mode.\n";
    }
    
    else if(cmd == "oloop"){
      result = fmod_ipdcmot_wmode(fs, FMOD_IPDCMOT_OPENLOOP);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_wmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      if(FMOD_IPDCMOT_OPENLOOP == some_uint8)
	cout << "OPENLOOP mode.\n";
      else
	cout << "Failed to enter OPENLOOP mode.\n";
    }
    
    else if(cmd == "wait"){
      result = fmod_ipdcmot_wmode(fs, FMOD_IPDCMOT_WAIT);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_wmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      if(FMOD_IPDCMOT_WAIT == some_uint8)
	cout << "WAIT mode.\n";
      else
	cout << "Failed to enter WAIT mode.\n";
    }
    
    else if(cmd == "speed"){
      result = fmod_ipdcmot_wmode(fs, FMOD_IPDCMOT_SPEEDCTRL);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_wmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      if(FMOD_IPDCMOT_SPEEDCTRL == some_uint8)
	cout << "SPEEDCTRL mode.\n";
      else
	cout << "Failed to enter SPEEDCTRL mode.\n";
    }
    
    else if(cmd == "pos"){
      result = fmod_ipdcmot_wmode(fs, FMOD_IPDCMOT_POSCTRL);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_wmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      result = fmod_ipdcmot_rmode(fs, & some_uint8);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rmode(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      if(FMOD_IPDCMOT_POSCTRL == some_uint8)
	cout << "POSCTRL mode.\n";
      else
	cout << "Failed to enter POSCTRL mode.\n";
    }
    
    else if(cmd == "win"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_win(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_win(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rin(fs, & some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rin(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	printf("INPUT -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "rin"){
      result = fmod_ipdcmot_rin(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rin(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      printf("in: 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rinmin"){
      result = fmod_ipdcmot_rinmin(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rinmin(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("inmin: 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rinmax"){
      result = fmod_ipdcmot_rinmax(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rinmax(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("inmax: 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rpos"){
      result = fmod_ipdcmot_rpos(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rpos(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      printf("position: 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rspeed"){
      result = fmod_ipdcmot_rspeed(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rspeed(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("speed: 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rimax"){
      result = fmod_ipdcmot_rimax(fs, & some_double);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rimax(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("imax: %f\n", some_double);
    }
    
    else if(cmd == "winoff"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_winoff(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_winoff(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("input OFFSET -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "winmin"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_winmin(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_winmin(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("MINIMUM input -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "winmax"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_winmax(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_winmax(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("MAXIMUM input -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "wkp"){
      if( ! (cin >> some_double))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wkp(fs, some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wkp(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rkp(fs, & some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rkp(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	printf("PROPORTIONAL constant -> %f\n", some_double);
      }
    }
    
    else if(cmd == "wki"){
      if( ! (cin >> some_double))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wki(fs, some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wki(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rki(fs, & some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rki(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	printf("INTEGRATION constant -> %f\n", some_double);
      }
    }
    
    else if(cmd == "wkd"){
      if( ! (cin >> some_double))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wkd(fs, some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wkd(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rkd(fs, & some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rkd(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	printf("DERIVATION constant -> %f\n", some_double);
      }
    }
    
    else if(cmd == "wimax"){
      if( ! (cin >> some_double))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wimax(fs, some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wimax(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rimax(fs, & some_double);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rimax(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("maximum CURRENT -> %f\n", some_double);
      }
    }
    
    else if(cmd == "wacc"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wacc(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wacc(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_racc(fs, & some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_racc(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("ACCELERATION -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "wdec"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wdec(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wdec(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rdec(fs, & some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rdec(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("DECELERATION -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "wtspeed"){
      if( ! (cin >> some_int32))
	cout << "ERROR reading parameter\n";
      else{
	result = fmod_ipdcmot_wtspeed(fs, some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_wtspeed(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	result = fmod_ipdcmot_rtspeed(fs, & some_int32);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ipdcmot_rtspeed(): " << fmod_errstr(result)
	       << "\n";
	  return 1;
	}
	printf("TOP SPEED -> 0x%08X = %d\n", some_int32, some_int32);
      }
    }
    
    else if(cmd == "rkp"){
      result = fmod_ipdcmot_rkp(fs, & some_double);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rkp(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      printf("kp = %f\n", some_double);
    }
    
    else if(cmd == "rkd"){
      result = fmod_ipdcmot_rkd(fs, & some_double);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rkd(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      printf("kd = %f\n", some_double);
    }
    
    else if(cmd == "rki"){
      result = fmod_ipdcmot_rki(fs, & some_double);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rki(): " << fmod_errstr(result) << "\n";
	return 1;
      }
      printf("ki = %f\n", some_double);
    }

    else if(cmd == "racc"){
      result = fmod_ipdcmot_racc(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_racc(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("acceleration = 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rdec"){
      result = fmod_ipdcmot_rdec(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rdec(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("deceleration = 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else if(cmd == "rtspeed"){
      result = fmod_ipdcmot_rtspeed(fs, & some_int32);
      if(result != FMOD_OK){
	cerr << "ERROR: fmod_ipdcmot_rtspeed(): " << fmod_errstr(result)
	     << "\n";
	return 1;
      }
      printf("top speed = 0x%08X = %d\n", some_int32, some_int32);
    }
    
    else
      cout << "ERROR: unknown command \"" << cmd << "\"\n";
    
    cout << argv[0] << ":" << chname << " > ";
    if( ! (cin >> cmd)){
      cerr << "ERROR reading command\n";
      return 1;
    }
  }
  
  return 0;
}


void cleanup()
{
  if(0 >= fd){
    cerr << "nothing to clean up.\n";
    return;
  }
  
  if((0 < fd)
     && (0 != tcp_close(fd)))
    cerr << "warning: tcp_close() failed.\n";
  
  if(0 != ka){
    keepalive_stop(ka);
    keepalive_delete(ka);
  }
  
  if(0 != fs)
    fmod_delete(fs);
}


struct keepalive_s * keepalive_new(struct fmod_s * fs,
				   unsigned int seconds,
				   FILE * out)
{
  struct keepalive_s * ka = new struct keepalive_s();
  ka->fs = fs;
  ka->seconds = seconds;
  ka->out = out;
  ka->stop = 0;
  ka->thread = 0;
  return ka;
}


void keepalive_delete(struct keepalive_s * ka)
{
  delete ka->thread;
  delete ka;
}


void * keepalive_run(struct keepalive_s * ka)
{
  while(0 == ka->stop){
    int32_t pos;
    int result(fmod_ipdcmot_rpos(ka->fs, & pos));
    if(FMOD_OK != result)
      fprintf(ka->out, "keepalive(): fmod_ipdcmot_rpos() returned %d.\n",
	      result);
    //     else
    //       fprintf(ka->out, "keepalive(): pos = %d\n", pos);
    sleep(ka->seconds);
  }
  ka->stop = 0;
  return ka;
}


void keepalive_start(struct keepalive_s * ka)
{
  if(0 != ka->thread)
    keepalive_stop(ka);
  
  ka->stop = 0;
  ka->thread = new pthread_t();
  if(0 != pthread_create(ka->thread, 0, (void*(*)(void*)) keepalive_run, ka)){
    delete ka->thread;
    ka->thread = 0;
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }
}


void keepalive_stop(struct keepalive_s * ka)
{
  if(0 == ka->thread)
    return;
  
  ka->stop = 1;
  usleep(100000);
  while(0 != ka->stop){
    fprintf(ka->out, ".");
    usleep(100000);
  }
  
  if(0 != pthread_join( * ka->thread, 0))
    fprintf(ka->out, "pthread_join() failed: %s\n", strerror(errno));
  delete ka->thread;
  ka->thread = 0;
}
