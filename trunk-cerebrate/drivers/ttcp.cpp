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
  
  for(int i(0); i < 3; ++i){
    int result(fmod_tcp_wiodir(fs, i, 0, dbg));
    if(result != FMOD_OK){
      cout << "ERROR: fmod_tcp_wiodir(): " << fmod_errstr(result) << "\n";
      return 1;
    }
    uint8_t iodir;
    result = fmod_tcp_riodir(fs, i, & iodir, dbg);
    if(result != FMOD_OK){
      cout << "ERROR: fmod_tcp_riodir(): " << fmod_errstr(result) << "\n";
      return 1;
    }
    if(0 != iodir){
      printf("IODIR%d should be 0x00, but is 0x%02X\n", i, iodir);
      for(int i(1); i < 0x100; i <<= 1)
	if(iodir & i)
	  cout << "  in\n";
	else
	  cout << "  out\n";
      return 1;
    }
  }
  
  for(int bit(0); true; bit = ++bit % 8){
    cout << chname << ": bit = " << bit << " => " << (1 << bit) << "\n";
    for(int i(0); i < 3; ++i){
      int result(fmod_tcp_wio(fs, i, (1 << bit), dbg));
      if(result != FMOD_OK)
	cout << "ERROR: fmod_tcp_wio(" << i << "," << (1 << bit) << "): "
	     << fmod_errstr(result) << "\n";
      else{
	uint8_t io;
	result = fmod_tcp_rio(fs, i, & io, dbg);
	if(result != FMOD_OK)
	  cout << "ERROR: fmod_tcp_rio(" << i << "," << (1 << bit) << "): "
	       << fmod_errstr(result) << "\n";
      }
    }
    usleep(500000);
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
