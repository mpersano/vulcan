/* modeldef.h -- part of vulcan
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

#ifndef MODELDEF_H_
#define MODELDEF_H_

struct dict;

void
modeldef_parse_file(struct dict *modeldef_dict, const char *filename);

#endif /* MODELDEF_H_ */
