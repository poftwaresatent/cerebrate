%{
#include <stdlib.h>
#include <stdio.h>
#include "effect_parse.tab.h"
extern double eelval;
%}

%%
balloon              return TBALLOON;
warp                 return TWARP;
spike                return TSPIKE;
bump                 return TBUMP;
distance             return TDISTANCE;
direction            return TDIRECTION;
point                return TPOINT;
line                 return TLINE;
plane                return TPLANE;

[+-]?[0-9]+\.?[0-9]* {
  char const * check = NULL;
  eelval = strtod(eetext, check);
  /* this does not work as expected though */
  if (check == eetext) {
    fprintf(stderr, "ERROR converting (start of) '%s' to double\n", eetext);
    return REALERROR;
  }
  return REAL;
}

[ \t\n]+             /* ignore whitespace */
%%
