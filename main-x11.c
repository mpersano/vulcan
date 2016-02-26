/* main-x11.c -- part of vulcan
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
#include <GL/gl.h>
#include <GL/glu.h>
#include <assert.h>
#include <sys/wait.h>
#include <GL/glx.h>
#include <X11/keysym.h>
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
#define VBUTTON(x) ((x) == Button1 ? BTN_LEFT : (x) == Button3 ? BTN_RIGHT : 0)

enum {
	WIDTH = 640,			/* window start width */
	HEIGHT = 512,			/* window start height */
	DEFAULT_MAX_DEPTH = 4,		/* default AI search depth */
	CHILD_WAIT_TIMEOUT = 1,		/* in seconds */
	UTIMER = 33333
};

static Atom close_atom;

struct worker_thread {
	pid_t pid;
	int write_to_fd, read_from_fd; /* pipes */
};

struct worker_request {
	struct board_state state;
	int side;
	int max_depth;
};

static Display *the_display;
static Window the_window;
static struct worker_thread the_worker;

long
msecs(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return
	  (unsigned long)tv.tv_sec*1000UL +
	  (unsigned long)tv.tv_usec/1000UL;
}

void
swap_buffers(void)
{
	glXSwapBuffers(the_display, the_window);
}

static void
sys_sigblock(int sig)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, sig);
	sigprocmask(SIG_BLOCK, &mask, (sigset_t *)NULL);
}


static void
sys_sigrelease(int sig)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, sig);
	sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)NULL);
}

static
void (*sys_sigset(int sig, void(*func)(int)))(int)
{
	struct sigaction act, oact;

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = func;
	sigaction(sig, &act, &oact);

	return oact.sa_handler;
}

static void
reap_children(int ignored)
{
	int res;

	sys_sigblock(SIGCHLD);

	for (;;) {
		while ((res = waitpid((pid_t)-1, NULL, WNOHANG)) > 0)
			;

		if (!(res < 0 && errno == EAGAIN))
			break;
	}

	sys_sigrelease(SIGCHLD);
}

static void
kill_worker(struct worker_thread *w)
{
	fd_set fds;
	struct timeval tm;
	int r;

	kill(w->pid, SIGUSR1);

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(w->read_from_fd, &fds);

		tm.tv_sec = CHILD_WAIT_TIMEOUT;
		tm.tv_usec = 0;

		if ((r = select(w->read_from_fd + 1, &fds, NULL, NULL, &tm)) < 0) {
			if (errno == EINTR)
				continue;

			warn("select failed: %s", strerror(errno));
			break;
		} else if (r == 0) {
		} else {
			int r;
			char buf[512];

			if ((r = read(w->read_from_fd, buf, sizeof buf)) < 0) {
				if (errno == EINTR)
					continue;

				warn("child fd not closed properly?");
				break;
			}

			if (r == 0)
				break;
		}
	}

	close(w->read_from_fd);
	close(w->write_to_fd);
}

static ssize_t
read_exact(int fd, void *vbuf, size_t length)
{
	ssize_t r, remaining;
	char *buf;

	buf = vbuf;
	remaining = length;

	for (;;) {
		if ((r = read(fd, buf, remaining)) == remaining)
			break;
		
		if (r < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				warn("read error: %s", strerror(errno));
				return -1;
			}
		} else if (r == 0) {
			warn("eof while reading from pipe");

			return remaining == length ? 0 : -1;
		}

		remaining -= r;
		buf += r;
	}

	return length;
}

static int
write_exact(int fd, void *vbuf, size_t length)
{
	size_t r, remaining;
	char *buf;

	buf = vbuf;
	remaining = length;

	for (;;) {
		if ((r = write(fd, buf, remaining)) == remaining)
			break;

		if (r < 0) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EPIPE) {
				return 0;
			} else {
				return -1;
			}
		}
		
		remaining -= r;
		buf += r;
	}

	return length;
}

static void
worker_loop(void)
{
	struct worker_request req;
	union move next_move;

	for (;;) {
		/* read request */
		if (read_exact(STDIN_FILENO, &req, sizeof req) != sizeof req)
			panic("worker got eof while reading data");

		get_best_move(&next_move, &req.state, req.side, req.max_depth);

		/* write answer */
		write(STDOUT_FILENO, &next_move, sizeof next_move);
	}
}

static void
create_worker(struct worker_thread *worker)
{
	int p0[2], p1[2];
	pid_t child;

	if (pipe(p0) || pipe(p1))
		panic("pipe failed: %s", strerror(errno));

	if ((child = fork()) < 0) {
		panic("fork failed: %s", strerror(errno));
	} else if (child > 0) {
		/* parent */
		close(p0[1]);
		close(p1[0]);
		worker->write_to_fd = p1[1];
		worker->read_from_fd = p0[0];
		worker->pid = child;
	} else {
		/* child */
		close(p1[1]);
		close(p0[0]);

		if (dup2(p1[0], STDIN_FILENO) < 0 ||
		  dup2(p0[1], STDOUT_FILENO) < 0) {
			panic("dup2 failed: %s", strerror(errno));
		}

		close(p1[0]);
		close(p0[1]);
		sys_sigset(SIGCHLD, SIG_IGN);

		worker_loop();
	}
}

int
send_worker_request(void)
{
	struct worker_request req;

	req.state = ui.board_state;
	req.side = ui.cur_side;
	req.max_depth = ui.max_depth;

	if (write_exact(the_worker.write_to_fd, &req, sizeof req) < 0)
		return -1;

	return 0;
}

