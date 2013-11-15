%{
#include "effect.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define YYSTYPE double

extern FILE * eein;

extern int eeparse();
extern int eelex();

typedef struct tab_s {
  int error;
  int verbose;
  effect_distref_t tmpdistref;
  effect_t tmpwibble;
  effect_t balloon;
  effect_t warp;
} tab_t;

static tab_t tab;


int effect_parse_file(FILE * configfile,
		      effect_t * balloon,
		      effect_t * warp,
		      int verbose)
{
  int result;
  tab.error = 0;
  tab.verbose = verbose;
  tab.tmpwibble.amplitude = NULL;
  eein = configfile;

  result = eeparse();
  if (0 != result)
    return result;
  if (0 != tab.error)
    return 42;

  if (NULL != balloon) {
    if (NULL != tab.balloon.amplitude)
      memcpy(balloon, &tab.balloon, sizeof(*balloon));
    else
      bzero(balloon, sizeof(*balloon));
    effect_configure_balloon(balloon);
  }
  
  if (NULL != warp) {
    if (NULL != tab.warp.amplitude)
      memcpy(warp, &tab.warp, sizeof(*warp));
    else
      bzero(warp, sizeof(*warp));
    effect_configure_warp(warp);
  }
  
  return 0;
}


void eeerror(const char * str)
{
  fprintf(stderr, "eeerror: %s\n", str);
}


int eewrap()
{
  return 1;
}


%}

%token TBALLOON
%token TWARP
%token TSPIKE
%token TBUMP
%token TDISTANCE
%token TDIRECTION
%token TPOINT
%token TLINE
%token TPLANE
%token REAL
%token REALERROR

%%

commands: /* empty */
        | commands command
        ;

command:
        balloon
        |
        warp
        |
        realerror
        ;

realerror:
	REALERROR
        {
	  tab.error = 1;
        }
        ;

balloon:
	TBALLOON spike distance
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> spike balloon\n");
	  memcpy(&tab.balloon, &tab.tmpwibble, sizeof(tab.balloon));
	  tab.tmpwibble.amplitude = NULL;
        }
        |
	TBALLOON bump distance
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> bump balloon\n");
	  memcpy(&tab.balloon, &tab.tmpwibble, sizeof(tab.balloon));
	  tab.tmpwibble.amplitude = NULL;
        }
	;

warp:
	TWARP spike distance direction
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> spike warp\n");
	  memcpy(&tab.warp, &tab.tmpwibble, sizeof(tab.warp));
	  tab.tmpwibble.amplitude = NULL;
        }
	|
	TWARP bump distance direction
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> bump warp\n");
	  memcpy(&tab.warp, &tab.tmpwibble, sizeof(tab.warp));
	  tab.tmpwibble.amplitude = NULL;
        }
	;

spike:
	TSPIKE REAL REAL REAL REAL REAL REAL
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> spike %f %f %f %f %f %f\n", $2, $3, $4, $5, $6, $7);
	  tab.tmpwibble.duration    = $2;
	  tab.tmpwibble.period      = $3;
	  tab.tmpwibble.speed       = $4;
	  tab.tmpwibble.minval      = $5;
	  tab.tmpwibble.maxgrow     = $6;
	  tab.tmpwibble.power       = $7;
	  tab.tmpwibble.amplitude   = effect_amplitude_spike;
        }

bump:
	TBUMP REAL REAL REAL REAL REAL REAL
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> bump %f %f %f %f %f %f\n", $2, $3, $4, $5, $6, $7);
	  tab.tmpwibble.duration    = $2;
	  tab.tmpwibble.period      = $3;
	  tab.tmpwibble.speed       = $4;
	  tab.tmpwibble.minval      = $5;
	  tab.tmpwibble.maxgrow     = $6;
	  tab.tmpwibble.power       = $7;
	  tab.tmpwibble.amplitude   = effect_amplitude_bump;
        }
        ;

