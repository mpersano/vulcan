/* dumpglyphs.c -- part of vulcan
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
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h> /* for basename(3) */
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include "panic.h"

enum {
	GLYPH_BORDER_SIZE = 2
};

struct glyph {
	int code;
	int width;
	int height;
	int left;
	int top;
	int advance_x;
	int advance_y;
	int texture_x;
	int texture_y;
	int transposed;
	unsigned *bitmap;
};

struct node {
	int x, y;
	int width, height;
	int used;
	struct node *left, *right;
};

static char ttf_filename[PATH_MAX + 1];
static char font_basename[NAME_MAX];

static int font_size;

static int start_code;
static int end_code;
static int num_glyphs;

static struct glyph *glyphs;

static int texture_width;
static int texture_height;

static char texture_filename[PATH_MAX + 1];

static unsigned *texture;

static char fontdef_filename[PATH_MAX + 1];

static void
init_glyph(struct glyph *g, int code, FT_Face face)
{
	unsigned *p, *q, *rgba[2];
	const unsigned char *r;
	FT_GlyphSlot slot;
	int n, i, j, src, dest;

	if ((FT_Load_Char(face, code, FT_LOAD_RENDER)) != 0)
		panic("FT_Load_Char");

	slot = face->glyph;

	g->code = code;
	g->width = slot->bitmap.width + 2*GLYPH_BORDER_SIZE;
	g->height = slot->bitmap.rows + 2*GLYPH_BORDER_SIZE;
	g->left = slot->bitmap_left;
	g->top = slot->bitmap_top;
	g->advance_x = slot->advance.x/64;
	g->advance_y = slot->advance.y/64;
	g->texture_x = g->texture_y = -1;
	g->transposed = 0;

	rgba[0] = calloc(sizeof *rgba[0], g->width*g->height);
	rgba[1] = calloc(sizeof *rgba[1], g->width*g->height);

	p = &rgba[0][GLYPH_BORDER_SIZE*g->width + GLYPH_BORDER_SIZE];
	r = slot->bitmap.buffer;

	for (i = 0; i < slot->bitmap.rows; i++) {
		for (j = 0; j < slot->bitmap.width; j++) {
			const unsigned b = r[j];
			p[j] = b | (b << 8) | (b << 16) | (b << 24);
		}

		p += g->width;
		r += slot->bitmap.width;
	}

	/* low pass filter */

	src = 0;
	dest = 1;

	for (n = 0; n < 2; n++) { /* two passes */
		p = rgba[dest];
		q = rgba[src];

		for (i = 0; i < g->height; i++) {
			for (j = 0; j < g->width; j++) {
				unsigned v = 0;
				unsigned *t = q - g->width - 1;

#define ACC_ROW { if (j > 0) v += (t[0] >> 24); \
  v += (t[1] >> 24); if (j < g->width - 1) v += (t[2] >> 24); }
				if (i > 0)
					ACC_ROW
				t += g->width;

				ACC_ROW
				t += g->width;

				if (i < g->height - 1)
					ACC_ROW
#undef ACC_ROW

				v /= 9;

				*p++ = (*q++ & 0xffffff) | (v << 24);
			}
		}

		src ^= 1;
		dest ^= 1;
	}

	/* enhance */

	for (p = rgba[0]; p != &rgba[0][g->width*g->height]; p++) {
		unsigned v = (*p >> 24) << 3;
		if (v > 0xff)
			v = 0xff;
		*p = (*p & 0xffffff) | (v << 24);
	}

	free(rgba[1]);

	g->bitmap = rgba[0];
}

