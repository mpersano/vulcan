/* render.c -- part of vulcan
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
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <sys/param.h>
#include <assert.h>

#include "panic.h"
#include "gl_util.h"
#include "list.h"
#include "dict.h"
#include "game.h"
#include "vector.h"
#include "matrix.h"
#include "mesh.h"
#include "bsptree.h"
#include "vrml_tree.h"
#include "image.h"
#include "cylinder.h"
#include "graphics.h"
#include "model.h"
#include "modeldef.h"
#include "render.h"
#include "pathnames.h"
#include "ui.h"
#include "ui_util.h"

#define MODEL_FILE "chess-models.modeldef"

#define MAX_ATTACK_BOARDS (NUM_MAIN_BOARDS*8)
#define MAX_BOARDS (NUM_MAIN_BOARDS + MAX_ATTACK_BOARDS)

static struct vector light_position_0;
static struct vector light_position_1;
static const GLfloat light_ambient[] = { 0.1f, 0.1f, 0.1f, 1.f };
static const GLfloat light_diffuse[] = { .8f, .8f, .8f, 1.f };
static const GLfloat light_specular[] = { .8f, .8f, .8f, 1.f };

static const GLfloat front_shininess[] = { 30.0 };
static const GLfloat front_specular[] = { 0.4, 0.4, 0.4, 1.0 };

struct model *piece_models[NUM_PIECES];
struct mesh *unit_cylinder;

static GLuint piece_texture;
static GLuint board_texture;

struct rgb {
	float r, g, b;
};

struct material {
	struct rgb ambient;
	struct rgb diffuse;
	struct rgb specular;
	int shine;
};

static const float board_alpha = .7f;

/* colors for black square for each selection state */
static struct rgb black_rgb[4] =
  { { .2f, .2f, .2f },		/* neutral */
    { .1f, .6f, .1f },		/* selected square */
    { .6f, .2f, 1.f },		/* attacked square */
    { .5f, .5f, .5f } };	/* highlighted square */

/* colors for white square for each selection state */
static struct rgb white_rgb[4] =
  { { .8f, .8f, .8f },		/* neutral */
    { .4f, 1.f, .4f },		/* selected square */
    { .6f, .2f, 1.f },		/* attacked square */
    { 1.f, 1.f, 1.f } };	/* highlighted square */

/* materials for white piece when unselected, selected or attacked */
static const struct material white_piece_material[4] =
  { { { .2f, .2f, .2f }, { .6f, .6f, .2f }, { .9f, .9f, .9f }, 12 },
    { { .2f, .2f, .2f }, { .4f, 1.f, .4f }, { .9f, .9f, .9f }, 12 },
    { { .2f, .2f, .2f }, { .6f, .2f, 1.f }, { .9f, .9f, .9f }, 12 },
    { { .6f, .6f, .6f }, { 1.f, 1.f, .6f }, { 1.f, 1.f, 1.f }, 2 } };

/* materials for black piece when unselected, selected or attacked */
static const struct material black_piece_material[4] = 
  { { { .2f, .2f, .2f }, { .9f, .2f, .2f }, { .9f, .9f, .9f }, 12 },
    { { .2f, .2f, .2f }, { .4f, 1.f, .4f }, { .9f, .9f, .9f }, 12 },
    { { .2f, .2f, .2f }, { .6f, .2f, 1.f }, { .9f, .9f, .9f }, 12 },
    { { .4f, .4f, .4f }, { 1.f, .4f, .4f }, { 1.f, 1.f, 1.f }, 2 } };

/* display lists */
static GLuint piece_disp_lists[NUM_PIECES];
static GLuint cylinder_disp_list;

void
set_opengl_material(const struct material *mat)
{
	GLfloat diffuse[4];
	GLfloat specular[4];

	memcpy(diffuse, &mat->diffuse, sizeof mat->diffuse);
	memcpy(specular, &mat->specular, sizeof mat->specular);

	diffuse[3] = specular[3] = 1.f;

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat->shine);
}

