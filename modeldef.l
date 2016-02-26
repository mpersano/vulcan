%{
/* modeldef_scan.l -- part of vulcan
 *
 * This program is copyright (C) 2006 Mauro Persano, and is free
 * software which is freely distributable under the terms of the
 * GNU public license, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "modeldef_y_tab.h"
#include "yyerror.h"

%}
%%

"-"?[0-9]+		{
				yylval.longval = atol(yytext);
				return INTEGER;
			}
"-"?[0-9]*"."?[0-9]*([eE][+-]?[0-9]+)?	{
				yylval.floatval = atof(yytext);
				return FLOAT;
			}
model			{ return MODEL; }
mesh			{ return MESH; }
coordinates		{ return COORDINATES; }
triangles		{ return TRIANGLES; }
normals			{ return NORMALS; }
volume			{ return VOLUME; }
bounding-sphere		{ return BOUNDING_SPHERE; }
center			{ return CENTER; }
radius			{ return RADIUS; }
bsp-tree		{ return BSP_TREE; }
planes			{ return PLANES; }
nodes			{ return NODES; }
[a-zA-Z_][a-zA-Z0-9_]*	{ yylval.strval = strdup(yytext); return STRINGLIT; }
"{"|"}"|"["|"]"|","	{ return *yytext; }
"#"[^\n]*
"\""			{
				static char *buf;
				static int buflen = 0;
				int i, c;

				if (buflen == 0)
					buf = malloc(1 + (buflen = 1));

				i = 0;

				while ((c = input()) != '"' && c != EOF) {
					if (c == '\\')
						c = input();

					if (i == buflen)
						buf = realloc(buf,
						  1 + (buflen *= 2));

					buf[i++] = c;
				}

				buf[i] = '\0';

				yylval.strval = strdup(buf);

				return STRINGLIT;
			}
[ \t]+			/* eat up whitespace */
\n			{ ++lineno; }
.			{
				printf("invalid char '%c'\n", *yytext);
				yyerror("invalid character");
			}

%%
int yywrap(void)
{
  return 1;
} 
