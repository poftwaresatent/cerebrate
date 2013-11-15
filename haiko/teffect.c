#include "effect.h"
#include <stdio.h>


int main(int argc, char ** argv)
{
  FILE * configfile;
  effect_t balloon;
  effect_t warp;
  
  if (argc > 1)
    configfile = fopen(argv[1], "r");
  else
    configfile = stdin;
  
  if (0 != effect_parse_file(configfile, &balloon, &warp, 1))
    fprintf(stderr, "ooopsie, effect_parse_file() failed\n");
  else {
#warning "do something here"
  }
  
  return 0;
}