static void
set_lights(void)
{
	struct matrix r;
	union {
		struct vector v;
		float f[4];
	} light_pos;

	light_pos.f[3] = 0.f;

	ui_set_modelview_matrix(&r);

	mat_rotate_copy(&light_pos.v, &r, &light_position_0);

	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos.f);

	mat_rotate_copy(&light_pos.v, &r, &light_position_1);

	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_POSITION, light_pos.f);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_specular);
}

static void
render_mesh(const struct mesh *mesh)
{
	struct polygon *poly, *end;
	struct vertex *mesh_vtx, *v0, *v1, *v2;
	int *vtx_index;
	int i;
	float min_z, max_z;

	glBegin(GL_TRIANGLES);

	mesh_vtx = mesh->vtx;

	min_z = max_z = mesh->vtx[0].pos.z;

	for (i = 1; i < mesh->nvtx; i++) {
		if (mesh->vtx[i].pos.z < min_z)
			min_z = mesh->vtx[i].pos.z;

		if (mesh->vtx[i].pos.z > max_z)
			max_z = mesh->vtx[i].pos.z;
	}

	end = &mesh->poly[mesh->npoly];

	for (poly = mesh->poly; poly != end; poly++) {
		vtx_index = poly->vtx_index;
		v0 = &mesh_vtx[vtx_index[0]];

		for (i = 1; i < poly->nvtx - 1; i++) {
			v1 = &mesh_vtx[vtx_index[i]];
			v2 = &mesh_vtx[vtx_index[i + 1]];

#define SET_TEXTURE_COORD(p) \
  glTexCoord2f((atan2f((p).x, (p).y) + M_PI)/2.f*M_PI, ((p).z - min_z)/max_z)

			glNormal3f(v0->normal.x, v0->normal.y, v0->normal.z);
			SET_TEXTURE_COORD(v0->pos);
			glVertex3f(v0->pos.x, v0->pos.y, v0->pos.z);

			glNormal3f(v1->normal.x, v1->normal.y, v1->normal.z);
			SET_TEXTURE_COORD(v1->pos);
			glVertex3f(v1->pos.x, v1->pos.y, v1->pos.z);

			glNormal3f(v2->normal.x, v2->normal.y, v2->normal.z);
			SET_TEXTURE_COORD(v2->pos);
			glVertex3f(v2->pos.x, v2->pos.y, v2->pos.z);

#undef SET_TEXTURE_COORD
		}
	}

	glEnd();
}


static void
init_display_list_from_mesh(GLuint display_list, struct mesh *mesh)
{
	glNewList(display_list, GL_COMPILE);

	if (glGetError() != GL_NO_ERROR)
		panic("glNewList failed");

	render_mesh(mesh);

	glEndList();
}


static void
render_display_list(GLuint display_list, const struct matrix *m)
{
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

	set_opengl_matrix(m);

	glCallList(display_list);

	glPopMatrix();
}

void
render_piece_at(const struct matrix *mv, const struct vector *pos, int piece,
  int flags, int reflected)
{
	struct matrix m, t;
	int p;

	glDisable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_BLEND);

	if (ui.do_textures) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, piece_texture);
	}

	if (reflected) {
		light_position_0.z = -light_position_0.z; /* HACK */
		light_position_1.z = -light_position_1.z; /* HACK */
	}

	set_lights();

	if (reflected) {
		light_position_0.z = -light_position_0.z;
		light_position_1.z = -light_position_1.z;
	}

	mat_make_translation_from_vec(&t, pos);

	mat_mul_copy(&m, mv, &t);

	if (piece & BLACK_FLAG) {
		/* if it's a black piece, rotate it
		   180 degrees around the z axis */

		struct matrix r;

		mat_make_rotation_around_z(&r, M_PI);
		mat_mul_copy(&m, &m, &r);

		set_opengl_material(&black_piece_material[flags]);
	} else {
		set_opengl_material(&white_piece_material[flags]);
	}

	if (reflected) {
		struct matrix s;

		mat_make_scale_xyz(&s, 1.f, 1.f, -1.f);
		mat_mul_copy(&m, &m, &s);

		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
	}

	p = (piece & PIECE_MASK) - PAWN;

	assert(p >= 0 && p < NUM_PIECES);

	mat_mul_copy(&m, &m, &piece_models[p]->transform);

	render_display_list(piece_disp_lists[p], &m);
}