static void
gen_glyphs(void)
{
	FT_Library library;
	FT_Face face;
	int i;

	if ((FT_Init_FreeType(&library)) != 0)
		panic("FT_Init_FreeType");

	if ((FT_New_Face(library, ttf_filename, 0, &face)) != 0)
		panic("FT_New_Face");

	if ((FT_Set_Char_Size(face, font_size*64, 0, 100, 0)) != 0)
		panic("FT_Set_Char_Size");

	num_glyphs = end_code - start_code + 1;

	glyphs = calloc(num_glyphs, sizeof *glyphs);

	for (i = 0; i < num_glyphs; i++)
		init_glyph(&glyphs[i], i + start_code, face);

	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

static struct node *
node_make(int x, int y, int width, int height)
{
	struct node *p = malloc(sizeof *p);

	p->x = x;
	p->y = y;
	p->width = width;
	p->height = height;
	p->used = 0;
	p->left = p->right = NULL;

	return p;
}

void
node_free(struct node *root)
{
	if (root->left)
		node_free(root->left);

	if (root->right)
		node_free(root->right);

	free(root);
}

/* find smallest node in which box fits */
struct node *
node_find(struct node *root, int width, int height)
{
	struct node *p, *q;

	if (root->left) {
		/* not a leaf */

		p = node_find(root->left, width, height);

		q = node_find(root->right, width, height);

		if (p == NULL)
			return q;

		if (q == NULL)
			return p;

		return p->width*p->height < q->width*q->height ? p : q;
	} else {
		/* leaf node */

		/* already used? */
		if (root->used)
			return NULL;

		/* box fits? */
		if (root->width < width || root->height < height)
			return NULL;

		return root;
	}
}

struct node *
node_insert(struct node *root, int width, int height)
{
	struct node *p;

	if (root->left) {
		/* not a leaf */

		if ((p = node_insert(root->left, width, height)) != NULL)
			return p;

		return node_insert(root->right, width, height);
	} else {
		/* leaf */

		int dw, dh;

		if (root->used)
			return NULL;

		if (root->width < width || root->height < height)
			return NULL;

		if (root->width == width && root->height == height)
			return root;

		dw = root->width - width;
		dh = root->height - height;

		if (dw > dh) {
			root->left = node_make(root->x, root->y,
			  width, root->height);

			root->right = node_make(root->x + width,
			  root->y, root->width - width, root->height);
		} else {
			root->left = node_make(root->x, root->y,
			  root->width, height);

			root->right = node_make(root->x,
			  root->y + height, root->width,
			  root->height - height);
		}

		return node_insert(root->left, width, height);
	}
}

static int 
compare_glyph(const void *pa, const void *pb)
{
	const struct glyph *a = (const struct glyph *)pa;
	const struct glyph *b = (const struct glyph *)pb;

	return b->width*b->height - a->width*a->height;
}

static void
glyph_write_to_texture(const struct glyph *glyph)
{
	int i, j;
	unsigned *p;
	const unsigned *q;

	p = &texture[texture_width*glyph->texture_y + glyph->texture_x];
	q = glyph->bitmap;

	if (!glyph->transposed) {
		for (i = 0; i < glyph->height; i++) {
			memcpy(p, q, glyph->width*sizeof *p);
			p += texture_width;
			q += glyph->width;
		}
	} else {
		for (i = 0; i < glyph->height; i++) {
			for (j = 0; j < glyph->width; j++) {
				p[j*texture_width + i] = q[i*glyph->width + j];
			}
		}
	}
}

void
pack_glyphs(void)
{
	struct node *root;
	struct glyph *g;

	texture = calloc(sizeof *texture, texture_width*texture_height);

	/* sort glyphs by area */

	qsort(glyphs, num_glyphs, sizeof *glyphs, compare_glyph);

	/* pack glyphs in texture */

	root = node_make(0, 0, texture_width, texture_height);

	for (g = glyphs; g != &glyphs[num_glyphs]; g++) {
		struct node *n, *nt;

		/* smallest node in which glyph fits */
		n = node_find(root, g->width, g->height);

		/* smallest node in which transposed glyph fits */
		nt = node_find(root, g->height, g->width);

		if (n == NULL && nt == NULL)
			panic("texture too small?");

		if (nt == NULL || n->width*n->height <= nt->width*nt->height) {
			/* allocate node for glyph */

			n = node_insert(n, g->width, g->height);
			assert(n != NULL);
		} else {
			g->transposed = 1;

			/* allocate node for transposed glyph */

			n = node_insert(nt, g->height, g->width);
			assert(n != NULL);
		}

		n->used = 1;

		g->texture_x = n->x;
		g->texture_y = n->y;

		glyph_write_to_texture(g);
	}

	node_free(root);
}


#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

static void
write_texture(void)
{
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *fp;
	int i;

	if ((fp = fopen(texture_filename, "wb")) == NULL)
		panic("fopen");

	if ((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	  (png_voidp)NULL, NULL, NULL)) == NULL)
		panic("png_create_write_struct");

	if ((info_ptr = png_create_info_struct(png_ptr)) == NULL)
		panic("png_create_info_struct");

	if (setjmp(png_jmpbuf(png_ptr)))
		panic("png error");

	png_init_io(png_ptr, fp);

	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	png_set_IHDR(png_ptr, info_ptr, texture_width, texture_height, 8,
	  PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
	  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	for (i = 0; i < texture_height; i++)
		png_write_row(png_ptr,
		  (unsigned char *)&texture[i*texture_width]);

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}

static void
write_fontdef(void)
{
	FILE *fp;
	const struct glyph *g;

	if ((fp = fopen(fontdef_filename, "w")) == NULL)
		panic("fopen");

	fprintf(fp, "%s {\n", font_basename);

	fprintf(fp, "\tsize %d\n", font_size);

	fprintf(fp, "\tsource \"%s\"\n", texture_filename);

	for (g = glyphs; g != &glyphs[num_glyphs]; g++) {
		fprintf(fp, "\tglyph \"%s%c\" %d %d %d %d %d %d %d %d %d\n",
		  g->code == '\\' || g->code == '"' ? "\\" : "", g->code,
		  g->width, g->height, g->left, g->top,
		  g->advance_x, g->advance_y, g->texture_x, g->texture_y,
		  g->transposed);
	}

	fprintf(fp, "}\n");

	fclose(fp);
}

static void
usage(char *argv0)
{
	fprintf(stderr, "Usage: %s [options] <font file>\n", argv0);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options are:\n");
	fprintf(stderr, "  -S  font size\n");
	fprintf(stderr, "  -s  start glyph code\n");
	fprintf(stderr, "  -e  end glyph code\n");
	fprintf(stderr, "  -W  texture width\n");
	fprintf(stderr, "  -H  texture height\n");
	fprintf(stderr, "  -h  show usage\n");
	fprintf(stderr, "  -I  font base name\n");

	exit(1);
}

int
main(int argc, char *argv[])
{
	int c;

	font_size = 26;
	start_code = 32;
	end_code = 126;
	texture_width = 256;
	texture_height = 256;

	while ((c = getopt(argc, argv, "S:s:e:I:W:H:h")) != EOF) {
		char *after;

		switch (c) {
			case 'S':
				font_size = strtol(optarg, &after, 10);
				if (after == optarg)
					usage(*argv);
				break;

			case 's':
				start_code = strtol(optarg, &after, 10);
				if (after == optarg)
					usage(*argv);
				break;

			case 'e':
				end_code = strtol(optarg, &after, 10);
				if (after == optarg)
					usage(*argv);
				break;

			case 'I':
				strncpy(font_basename, optarg,
				  sizeof font_basename);
				break;

			case 'W':
				texture_width = strtol(optarg, &after, 10);
				if (after == optarg)
					usage(*argv);
				break;

			case 'H':
				texture_height = strtol(optarg, &after, 10);
				if (after == optarg)
					usage(*argv);
				break;

			case 'h':
				usage(*argv);
				break;
		}
	}

	if (optind == argc)
		usage(*argv);

	strncpy(ttf_filename, argv[optind], sizeof ttf_filename - 1);

	if (!*font_basename) {
		char temp[PATH_MAX + 1], *dot;

		strncpy(temp, ttf_filename, sizeof temp - 1);

		strncpy(font_basename, basename(temp),
		  sizeof font_basename - 1);
	
		dot = strrchr(font_basename, '.');
		if (dot != NULL)
			*dot = '\0';
	}

	snprintf(texture_filename, sizeof texture_filename,
	  "%s.png", font_basename);

	snprintf(fontdef_filename, sizeof fontdef_filename,
	  "%s.fontdef", font_basename);

	gen_glyphs();
	pack_glyphs();
	write_texture();
	write_fontdef();

	return 0;
}
