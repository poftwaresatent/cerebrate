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


using namespace std;


void cleanup();
int testrreg(struct fmod_s * fs);
int testwreg(struct fmod_s * fs);
int testrreg32(struct fmod_s * fs);


static int fd(0);
static struct fmod_s * fs(0);


int main(int argc, char ** argv)
{
  set_cleanup(cleanup);
  
  string chname("169.254.5.5");
  if(argc > 1)
    chname = argv[1];
  
  fd = tcp_open(8010, chname.c_str());
  if(fd < 0){
    cerr << "tcp_open(8010, " << chname << ") returned " << fd << ".\n";
    return 1;
  }
  fs = fmod_new(fd);
  
  string cmd("help");
  while(true){
    if((cmd == "quit") || (cmd == "q"))
      break;
    
    else if((cmd == "help") || (cmd == "h"))
      cout << "Available commands:\n"
	   << "  quit               exit this program\n"
	   << "  help               write this message\n"
	   << "  wreg hexadr hexval write val to register\n"
	   << "  rreg hexadr        read register\n"
	   << "  wip uint8[4]       write IP address\n"
	   << "  str                store user parameters\n";
    
    else if(cmd == "wreg"){
      string hexadr, hexval;
      if( ! (cin >> hexadr >> hexval))
	cerr << "error reading hexadr and hexval\n";
      else{
	unsigned int adr, val;
	if(1 != sscanf(hexadr.c_str(), "%x", &adr))
	  cerr << "error parsing hexadr\n";
	else{
	  if(1 != sscanf(hexval.c_str(), "%x", &val))
	    cerr << "error parsing hexval\n";
	  else{
	    printf("writing 0x%02x to register 0x%02x\n", val, adr);
	    uint8_t adr8(adr & 0xff);
	    uint8_t val8(val & 0xff);
	    const int status(fmod_wreg(fs, 1234, adr8, &val8, 1, 0));
	    if(FMOD_OK != status)
	      cerr << "fmod_wreg(): " << fmod_errstr(status) << "\n";
	    else
	      cout << "OK\n";
	  }
	}
      }
    }
    
    else if(cmd == "rreg"){
      string hexadr;
      if( ! (cin >> hexadr))
	cerr << "error reading hexadr\n";
      else{
	unsigned int adr;
	if(1 != sscanf(hexadr.c_str(), "%x", &adr))
	  cerr << "error parsing hexadr\n";
	else{
	  uint8_t adr8(adr & 0xff);
	  uint8_t val8;
	  const int status(fmod_rreg(fs, 1234, adr8, &val8, 1, 0));
	  if(FMOD_OK != status)
	    cerr << "fmod_rreg(): " << fmod_errstr(status) << "\n";
	  else
	    printf("register 0x%02x has value 0x%02x\n", adr8, val8);
	}
      }
    }
    
    else if(cmd == "wip"){
      int ipaddr[4];
      if(4 != fscanf(stdin, "%d.%d.%d.%d",
		     ipaddr, ipaddr + 1, ipaddr + 2, ipaddr + 3))
	cerr << "error reading IP address, format: abc.def.geh.ijk\n";
      else{
	uint8_t u8[4];
	for(int i(0); i < 4; ++i){
	  u8[i] = ipaddr[i] & 0xFF;
	  cout << "  u8[" << i << "] = " << (int) u8[i] << "\n";
	}

	uint8_t oldip[4];
	int result = fmod_ripaddr(fs, oldip, stderr);
	if(result != FMOD_OK){
	  cerr << "ERROR: fmod_ripaddr(): " << fmod_errstr(result) << "\n";
	  return 1;
	}
	cout << "old IP: " << (int) oldip[0] << "." << (int) oldip[1]
	     << "." << (int) oldip[2] << "." << (int) oldip[3] << "\n";
	
	result = fmod_wipaddr(fs, u8, stderr);
	if(result != FMOD_OK)
	  cerr << "ERROR: fmod_wipaddr(): " << fmod_errstr(result) << "\n";
	else{
	  static const bool undo(false);
	  if(undo){
	    result = fmod_wipaddr(fs, oldip, stderr);
	    if(result != FMOD_OK){
	      cerr << "ERROR: fmod_wipaddr(): " << fmod_errstr(result) << "\n"
		   << "  Dang, and this while restablishing the old IP!\n";
	      return 1;
	    }
	  }
	  else{
	    fmod_delete(fs);
	    if(0 != tcp_close(fd))
	      cerr << "warning: tcp_close() failed.\n";
	    
	    {
	      ostringstream chn;
	      chn << (int) u8[0] << "." << (int) u8[1] << "."
		  << (int) u8[2] << "." << (int) u8[3];
	      chname = chn.str();
	    }
	    
	    fd = tcp_open(8010, chname.c_str());
	    if(fd < 0){
	      cerr << "tcp_open(8010, " << chname << ") returned "
		   << fd << ".\n";
	      return 1;
	    }
	    fs = fmod_new(fd);
	  }
	}
      }
    }
    
    else
      cout << "ERROR: unknown command \"" << cmd << "\"\n";
    
    cout << "[" << chname << "]> ";
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
  
  if(0 != fs)
    fmod_delete(fs);
}


int testrreg(struct fmod_s * fs)
{
  int result;
  uint8_t value[6];
  
  result = fmod_rreg(fs, 0x1234, 0x11, value, 6, stdout);
  if(result != FMOD_OK){
    cerr << "fmod_rreg(): " << fmod_errstr(result) << ".\n";
    return 1;
  }
  printf("register 0x11 (MAC) = %02X:%02X:%02X:%02X:%02X:%02X\n",
	 value[0], value[1], value[2], value[3], value[4], value[5]);
  
  return 0;
}


int testwreg(struct fmod_s * fs)
{
//    static const uint8_t oname[] = {
//      0x49, 0x50, 0x44, 0x43,
//      0x4D, 0x4F, 0x54, 0x20,
//      0x34, 0x38, 0x2F, 0x31,
//      0x2E, 0x35, 0x20, 0x20
//    };
//    int result;
//    result = fmod_wreg(fs, 0x1234, 0x15, oname, 16, 0);
//    if(result != FMOD_OK){
//      cerr << "fixing old name: " << fmod_errstr(result) << ".\n";
//      return 1;
//    }

  uint8_t oldname[17];  
  int result;
  result = fmod_rreg(fs, 0x1234, 0x15, oldname, 16, 0);
  if(result != FMOD_OK){
    cerr << "reading old name: " << fmod_errstr(result) << ".\n";
    return 1;
  }
  oldname [16] = 0;
  cout << "old name: " << reinterpret_cast<char *>(oldname) << "\n";
  
  uint8_t newname[16];
  for(uint8_t i(0); i < 16; ++i)
    newname[i] = i;
  result = fmod_wreg(fs, 0x1234, 0x15, newname, 16, 0);
  int retval(0);
  if(result != FMOD_OK){
    cerr << "writing new name: " << fmod_errstr(result) << ".\n";
    retval = 1;
  }
  else{
    uint8_t check[16];
    result = fmod_rreg(fs, 0x1234, 0x15, check, 16, 0);
    if(result != FMOD_OK){
      cerr << "reading new name: " << fmod_errstr(result) << ".\n";
      retval = 1;
    }
    else{
      for(int i(0); i < 16; ++i)
	if(check[i] != newname[i]){
	  cerr << "check failed for byte #" << i << ":\n"
	       << "  want: " << newname[i] << "\n"
	       << "  have: " << check[i] << "\n";
	  retval = 1;
	  break;
	}
      result = fmod_wreg(fs, 0x1234, 0x15, oldname, 16, 0);
      if(result != FMOD_OK){
	cerr << "writing old name: " << fmod_errstr(result) << ".\n";
	retval = 1;
      }
    }
  }
  
  return retval;
}


int testrreg32(struct fmod_s * fs)
{
  int result;
  int32_t value;
  
  result = fmod_rreg32(fs, 0x1234, 0x00, & value, 0);
  if(result != FMOD_OK){
    cerr << "fmod_rreg32(): " << fmod_errstr(result) << ".\n";
    return 1;
  }
  printf("register 0x00 (TYPE) = 0x%08X\n", value);
  
  return 0;
}