static void
render_board_pieces(const struct matrix *mv, const struct vector *board_pos,
  const unsigned char *state, const unsigned char *flags, int size,
  int reflected)
{
	int i, j, s;
	float start_x, start_y;
	unsigned const char *pf, *ps;
	struct vector p;

	start_x = board_pos->x - .5f*SQUARE_SIZE*(size - 1);
	start_y = board_pos->y - .5f*SQUARE_SIZE*(size - 1);
	p.z = board_pos->z;

	p.x = start_x;
	pf = flags;
	ps = state;

	for (i = 0; i < size; i++) {
		p.y = start_y;

		for (j = 0; j < size; j++) {
			s = *ps++;

			if (s != EMPTY)
				render_piece_at(mv, &p, s, *pf, reflected);

			p.y += SQUARE_SIZE;

			++pf;
		}

		ps += BCOLS - size;
		pf += BCOLS - size;

		p.x += SQUARE_SIZE;
	}
}


static void
render_board_squares(const struct matrix *mv, const struct vector *pos,
  const unsigned char *state, const unsigned char *flags, int size,
  int transparent)
{
	int i, j;
	float x, y, z, start_x, start_y;
	struct rgb *color;
	unsigned const char *pf;
	int s;

	start_x = pos->x - .5f*SQUARE_SIZE*size;
	start_y = pos->y - .5f*SQUARE_SIZE*size;
	z = pos->z;

	/* glPushAttrib(GL_ALL_ATTRIB_BITS); */

	if (transparent) {
		glDisable(GL_LIGHTING);
		glShadeModel(GL_FLAT);
	} else {
		glShadeModel(GL_SMOOTH);

		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		set_lights();
	}

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

	set_opengl_matrix(mv);

	if (ui.do_textures) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, board_texture);
	}

	glBegin(GL_QUADS);

	x = start_x;
	pf = flags;

	for (i = 0; i < size; i++) {
		y = start_y;

		for (j = 0; j < size; j++) {
			s = *pf++;

			color = &(((i^j)&1) ? black_rgb : white_rgb)[s];

			glColor4f(color->r, color->g, color->b, board_alpha);

			glNormal3f(0.f, 0.f, 1.f);
			glTexCoord2f(i*.2, j*.2);
			glVertex3f(x, y, z);
			glTexCoord2f((i + 1)*.2, j*.2);
			glVertex3f(x + SQUARE_SIZE, y, z);
			glTexCoord2f((i + 1)*.2, (j + 1)*.2);
			glVertex3f(x + SQUARE_SIZE, y + SQUARE_SIZE, z);
			glTexCoord2f(i*.2, (j + 1)*.2);
			glVertex3f(x, y + SQUARE_SIZE, z);

			y += SQUARE_SIZE;
		}

		pf += BCOLS - size;
		x += SQUARE_SIZE;
	}

	glEnd();

	glPopMatrix();

	/* glPopAttrib(); */

	/* glDisable(GL_COLOR_MATERIAL); */
}

