/* image.h -- part of vulcan
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

#ifndef IMAGE_H_
#define IMAGE_H_

struct image {
	int width;
	int height;
	unsigned *bits;
};

struct image *
image_make_from_png(const char *png_filename);

void
image_free(struct image *img);

#endif /* IMAGE_H_ */
