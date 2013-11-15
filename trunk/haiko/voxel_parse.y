%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "voxel.h"

#define YYSTYPE char *

extern FILE * vvin;

extern int vvparse();
extern int vvlex();

/* would like to use re-entrant parse-param, but that seem "experimental" */
static voxel_parse_tab_t * voxel_parse_tab;


int voxel_parse_file(FILE * configfile, voxel_parse_tab_t * parse_tab)
{
  voxel_parse_init(parse_tab);	/* blindly kills any previous contents... */
  vvin = configfile;
  voxel_parse_tab = parse_tab;
  return vvparse();
}


void vverror(const char * str)
{
  fprintf(stderr, "error: %s\n", str);
}


int vvwrap()
{
  return 1;
}


static void extract_empty(const char * emptystr)
{
  if (strlen(emptystr) < 3)
    fprintf(stderr, "extract_empty(): emptystr too short\n");
  else
    voxel_parse_empty(voxel_parse_tab, emptystr[1]);
}


static void extract_color(const char * charstr,
			  const char * redstr,
			  const char * greenstr,
			  const char * bluestr)
{
  char colchar = 'X';
  int red = 0;
  int green = 0;
  int blue = 0;
  
  if (strlen(charstr) < 3)
    fprintf(stderr, "extract_color(): charstr too short\n");
  else
    colchar = charstr[1];

  red = (int) strtol(redstr, (char **) NULL, 0);
  if ((red < 0) || (red > 255)) {
    fprintf(stderr, "extract_color(): 0 <= red <= 255, not %d\n", red);
    red = 0;
  }
  
  green = (int) strtol(greenstr, (char **) NULL, 0);
  if ((green < 0) || (green > 255)) {
    fprintf(stderr, "extract_color(): 0 <= green <= 255, not %d\n", green);
    green = 0;
  }
  
  blue = (int) strtol(bluestr, (char **) NULL, 0);
  if ((blue < 0) || (blue > 255)) {
    fprintf(stderr, "extract_color(): 0 <= blue <= 255, not %d\n", blue);
    blue = 0;
  }
  
  voxel_parse_color(voxel_parse_tab, colchar,
		    red / 255.0, green / 255.0, blue / 255.0);
}


static void extract_line(const char * linestr)
{
  size_t olen = strlen(linestr);
  if (2 >= olen)
    voxel_parse_line(voxel_parse_tab, "");
  else {
    char * buf = calloc(olen - 1, sizeof(*buf)); /* one more for '\0' */
    strncpy(buf, linestr + 1, olen - 2);
    voxel_parse_line(voxel_parse_tab, buf);
    free(buf);
  }
}

%}

%token EMPTY COLOR LAYER LINE CHAR STRING NUMBER

%%

commands: /* empty */
        | commands command
        ;

command:
        empty
        |
        color
        |
        layer
        |
        line
        ;

empty:
        EMPTY CHAR
        {
	  extract_empty($2);
        }
        ;

color:
        COLOR CHAR NUMBER NUMBER NUMBER
        {
	  extract_color($2, $3, $4, $5);
        }
        ;

layer:
	LAYER
	{
	  voxel_parse_layer(voxel_parse_tab);
	}
	;

line:
	LINE STRING
	{
	  extract_line($2);
	}
	;