static void
render_board_reflections(const struct matrix *mv,
  const struct vector *pos, const unsigned char *state,
  const unsigned char *flags, int size)
{
	float z, start_x, start_y;

	start_x = pos->x - .5f*SQUARE_SIZE*size;
	start_y = pos->y - .5f*SQUARE_SIZE*size;
	z = pos->z;

	glEnable(GL_STENCIL_TEST);

	glClear(GL_STENCIL_BUFFER_BIT);

	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDisable(GL_CULL_FACE);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

	set_opengl_matrix(mv);

	glDisable(GL_DEPTH_TEST);
	glBegin(GL_QUADS);

	glVertex3f(start_x, start_y, z);
	glVertex3f(start_x + size*SQUARE_SIZE, start_y, z);
	glVertex3f(start_x + size*SQUARE_SIZE, start_y + size*SQUARE_SIZE, z);
	glVertex3f(start_x, start_y + size*SQUARE_SIZE, z);

	glEnd();

	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glEnable(GL_STENCIL_TEST);

	glStencilFunc(GL_EQUAL, 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	render_board_pieces(mv, pos, state, flags, size, 1);

	glDisable(GL_STENCIL_TEST);
}

static void
render_board(const struct matrix *mv, const struct vector *board_pos,
  const unsigned char *state, const unsigned char *flags, int size)
{
	if (!ui.do_reflections) {
		render_board_pieces(mv, board_pos, state, flags, size, 0);
		render_board_squares(mv, board_pos, state, flags, size, 1);
	} else {
		render_board_reflections(mv, board_pos, state, flags, size);
		render_board_squares(mv, board_pos, state, flags, size, 0);
		render_board_pieces(mv, board_pos, state, flags, size, 0);

	}
}


/*
 * render_pins --
 *	Render the pins that connect main boards to attack boards.
 */
static void
render_pins(const struct board_state *state, const struct matrix *mv)
{
	struct matrix m, s, t;
	struct vector pos;
	int i, j;

	/* render cylinders */

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_COLOR_MATERIAL);

	set_lights();

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j)) {
				get_attack_board_xyz(&pos, i, j);

				mat_make_scale_xyz(&s, 3.f, 3.f,
				  ATTACK_BOARD_DISTANCE);

				if (ATTACK_BOARD_IS_ABOVE(j))
					pos.z -= ATTACK_BOARD_DISTANCE;

				mat_make_translation_from_vec(&t, &pos);
				mat_mul_copy(&m, &t, &s);
				mat_mul_copy(&m, mv, &m);

				set_opengl_material(
				  ATTACK_BOARD_IS_BLACK(state, i, j) ?
				    &black_piece_material[0] :
				    &white_piece_material[0]);

				render_display_list(cylinder_disp_list, &m);
			}
		}
	}

	glDisable(GL_NORMALIZE);
}


/*
 * render_boards --
 *	Render main boards and attack boards for the given game state, in
 *	back-to-front order.
 */
static void
render_boards(const struct board_state *state,
  const struct selected_squares *flags, const struct matrix *mv)
{
	static struct board_to_render {
		struct vector orig_pos;		/* untransformed position of
						 * center of the board */
		struct vector pos;		/* transformed position */
		int size;			/* in squares */
		const unsigned char *state;
		const unsigned char *flags;
	} boards[MAX_BOARDS];
	int render_order[MAX_BOARDS];
	int num_boards;
	struct board_to_render *p;
	struct position pos;
	int i, j;

	/* sort boards back to front (for transparency) */

	p = boards;
	num_boards = 0;

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		/* main board */

		assert(num_boards < MAX_BOARDS);

		p->size = MAIN_BOARD_SIZE;
		get_main_board_xyz(&p->orig_pos, i);
		mat_transform_copy(&p->pos, mv, &p->orig_pos);

		get_main_board_position(&pos, i);
		p->state = &state->board[pos.level][pos.square];
		p->flags = &flags->selection[pos.level][pos.square];

		++p;
		++num_boards;

		/* attack boards */

		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j)) {
				assert(num_boards < MAX_BOARDS);

				p->size = ATTACK_BOARD_SIZE;
				get_attack_board_xyz(&p->orig_pos, i, j);
				mat_transform_copy(&p->pos, mv, &p->orig_pos);

				get_attack_board_position(&pos, i, j);
				p->state = &state->board[pos.level][pos.square];
				p->flags = &flags->selection[pos.level]
				  [pos.square];

				++p;
				++num_boards;
			}
		}
	}

	/* bubble sort is enough for N this small */

	for (i = 0; i < num_boards; i++)
		render_order[i] = i;

	for (i = 0; i < num_boards; i++) {
		for (j = num_boards - 1; j > i; j--) {
			int test;

			if (ui.do_reflections) {
				test = boards[render_order[j - 1]].pos.z <
				  boards[render_order[j]].pos.z;
			} else {
				test = boards[render_order[j - 1]].pos.z >
				  boards[render_order[j]].pos.z;
			}

			if (test) {
				int t = render_order[j - 1];
				render_order[j - 1] = render_order[j];
				render_order[j] = t;
			}
		}
	}

	/* render boards */

	for (i = 0; i < num_boards; i++) {
		p = &boards[render_order[i]];
		render_board(mv, &p->orig_pos, p->state, p->flags, p->size);
	}
}


