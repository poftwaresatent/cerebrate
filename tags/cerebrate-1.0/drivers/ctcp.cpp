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
#include "fmod_tcp.h"
#include <iostream>
#include <sstream>
#include <string>


using namespace std;


void cleanup();


static int fd(0);
static struct fmod_s * fs(0);


int main(int argc,
	 char ** argv)
{
  FILE * dbg = 0;
  
  set_cleanup(cleanup);
  
  string chname;
  if(argc > 1)
    chname = argv[1];
  else
    chname = "iocactus";
  
  fd = tcp_open(8010, chname.c_str());
  if(fd < 0)
    return 1;
  fs = fmod_new(fd);
  
  string cmd("help");
  while(true){
    int result, num, val;
    uint16_t some_uint16;
    uint8_t some_uint8;
    
    if(cmd == "quit")
      break;
    
    else if(cmd == "help")
      cout << "Available commands:\n"
	   << "  quit               exit this program\n"
	   << "  help               write this message\n"
	   << "  riodir <num>       read IO direction\n"
	   << "  rio    <num>       read IO\n"
	   << "  rad    <num>       read AD\n"
	   << "  wiodir <num> <hex> write IO direction\n"
	   << "  wio    <num> <hex> write IO\n";
    
    else if(cmd == "riodir"){
      if( ! (cin >> num))
	cout << "ERROR reading parameter\n";
      result = fmod_tcp_riodir(fs, num, & some_uint8, dbg);
      if(result != FMOD_OK)
	cout << "ERROR: fmod_tcp_riodir(): " << fmod_errstr(result) << "\n";
      else{
	printf("IODIR%d: 0x%02X\n", num, some_uint8);
	for(int i(1); i < 0x100; i <<= 1)
	  if(some_uint8 & i)
	    cout << "  in\n";
	  else
	    cout << "  out\n";
      }
    }
    
    else if(cmd == "rio"){
      if( ! (cin >> num))
	cout << "ERROR reading parameter\n";
      result = fmod_tcp_rio(fs, num, & some_uint8, dbg);
      if(result != FMOD_OK)
	cout << "ERROR: fmod_tcp_rio(): " << fmod_errstr(result) << "\n";
      else{
	printf("IO%d: 0x%02X\n", num, some_uint8);
	uint8_t dir;
	result = fmod_tcp_riodir(fs, num, & dir, dbg);
	if(result != FMOD_OK)
	  cout << "ERROR: fmod_tcp_riodir(): " << fmod_errstr(result) << "\n";
	else
	  for(int i(1); i < 0x100; i <<= 1)
	    if(dir & i){
	      if(some_uint8 & i)
		cout << "  1\n";
	      else
		cout << "  0\n";
	    }
	    else
	      cout << "  x\n";
      }
    }
    
    else if(cmd == "rad"){
      if( ! (cin >> num))
	cout << "ERROR reading parameter\n";
      result = fmod_tcp_rad(fs, num, & some_uint16, dbg);
      if(result != FMOD_OK)
	cout << "ERROR: fmod_tcp_rad(): " << fmod_errstr(result) << "\n";
      else
	printf("AD%d: 0x%04X = %d\n", num, some_uint16, some_uint16);
    }
    
    else if(cmd == "wiodir"){
      if( ! (cin >> num))
	cout << "ERROR reading first parameter\n";
      if(fscanf(stdin, "%i", & val) != 1)
	cout << "ERROR reading second parameter\n";
      result = fmod_tcp_wiodir(fs, num, val & 0xFF, dbg);
      if(result != FMOD_OK)
	cout << "ERROR: fmod_tcp_wiodir(): " << fmod_errstr(result) << "\n";
      else{
	result = fmod_tcp_riodir(fs, num, & some_uint8, dbg);
	if(result != FMOD_OK)
	  cout << "ERROR: fmod_tcp_riodir(): " << fmod_errstr(result) << "\n";
	else{
	  printf("IODIR%d: 0x%02X\n", num, some_uint8);
	  for(int i(1); i < 0x100; i <<= 1)
	    if(some_uint8 & i)
	      cout << "  in\n";
	    else
	      cout << "  out\n";
	}
      }
    }
    
    else if(cmd == "wio"){
      if( ! (cin >> num))
	cout << "ERROR reading first parameter\n";
      if(fscanf(stdin, "%i", & val) != 1)
	cout << "ERROR reading second parameter\n";
      result = fmod_tcp_wio(fs, num, val & 0xFF, dbg);
      if(result != FMOD_OK)
	cout << "ERROR: fmod_tcp_wio(): " << fmod_errstr(result) << "\n";
      else{
	result = fmod_tcp_rio(fs, num, & some_uint8, dbg);
	if(result != FMOD_OK)
	  cout << "ERROR: fmod_tcp_rio(): " << fmod_errstr(result) << "\n";
	else{
	  printf("IO%d: 0x%02X\n", num, some_uint8);
	uint8_t dir;
	result = fmod_tcp_riodir(fs, num, & dir, dbg);
	if(result != FMOD_OK)
	  cout << "ERROR: fmod_tcp_riodir(): " << fmod_errstr(result) << "\n";
	else
	  for(int i(1); i < 0x100; i <<= 1)
	    if(dir & i){
	      if(some_uint8 & i)
		cout << "  1\n";
	      else
		cout << "  0\n";
	    }
	    else
	      cout << "  x\n";
	}
      }
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
  
  if(tcp_close(fd) != 0)
    cerr << "warning: tcp_close() failed.\n";
  
  if(0 == fs)
    return;
  fmod_delete(fs);
}