static void
init_signals(void)
{
	sys_sigset(SIGCHLD, &reap_children);
	sys_sigset(SIGPIPE, SIG_IGN);
}

static void
on_key_press(XKeyEvent *xkey)
{
	int code;

	code = XLookupKeysym(xkey, 0);

	switch (code) {
		case XK_Control_L:
		case XK_Control_R:
			ui.control_key_down = 1;
			break;

		case XK_Escape:
			ui_on_escape_release();
			break;
	}
}

static void
on_key_release(XKeyEvent *xkey)
{
	int code;

	code = XLookupKeysym(xkey, 0);

	switch (code) {
		case XK_Control_L:
		case XK_Control_R:
			ui.control_key_down = 0;
			break;

		case XK_Escape:
			ui_on_escape_press();
			break;
	}
}

static int
on_x_event(void)
{
	XEvent event;
	int done;

	done = 0;

	while (XPending(the_display)) {
		XNextEvent(the_display, &event);

		switch (event.type) {
			case ClientMessage:
				if ((Atom)(event.xclient.data.l[0]) ==
				  close_atom)
				  	ui.to_quit = 1;
				break;

			case ButtonPress:
				ui_on_button_press(
				  VBUTTON(event.xbutton.button),
				  event.xbutton.x, event.xbutton.y);
				break;

			case ButtonRelease:
				ui_on_button_release(
				  VBUTTON(event.xbutton.button),
				  event.xbutton.x, event.xbutton.y);
				break;

			case KeyPress:
				on_key_press(&event.xkey);
				break;

			case KeyRelease:
				on_key_release(&event.xkey);
				break;

			case MotionNotify:
				while (XCheckTypedWindowEvent(the_display,
				  the_window, MotionNotify, &event))
					;

				ui_on_motion_notify(
				  VBUTTON(event.xbutton.button),
				  event.xbutton.x, event.xbutton.y);
				break;

			case Expose:
				ui_redraw();
				break;

			case ConfigureNotify:
				ui_reshape(event.xconfigure.width,
				  event.xconfigure.height);
				break;
		}
	}

	return done;
}

static void
on_x_worker_reply(void)
{
	union move move = {0};

	if (read_exact(the_worker.read_from_fd, &move, sizeof move) !=
	  sizeof move)
		panic("invalid data from worker");

	ui_on_worker_reply(&move);
}

void
event_loop(void)
{
	fd_set fds;
	int xfd, max_fd;
	struct timeval tv;
	long prev_ticks, to_wait;

	xfd = ConnectionNumber(the_display);
	max_fd = MAX(xfd, the_worker.read_from_fd);

	prev_ticks = msecs();

	on_x_event();

	while (!ui.to_quit) {
		ui_on_idle();

		ui_redraw();

		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		FD_SET(the_worker.read_from_fd, &fds);

		to_wait = UTIMER - (msecs() - prev_ticks);

		if (to_wait < 0)
			to_wait = 0;

		tv.tv_sec = to_wait/1000000ul;
		tv.tv_usec = to_wait%1000000ul;

		if ((select(max_fd + 1, &fds, NULL, NULL, &tv)) < 0) {
			if (errno != EINTR)
				panic("select failed: %s", strerror(errno));
		}

		prev_ticks = msecs();

		if (FD_ISSET(xfd, &fds))
			on_x_event();

		if (FD_ISSET(the_worker.read_from_fd, &fds))
			on_x_worker_reply();
	}
}

static Window
make_window(Display *display, int width, int height)
{
	int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    GLX_DEPTH_SIZE, 1,
		    GLX_STENCIL_SIZE, 1,
		    None };
	int screen;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	Window window;
	GLXContext ctx;
	XVisualInfo *visinfo;

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);

	if ((visinfo = glXChooseVisual(display, screen, attrib)) == NULL)
	   panic("couldn't get RGB, double-buffered visual");

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(display, root, visinfo->visual,
	  AllocNone);
	attr.event_mask = StructureNotifyMask|ExposureMask|ButtonPressMask|
	  ButtonReleaseMask|KeyPressMask|KeyReleaseMask|PointerMotionMask;
	mask = CWBackPixel|CWBorderPixel|CWColormap|CWEventMask;

	window = XCreateWindow(display, root, 0, 0, width, height, 0,
	  visinfo->depth, InputOutput, visinfo->visual, mask, &attr);

	if ((ctx = glXCreateContext(display, visinfo, NULL, True)) == NULL)
	   panic("glXCreateContext failed");

	glXMakeCurrent(display, window, ctx);

	return window;
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

	if ((the_display = XOpenDisplay(NULL)) == NULL)
		panic("couldn't open display");

	the_window = make_window(the_display, WIDTH, HEIGHT);

	if ((close_atom = XInternAtom(the_display, "WM_DELETE_WINDOW", 1)))
		XSetWMProtocols(the_display, the_window, &close_atom, 1);

	XMapWindow(the_display, the_window);

	ui_initialize(white_player, black_player, max_depth, WIDTH, HEIGHT);

	gl_initialize();

	ui_set_state(STATE_MAIN_MENU);

	init_signals();

	create_worker(&the_worker);

	event_loop();

	XCloseDisplay(the_display);

	kill_worker(&the_worker);

	return 0;
}
