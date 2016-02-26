/* main-sdl.c -- part of vulcan
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
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "version.h"
#include "panic.h"
#include "list.h"
#include "render.h"
#include "vector.h"
#include "matrix.h"
#include "mesh.h"
#include "bsptree.h"
#include "model.h"
#include "list.h"
#include "cylinder.h"
#include "pick.h"
#include "move.h"
#include "game.h"
#include "engine.h"
#include "font.h"
#include "ui.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define VBUTTON(x) ((x) == SDL_BUTTON_LEFT ? BTN_LEFT : (x) == SDL_BUTTON_RIGHT ? (BTN_RIGHT) : 0)

enum {
	WIDTH = 640,			/* window start width */
	HEIGHT = 512,			/* window start height */
	DEFAULT_MAX_DEPTH = 4,		/* default AI search depth */
	CHILD_WAIT_TIMEOUT = 1,		/* in seconds */
	UTIMER = 33
};

static struct worker_request {
	struct board_state state;
	int side;
	int max_depth;
} worker_request;

static struct {
	SDL_Thread *thread;
	union move next_move;
	int request_ready;
	SDL_mutex *request_mutex;
	SDL_cond *request_cond;
} worker_thread;

static int sdl_flags;

unsigned long
msecs(void)
{
	return SDL_GetTicks();
}

void
swap_buffers(void)
{
	SDL_GL_SwapBuffers();
}

static int 
worker_loop(void *dummy)
{
	int done = 0;

	while (!done) {
		struct worker_request req;
		SDL_Event event;

		SDL_mutexP(worker_thread.request_mutex);

		while (!worker_thread.request_ready)
			SDL_CondWait(worker_thread.request_cond,
			  worker_thread.request_mutex);

		done = worker_thread.request_ready == -1;

		worker_thread.request_ready = 0;

		if (!done)
			req = worker_request;

		SDL_mutexV(worker_thread.request_mutex);

		if (!done) {
			get_best_move(&worker_thread.next_move, &req.state,
			  req.side, req.max_depth);

			event.type = SDL_USEREVENT;
			SDL_PushEvent(&event);
		}
	}

	return 0;
}

static void
init_worker(void)
{
	worker_thread.request_cond = SDL_CreateCond();
	worker_thread.request_mutex = SDL_CreateMutex();
	worker_thread.request_ready = 0;

	worker_thread.thread = SDL_CreateThread(worker_loop, NULL);
}

static void
kill_worker(void)
{
	int dummy;

	SDL_mutexP(worker_thread.request_mutex);

	worker_thread.request_ready = -1;

	SDL_CondSignal(worker_thread.request_cond);
	SDL_mutexV(worker_thread.request_mutex);

	SDL_WaitThread(worker_thread.thread, &dummy);
}

int
send_worker_request(void)
{
	SDL_mutexP(worker_thread.request_mutex);

	worker_request.state = ui.board_state;
	worker_request.side = ui.cur_side;
	worker_request.max_depth = ui.max_depth;

	worker_thread.request_ready = 1;

	SDL_CondSignal(worker_thread.request_cond);
	SDL_mutexV(worker_thread.request_mutex);

	return 0;
}

void
event_loop(void)
{
	int done;
	SDL_Event event;
	long prev_ticks, to_wait;

	ui_reshape(WIDTH, HEIGHT);

	done = 0;

	prev_ticks = msecs();

	while (!done && !ui.to_quit) {
		ui_on_idle();

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					ui_on_button_press(
					  VBUTTON(event.button.button),
					  event.button.x, event.button.y);
					break;

				case SDL_MOUSEBUTTONUP:
					ui_on_button_release(
					  VBUTTON(event.button.button),
					  event.button.x, event.button.y);
					break;

				case SDL_MOUSEMOTION:
					ui_on_motion_notify(
					  VBUTTON(event.button.button),
					  event.motion.x, event.motion.y);
					break;

				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE)
						ui_on_escape_press();
					break;

				case SDL_KEYUP:
					if (event.key.keysym.sym == SDLK_ESCAPE)
						ui_on_escape_release();
					break;

				case SDL_VIDEORESIZE:
					ui_reshape(event.resize.w,
					  event.resize.h);

					break;

				case SDL_USEREVENT:
					/* answer from worker */
					ui_on_worker_reply(
					  &worker_thread.next_move);
					break;

				case SDL_QUIT:
					done = 1;
					break;

				default:
					break;
			}
		}

		ui_redraw();

		to_wait = UTIMER - (msecs() - prev_ticks);

		if (to_wait > 0)
			SDL_Delay(to_wait);

		prev_ticks = msecs();
	}
}

static void
initialize_sdl(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		panic("SDL_Init: %s", SDL_GetError());

	if (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1) < 0)
		panic("SDL_GL_SetAttribute: %s", SDL_GetError());

	sdl_flags = SDL_OPENGL/*|SDL_RESIZABLE*/;

	if (SDL_SetVideoMode(WIDTH, HEIGHT, 16, sdl_flags) == NULL)
		panic("SDL_SetVideoMode: %s", SDL_GetError());

	SDL_WM_SetCaption(VERSION, NULL);
}

static void
usage(void)
{
	fprintf(stderr, VERSION"\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  Usage: vulcan [options]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options are:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -c   computer vs. computer\n");
	fprintf(stderr, "  -u   human vs. human\n");
	fprintf(stderr, "  -b   play black\n");
	fprintf(stderr, "  -d   set maximum AI search depth\n");

	exit(1);
}

int
main(int argc, char *argv[])
{
	int c;
	enum player_type white_player, black_player;
	int max_depth;
	char *p;

	white_player = HUMAN_PLAYER;
	black_player = COMPUTER_PLAYER;
	max_depth = DEFAULT_MAX_DEPTH;

	while ((c = getopt(argc, argv, "cubd:h")) != EOF) {
		switch (c) {
			case 'c':
				white_player = black_player = COMPUTER_PLAYER;
				break;

			case 'u':
				white_player = black_player = HUMAN_PLAYER;
				break;

			case 'b':
				white_player = COMPUTER_PLAYER;
				black_player = HUMAN_PLAYER;
				break;

			case 'd':
				max_depth = strtol(optarg, &p, 10);
				if (p == optarg || max_depth < 1)
					usage();
				break;

			case 'h':
			default:
				usage();
		}
	}

	ui_initialize(white_player, black_player, max_depth, WIDTH, HEIGHT);

	initialize_sdl();
	gl_initialize();

	ui_set_state(STATE_MAIN_MENU);

	/* init_signals(); */
	init_worker();

	event_loop();

	kill_worker();

	SDL_Quit();

	return 0;
}