/*
 * render_board_state --
 *	Render a board state.
 */
void
render_board_state(const struct board_state *s,
  const struct selected_squares *f, const struct matrix *mv)
{
	if (ui.do_reflections) {
		render_boards(s, f, mv);
		render_pins(s, mv);
	} else {
		render_pins(s, mv);
		render_boards(s, f, mv);
	}
}


/*----------------------------------------------------------------------------
 *
 *	vrml mesh loading
 *
 */

static void
vrml_mesh_init_vtx(struct mesh *m, const struct list *points)
{
	struct list_node *ln;
	struct vertex *p;

	m->nvtx = points->length;
	p = m->vtx = malloc(m->nvtx * sizeof *m->vtx);

	for (ln = points->first; ln; ln = ln->next) {
		struct vrml_vector *v = (struct vrml_vector *)ln->data;
		vec_set(&p->pos, v->x, v->y, v->z);
		++p;
	}
}

static void
vrml_mesh_init_polys(struct mesh *m, const struct list *ifaces)
{
	struct list_node *ln;
	struct polygon *p, *end;
	int npoly;
	long idx;

	/* count polygons */

	npoly = 0;

	for (ln = ifaces->first; ln; ln = ln->next) {
		idx = (long)ln->data;

		if (idx == -1 || ln->next == NULL)
			++npoly;
	}

	/* initialize polygons */

	m->npoly = npoly;
	p = m->poly = malloc(m->npoly * sizeof *m->poly);

	end = &m->poly[m->npoly];
	ln = ifaces->first;

	for (p = m->poly; p != end; p++) {
		p->nvtx = 0;

		while (ln != NULL) {
			idx = (long)ln->data;
			ln = ln->next;

			if (idx == -1)
				break;

			assert(p->nvtx != MAX_POLY_VTX);

			p->vtx_index[p->nvtx++] = idx;
		}

		assert(p->nvtx == 3);
	}
}

static struct mesh *
vrml_mesh_make(struct vrml_node_coordinate3 *coord_node,
  struct vrml_node_indexed_face_set *iface_node)
{
	struct mesh *m;

	m = mesh_make();

	vrml_mesh_init_vtx(m, coord_node->points);
	vrml_mesh_init_polys(m, iface_node->indexes);

	return m;
}

struct vrml_context {
	struct vrml_node_coordinate3 *cur_coords_node;
};

static void
vrml_visit_node_list(struct vrml_context *ctx, const struct list *node_list,
  struct list *mesh_list)
{
	struct vrml_context cur_ctx = *ctx;
	struct list_node *ln;

	for (ln = node_list->first; ln; ln = ln->next) {
		union vrml_node *node = (union vrml_node *)ln->data;

		switch (node->common.type) {
			case VRML_NODE_SEPARATOR:
				vrml_visit_node_list(&cur_ctx,
				  node->separator.children, mesh_list);
				break;

			case VRML_NODE_COORDINATE3:
				cur_ctx.cur_coords_node = &node->coordinate3;
				break;

			case VRML_NODE_INDEXED_FACE_SET:
				if (cur_ctx.cur_coords_node == NULL)
					panic("vrml missing coords node?");

				list_append(mesh_list,
				  vrml_mesh_make(cur_ctx.cur_coords_node,
				    &node->indexed_face_set));
				break;

			default:
				break;
		}
	}
}

static struct list *
vrml_extract_meshes(struct list *vrml_node_list)
{
	struct vrml_context ctx = {0};
	struct list *mesh_list;

	mesh_list = list_make();

	vrml_visit_node_list(&ctx, vrml_node_list, mesh_list);

	return mesh_list;
}

