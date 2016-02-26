%{
/* vrml_gram.y -- part of vulcan
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

#include "yy_to_vrml.h"

#include <stdlib.h>
#include <stdio.h>

#include "yyerror.h"
#include "panic.h"
#include "vrml_tree.h"
#include "list.h"

int yylex(void);

static struct list *vrml_node_list;

%}

%union {
	long longval;
	float floatval;
	char *strval;
	struct vrml_vector *vecval;
	struct list *listval;
	union vrml_node *nodeval;
};

%token DEF INFO STRING COORDINATE3 POINT INDEXEDFACESET COORDINDEX SEPARATOR
%token <longval> INTEGER
%token <floatval> FLOAT
%token <strval> IDENTIFIER STRINGLIT

%type <floatval> SFFloat
%type <longval> SFLong
%type <vecval> SFVec3f
%type <strval> node_spec info_field SFString
%type <listval> coord3_field vec3f_list MFVec3f
%type <listval> ifaceset_field sflong_list MFLong
%type <listval> nodes
%type <nodeval> Coordinate3 IndexedFaceSet Separator Info node_body node

%%

input
: nodes					{ vrml_node_list = $1; }	

/*
{
			struct list_node *ln;

			for (ln = $1->first; ln; ln = ln->next) {
				union vrml_node *vn =
				  (union vrml_node *)ln->data;
				vn->common.print_fn(vn, 0);
			}
		}
*/
| /* nothing */
;

nodes
: nodes node				{ $$ = list_append($1, $2); }
| node					{ $$ = list_append(list_make(), $1); }
;

node
: node_spec node_body			{ $$ = $2; $$->common.name = $1; }
;

node_spec
: DEF IDENTIFIER			{ $$ = $2; }
| /* nothing */				{ $$ = NULL; }
;

node_body
: Separator
| Info
| Coordinate3
| IndexedFaceSet
;

Info
: INFO '{' info_field '}'		{ $$ = vrml_node_info_make($3); }
;

info_field
: STRING SFString			{ $$ = $2; }
;

Coordinate3
: COORDINATE3 '{' coord3_field '}'	{ $$ = vrml_node_coordinate3_make($3); }
;

coord3_field
: POINT MFVec3f				{ $$ = $2; }
;

MFVec3f
: '[' vec3f_list ']'			{ $$ = $2; }
| '[' vec3f_list ',' ']'		{ $$ = $2; }
;

vec3f_list
: vec3f_list ',' SFVec3f		{ $$ = list_append($1, $3); }
| SFVec3f				{ $$ = list_append(list_make(), $1); }
;

SFVec3f
: SFFloat SFFloat SFFloat		{ $$ = vrml_vector_make($1, $2, $3); }
;

IndexedFaceSet
: INDEXEDFACESET '{' ifaceset_field '}'	{ $$ = vrml_node_indexed_face_set_make($3); }
;

ifaceset_field
: COORDINDEX MFLong			{ $$ = $2; }
;

Separator
: SEPARATOR '{' nodes '}'		{ $$ = vrml_node_separator_make($3); }
;

MFLong
: '[' sflong_list ']'			{ $$ = $2; }
| '[' sflong_list ',' ']'		{ $$ = $2; }
;

sflong_list
: sflong_list ',' SFLong		{ $$ = list_append($1, (void *)$3); }
| SFLong				{ $$ = list_append(list_make(), (void *)$1); }
;

SFLong
: INTEGER
;

SFFloat
: FLOAT
| INTEGER				{ $$ = $1; }
;

SFString
: STRINGLIT				{ $$ = $1; }
;

%%

#include <stdio.h>

extern FILE *yyin;
extern int lineno;

struct list *
vrml_parse_file(const char *file_name)
{
	FILE *in;

	vrml_node_list = NULL;

	lineno = 1;

	if ((in = fopen(file_name, "r")) == NULL)
		return NULL;

	yyin = in;

	if (yyparse()) {
		if (vrml_node_list != NULL) {
			list_free(vrml_node_list,
			  (void(*)(void *))vrml_node_free);
			vrml_node_list = NULL;
		}
	}

	fclose(in);

	return vrml_node_list;
}
