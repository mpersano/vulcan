/* yyerror.c -- part of vulcan
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "panic.h"
#include "yyerror.h"

int lineno;

void
yyerror(const char *str)
{
	panic("error on line %d: %s", lineno, str);
}
