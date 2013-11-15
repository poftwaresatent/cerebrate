%{
/* not sure how strict we should be here */
#ifndef OSX
# define YYSTYPE char *
#endif

#include "voxel_parse.tab.h"
extern char * vvlval;
%}

%%
empty       return EMPTY;
color       return COLOR;
layer       return LAYER;
line        return LINE;
\'.\'       vvlval=vvtext; return CHAR;
\"[^\"]*\"  vvlval=vvtext; return STRING;
[0-9][0-9]* vvlval=vvtext; return NUMBER;
[ \t\n]+    /* ignore whitespace */
%%
