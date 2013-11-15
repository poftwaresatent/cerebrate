%{
#include <stdio.h>
#include "voxel_parse.tab.h"
extern char * yylval;
%}

%%
empty       return EMPTY;
color       return COLOR;
layer       return LAYER;
line        return LINE;
\'.\'       yylval=yytext; return CHAR;
\"[^\"]*\"  yylval=yytext; return STRING;
[0-9][0-9]* yylval=yytext; return NUMBER;
[ \t\n]+    /* ignore whitespace */
%%
