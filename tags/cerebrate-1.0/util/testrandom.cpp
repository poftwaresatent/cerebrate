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


#include "Random.hpp"
#include <iostream>


using namespace std;


int main(int argc, char ** argv)
{
  static const int nchances(100000);
  for(double chance(0); chance <= 1.0; chance += 0.1){
    double count(0);
    for(int n(0); n < nchances; ++n)
      if(Random::Uniform(chance))
	count += 1;
    cout << chance << ": " << count / nchances << "\n";
  }
  double sum(0);
  for(int n(0); n < nchances; ++n)
    sum += Random::Unit();
  cout << "overall mean (want 0.5): " << sum / nchances << "\n";
}