distance:
	TDISTANCE point
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> distance point\n");
	  tab.tmpdistref.type = EFFECT_DISTREF_POINT;
	  memcpy(&tab.tmpwibble.distance, &tab.tmpdistref,
		 sizeof(tab.tmpwibble.distance));
        }
	|
	TDISTANCE line
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> distance line\n");
	  tab.tmpdistref.type = EFFECT_DISTREF_LINE;
	  memcpy(&tab.tmpwibble.distance, &tab.tmpdistref,
		 sizeof(tab.tmpwibble.distance));
        }
	|
	TDISTANCE plane
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> distance plane\n");
	  tab.tmpdistref.type = EFFECT_DISTREF_PLANE;
	  memcpy(&tab.tmpwibble.distance, &tab.tmpdistref,
		 sizeof(tab.tmpwibble.distance));
        }
	;

direction:
	TDIRECTION point
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> direction point\n");
	  tab.tmpdistref.type = EFFECT_DISTREF_POINT;
	  memcpy(&tab.tmpwibble.direction, &tab.tmpdistref,
		 sizeof(tab.tmpwibble.distance));
        }
	|
	TDIRECTION line
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> direction line\n");
	  tab.tmpdistref.type = EFFECT_DISTREF_LINE;
	  memcpy(&tab.tmpwibble.direction, &tab.tmpdistref,
		 sizeof(tab.tmpwibble.distance));
        }
	|
	TDIRECTION plane
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> direction plane\n");
	  tab.tmpdistref.type = EFFECT_DISTREF_PLANE;
	  memcpy(&tab.tmpwibble.direction, &tab.tmpdistref,
		 sizeof(tab.tmpwibble.distance));
        }
	;

point:
	TPOINT REAL REAL REAL
        {
	  if (tab.error)
	    return 1;
	  if (tab.verbose)
	    printf(">>> point %f %f %f\n", $2, $3, $4);
	  tab.tmpdistref.point[0] = $2;
	  tab.tmpdistref.point[1] = $3;
	  tab.tmpdistref.point[2] = $4;
        }
        ;

line:
	TLINE REAL REAL REAL REAL REAL REAL
        {
	  double len;
	  if (tab.error)
	    return 1;
	  len = sqrt(pow($5, 2) + pow($6, 2) + pow($7, 2));
	  if (tab.verbose)
	    printf(">>> line %f %f %f %f %f %f\n"
		   "    length %f\n", $2, $3, $4, $5, $6, $7, len);
	  if (len < 1e-6) {	/* hm, hardcoded threshold */
	    if (tab.verbose)
	      printf("    ERROR: length too small\n");
	    tab.error = 1;
	    return 1;
	  }
	  tab.tmpdistref.point[0] = $2;
	  tab.tmpdistref.point[1] = $3;
	  tab.tmpdistref.point[2] = $4;
	  tab.tmpdistref.unit[0]  = $5 / len;
	  tab.tmpdistref.unit[1]  = $6 / len;
	  tab.tmpdistref.unit[2]  = $7 / len;
        }
        ;

plane:
	TPLANE REAL REAL REAL REAL REAL REAL
        {
	  double len;
	  if (tab.error)
	    return 1;
	  len = sqrt(pow($5, 2) + pow($6, 2) + pow($7, 2));
	  if (tab.verbose)
	    printf(">>> plane %f %f %f %f %f %f\n"
		   "    length %f\n", $2, $3, $4, $5, $6, $7, len);
	  if (len < 1e-6) {	/* hm, hardcoded threshold */
	    if (tab.verbose)
	      printf("    ERROR: length too small\n");
	    tab.error = 1;
	    return 1;
	  }
	  tab.tmpdistref.point[0] = $2;
	  tab.tmpdistref.point[1] = $3;
	  tab.tmpdistref.point[2] = $4;
	  tab.tmpdistref.unit[0]  = $5 / len;
	  tab.tmpdistref.unit[1]  = $6 / len;
	  tab.tmpdistref.unit[2]  = $7 / len;
        }
        ;