void
load_piece(struct model *piece_model, const char *file_name)
{
	struct list *vrml_node_list;
	struct list *mesh_list;
	struct mesh *mesh;
	struct matrix s, rz, rx;

	if ((vrml_node_list = vrml_parse_file(file_name)) == NULL)
		panic("failed to load %s", file_name);

	mesh_list = vrml_extract_meshes(vrml_node_list);
	list_free(vrml_node_list, (void(*)(void *))vrml_node_free);

	assert(mesh_list->length == 1);

	mesh = (struct mesh *)mesh_list->first->data;
	list_free(mesh_list, NULL);

	mat_make_scale(&s, .095f);
	mesh_transform_all_vertices(mesh, &s);

	mesh_init_poly_normals(mesh);
	mesh_init_vertex_normals(mesh);

	piece_model->mesh = mesh;

	piece_model->bsp_tree = bsp_tree_build_from_mesh(mesh);

	mat_make_rotation_around_z(&rz, -.5*M_PI);
	mat_make_rotation_around_x(&rx, .5*M_PI);
	mat_mul_copy(&piece_model->transform, &rz, &rx);

	model_find_bounding_sphere(piece_model);
}

static void
load_models(void)
{
	struct dict *model_dict;
	static const char *models[NUM_PIECES] = {
		"pawn",
		"rook",
		"knight",
		"bishop",
		"queen",
		"king"
	};
	static char model_file[MAXPATHLEN];
	int i;
	struct matrix rz, rx;

	model_dict = dict_make();

	snprintf(model_file, sizeof model_file, "%s/%s", MODEL_DIR, MODEL_FILE);

	modeldef_parse_file(model_dict, model_file);

	for (i = 0; i < NUM_PIECES; i++) {
		struct model *piece_model;

		piece_model = dict_get(model_dict, models[i]);

		if (piece_model == NULL)
			panic("model `%s' not found?", models[i]);

		mat_make_rotation_around_z(&rz, -.5*M_PI);
		mat_make_rotation_around_x(&rx, .5*M_PI);
		mat_mul_copy(&piece_model->transform, &rz, &rx);

		printf("loaded %s: %d vtx, %d poly, %d nodes in bsp tree\n",
		  models[i],
		  piece_model->mesh->nvtx,
		  piece_model->mesh->npoly,
		  piece_model->bsp_tree->num_children + 1);

		piece_models[i] = piece_model;
	}

	dict_free(model_dict, NULL);

	unit_cylinder = make_cylinder(10.f, 1.f, 1.f);
}

static GLuint
load_texture(const char *file_name)
{
	static char texture_file[MAXPATHLEN];
	struct image *image;
	GLuint texture_id;

	snprintf(texture_file, sizeof texture_file, "%s/%s", TEXTURE_DIR,
	  file_name);

	image = image_make_from_png(texture_file);
	texture_id = image_to_opengl_texture(image);
	image_free(image);

	return texture_id;
}

static void
load_textures(void)
{
	board_texture = load_texture("board-texture.png");
	piece_texture = load_texture("piece-texture.png");
}

static void
init_display_lists(void)
{
	int i;
	GLuint base;

	if ((base = glGenLists(NUM_PIECES + 1)) == 0)
		panic("glGenLists failed");

	for (i = 0; i < NUM_PIECES; i++) {
		piece_disp_lists[i] = base + i;
		init_display_list_from_mesh(piece_disp_lists[i],
		  piece_models[i]->mesh);
	}

	cylinder_disp_list = base + NUM_PIECES;
	init_display_list_from_mesh(cylinder_disp_list, unit_cylinder);
}

/*
 * init_render --
 *	Create display lists for pieces and cylinder.
 */
void
init_render(void)
{
	load_models();
	load_textures();
	init_display_lists();
	vec_set(&light_position_0, 2000.f, -8000.f, 5000.f);
	vec_set(&light_position_1, -2000.f, 8000.f, 5000.f);
}
