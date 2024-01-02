#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "bullet.h"
#include "list.h"

#define NAME "XDanmaku"
#define VERSION "2024.01.02"
#define loop while (true)
#define STDIN STDIN_FILENO

Display *dpy;
int scr;
XSizeHints *size_hints;
XWMHints *wm_hints;
XClassHint *class_hints;
XTextProperty window_name;
XFontStruct *font_info;
Window root;
XSetWindowAttributes attr;
int display_width;
int display_height;
/* char *font = "12x24"; */
char *font = "8x16";
char *prog;
bool eof_hit;
GC _gc;
Pixmap _pix;

int color = 0xffffff;
int delay = 2500;

void clean(void)
{
	XFreeGC(dpy, _gc);
	XFreePixmap(dpy, _pix);
	XCloseDisplay(dpy);
	exit(0);
}

void fail(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

void init(int argc, char **argv)
{
	if (!((prog = strrchr(argv[0], '/')) && ++prog))
		prog = argv[0];
	int event_base, error_base;
	if (!(size_hints = XAllocSizeHints()))
		fail("%s: failure allocating memory for XSizeHints\n", prog);
	if (!(wm_hints = XAllocWMHints()))
		fail("%s: failure allocating memory for XWMHints\n", prog);
	if (!(class_hints = XAllocClassHint()))
		fail("%s: failure allocating memory for XClassHint\n", prog);
	if (!(dpy = XOpenDisplay(NULL)))
		fail("%s: failed to connect to X server `%s'\n", prog, XDisplayName(NULL));
	if (!XShapeQueryExtension(dpy, &event_base, &error_base))
		fail("%s: X Shape extension unsupported by server `%s'\n", prog, XDisplayName(NULL));
	if (!(font_info = XLoadQueryFont(dpy, font)))
		fail("%s: failed to open font `%s'\n", prog, font);
	srand(time(NULL));
	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	attr.override_redirect = true;
	wm_hints->initial_state = NormalState;
	wm_hints->input = false;
	wm_hints->flags = StateHint;
	class_hints->res_class = prog;
	class_hints->res_name = NAME;
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);
	display_width = DisplayWidth(dpy, scr);
	display_height = DisplayHeight(dpy, scr);
	_pix = XCreatePixmap(dpy, root, 8, 8, 1);
	_gc = XCreateGC(dpy, _pix, 0, NULL);
	XSetBackground(dpy, _gc, 0x000000);
	XSetForeground(dpy, _gc, 0xffffff);
	XSetFont(dpy, _gc, font_info->fid);
	color = XWhitePixel(dpy, scr);
}

Bullet *pollfds(fd_set *fds)
{
	if (!eof_hit && FD_ISSET(STDIN, fds))
		return getbullet();
	return NULL;
}

void reselect(fd_set *fds)
{
	struct timeval tv = { .tv_sec = 0, .tv_usec = delay };
	FD_ZERO(fds);
	FD_SET(STDIN, fds);
	if (select(STDIN + 1, fds, NULL, NULL, &tv) == -1)
		fail("%s: select() error\n", prog);
}

void eoftick(List *list)
{
	if (!len(list) && eof_hit)
		clean();
	else if (eof_hit)
		usleep(delay);
}

int main(int argc, char **argv)
{
	init(argc, argv);
	List *list = mklist();
	fd_set fds;
	Bullet *b = NULL;

	loop {
		reselect(&fds);
		append(list, pollfds(&fds));
		eoftick(list);
		while ((b = iter(list)))
			if (canmove(b))
				destroy(list, b);
			else
				move(b);
		if (len(list))
			XFlush(dpy);
	}

	clean();
}
