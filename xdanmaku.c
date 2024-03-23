#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/shape.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include "xdanmaku.h"

/* ------------------------------------------------------------------ */

struct xdanmaku_config config = {
	.font = "12x24",
	.echo = false,
	.fg_color = 0xffffff,
	.bg_color = -1,
	.stroke = 0,
	.delay = 2500,
	.speed_min = 0.60,
	.speed_max = 0.75,
	.bullet_max = 0,
	.line_max = 128,
	.padding_top = 0,
	.padding_bottom = 0,
};

struct xdanmaku_state state = {
	.mask_fg = { 0xffff, 0xffff, 0xffff, 0xffff },
};

/* ------------------------------------------------------------------ */

void xdanmaku_fail(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(1);
}

#define fail xdanmaku_fail
void xdanmaku_init(int argc, char **argv)
{
	if (!((state.prog = strrchr(argv[0], '/')) && ++state.prog))
		state.prog = argv[0];
	int event_base, error_base;
	if (!(state.size_hints = XAllocSizeHints()))
		fail("%s: failure allocating memory for XSizeHints\n", state.prog);
	if (!(state.dpy = XOpenDisplay(NULL)))
		fail("%s: failed to connect to X server `%s'\n", state.prog, XDisplayName(NULL));
	if (!XShapeQueryExtension(state.dpy, &event_base, &error_base))
		fail("%s: X Shape extension unsupported by server `%s'\n", state.prog, XDisplayName(NULL));
	srand(time(NULL));
	state.attr.override_redirect = true;
	state.scr = DefaultScreen(state.dpy);
	state.root = RootWindow(state.dpy, state.scr);
	state.scr_width = DisplayWidth(state.dpy, state.scr);
	state.scr_height = DisplayHeight(state.dpy, state.scr);
	state.scr_depth = DefaultDepth(state.dpy, state.scr);
	state.pix = XCreatePixmap(state.dpy, state.root, 8, 8, 1);
	state.gc = XCreateGC(state.dpy, state.pix, 0, NULL);
	state.cpix = XCreatePixmap(state.dpy, state.root, 8, 8, state.scr_depth);
	state.cgc = XCreateGC(state.dpy, state.cpix, 0, NULL);
	XSetBackground(state.dpy, state.gc, 0x000000);
	XSetForeground(state.dpy, state.gc, 0xffffff);
	config.fg_color = XWhitePixel(state.dpy, state.scr);
	xdanmaku_findfont(config.font);
}
#undef fail


void xdanmaku_findfont(char *font)
{
	if (!(state.font_info = XLoadQueryFont(state.dpy, font))) {
		state.xftfont = XftFontOpenName(state.dpy, state.scr, font);
		state.drawfn = 1;
		state.cmap = XDefaultColormap(state.dpy, state.scr);
		state.vis = XDefaultVisual(state.dpy, state.scr);
		XftColorAllocValue(state.dpy, state.vis, state.cmap, &state.mask_fg, &state.xftc);
	} else {
		XSetFont(state.dpy, state.gc, state.font_info->fid);
		XSetFont(state.dpy, state.cgc, state.font_info->fid);
	}
}

void xdanmaku_clean(void)
{
	XFreeGC(state.dpy, state.gc);
	XFreeGC(state.dpy, state.cgc);
	XFreePixmap(state.dpy, state.pix);
	XFreePixmap(state.dpy, state.cpix);
	if (state.font_info)
		XUnloadFont(state.dpy, state.font_info->fid);
	if (state.drawfn) {
		XftColorFree(state.dpy, state.vis, state.cmap, &state.xftc);
		XftFontClose(state.dpy, state.xftfont);
	}
	XCloseDisplay(state.dpy);
}

/* ------------------------------------------------------------------ */

float randf(float min, float max)
{
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

int randrow(int text_height)
{
	int r = config.padding_top + rand() % (state.scr_height + 1 - config.padding_top);
	if (r > state.scr_height - text_height - config.padding_bottom)
		return randrow(text_height);
	return r;
}

/* ------------------------------------------------------------------ */

void (*drawstr[2])(Bullet *new, char *line, int n) = { xdrawstr, xftdrawstr };

#define B(c)  ((((c) & 0x0000ff) << 8) | ((c) & 0x0000ff))
#define G(c) B((((c) & 0x00ff00) >> 8))
#define R(c) B((((c) & 0xff0000) >> 16))

void xftdrawstr(Bullet *new, char *line, int n)
{
	XGlyphInfo extents;
	XftTextExtentsUtf8(state.dpy, state.xftfont, (XftChar8 *)line, n, &extents);
	int text_height = extents.height;
	int text_width = extents.xOff;
	int y = randrow(text_height);
	if (config.stroke > 0) {
		text_height += config.stroke * 2;
		text_width += config.stroke * 2;
		y += config.stroke;
	}
	Window win = XCreateSimpleWindow(state.dpy, state.root, state.scr_width, y,
			text_width, text_height, 0, 0, config.fg_color);
	XSizeHints size;
	size.flags = PPosition | PSize;
	size.width = text_width;
	size.height = text_height;
	size.x = state.scr_width;
	size.y = y;
	XSetWMNormalHints(state.dpy, win, &size);
	XChangeWindowAttributes(state.dpy, win, CWOverrideRedirect, &state.attr);
    Pixmap p;
	XftDraw *draw;
	int off_x = config.stroke > 0 ? config.stroke : 0;
	int off_y = extents.y + off_x;
	if (config.bg_color >= 0) {
		p = XCreatePixmap(state.dpy, win, text_width, text_height, state.scr_depth);
		XSetForeground(state.dpy, state.cgc, config.bg_color);
		XFillRectangle(state.dpy, p, state.cgc, 0, 0, text_width, text_height);
		draw = XftDrawCreate(state.dpy, p, state.vis, state.cmap);
		XftColor xftc_fg;
		XRenderColor mask_fg = {
			.alpha = 0xffff,
			.blue = B(config.fg_color),
			.green = G(config.fg_color),
			.red = R(config.fg_color)
		};
		XftColorAllocValue(state.dpy, state.vis, state.cmap, &mask_fg, &xftc_fg);
		XftDrawStringUtf8(draw, &xftc_fg, state.xftfont, off_x, off_y, (XftChar8 *)line, n);
		XftColorFree(state.dpy, state.vis, state.cmap, &xftc_fg);
		if (config.stroke > 0) {
			Pixmap mask = XCreatePixmap(state.dpy, win, text_width, text_height, 1);
			XftDraw *draw_mask = XftDrawCreateBitmap(state.dpy, mask);
			XftDrawStringUtf8(draw_mask, &state.xftc, state.xftfont, off_x, off_y, (XftChar8 *)line, n);
			XShapeCombineMask(state.dpy, win, ShapeBounding, 0, 0, mask, ShapeSet);
			for (int i = 1; i <= config.stroke; ++i) {
				XShapeCombineMask(state.dpy, win, ShapeBounding, -i, 0, mask, ShapeUnion);
				XShapeCombineMask(state.dpy, win, ShapeBounding, i, 0, mask, ShapeUnion);
				XShapeCombineMask(state.dpy, win, ShapeBounding, 0, i, mask, ShapeUnion);
				XShapeCombineMask(state.dpy, win, ShapeBounding, 0, -i, mask, ShapeUnion);
			}
			XftDrawDestroy(draw_mask);
			XFreePixmap(state.dpy, mask);
		}
		XSetWindowBackgroundPixmap(state.dpy, win, p);
	} else {
		p = XCreatePixmap(state.dpy, win, text_width, text_height, 1);
		draw = XftDrawCreateBitmap(state.dpy, p);
		XftDrawStringUtf8(draw, &state.xftc, state.xftfont, 0, off_y, (XftChar8 *)line, n);
		XShapeCombineMask(state.dpy, win, ShapeBounding, 0, 0, p, ShapeSet);
	}
	XMapWindow(state.dpy, win);
	XftDrawDestroy(draw);
	XFreePixmap(state.dpy, p);
	new->id = win;
	new->width = text_width;
	new->x = state.scr_width;
	new->y = y;
}

void xdrawstr(Bullet *new, char *line, int n)
{
	int text_width = XTextWidth(state.font_info, line, n);
	int text_height = state.font_info->ascent + state.font_info->descent;
	int border_width = 0, border = 0;
	int y = randrow(text_height);
	if (config.stroke > 0) {
		text_height += config.stroke * 2;
		text_width += config.stroke * 2;
		y += config.stroke;
	}
	Window win = XCreateSimpleWindow(state.dpy, state.root, state.scr_width, y,
			text_width, text_height, border_width, border, config.fg_color);
	XSizeHints size;
	size.flags = PPosition | PSize;
	size.width = text_width;
	size.height = text_height;
	size.x = state.scr_width;
	size.y = y;
	XSetWMNormalHints(state.dpy, win, &size);
	XChangeWindowAttributes(state.dpy, win, CWOverrideRedirect, &state.attr);
	Pixmap p;
	if (config.bg_color >= 0) {
		p = XCreatePixmap(state.dpy, win, text_width, text_height, state.scr_depth);
		XSetForeground(state.dpy, state.cgc, config.bg_color);
		XFillRectangle(state.dpy, p, state.cgc, 0, 0, text_width, text_height);
		XSetForeground(state.dpy, state.cgc, config.fg_color);
		int offset = config.stroke > 0 ? config.stroke : 0;
		XDrawString(state.dpy, p, state.cgc, offset, state.font_info->ascent + offset, line, n);
		if (config.stroke > 0) {
			Pixmap mask = XCreatePixmap(state.dpy, win, text_width, text_height, 1);
			XSetForeground(state.dpy, state.gc, 0xffffff);
			XDrawString(state.dpy, mask, state.gc, offset, state.font_info->ascent + offset, line, n);
			XShapeCombineMask(state.dpy, win, ShapeBounding, 0, 0, mask, ShapeSet);
			for (int i = 1; i <= config.stroke; ++i) {
				XShapeCombineMask(state.dpy, win, ShapeBounding, -i, 0, mask, ShapeUnion);
				XShapeCombineMask(state.dpy, win, ShapeBounding, i, 0, mask, ShapeUnion);
				XShapeCombineMask(state.dpy, win, ShapeBounding, 0, i, mask, ShapeUnion);
				XShapeCombineMask(state.dpy, win, ShapeBounding, 0, -i, mask, ShapeUnion);
			}
			XFreePixmap(state.dpy, mask);
		}
		XSetWindowBackgroundPixmap(state.dpy, win, p);
	} else {
		p = XCreatePixmap(state.dpy, win, text_width, text_height, 1);
		XSetForeground(state.dpy, state.cgc, config.bg_color);
		XDrawString(state.dpy, p, state.gc, 0, state.font_info->ascent, line, n);
		XShapeCombineMask(state.dpy, win, ShapeBounding, 0, 0, p, ShapeSet);
	}
	XMapWindow(state.dpy, win);
	XFreePixmap(state.dpy, p);
	new->id = win;
	new->width = text_width;
	new->x = state.scr_width;
	new->y = y;
}

/* ------------------------------------------------------------------ */

static char *strtrim(char *line)
{
	char *end = NULL;
	while (isspace(*line)) ++line;
	if (*line == '\0')
		return NULL;
	end = line + strlen(line) - 1;
	while (end > line && isspace(*end)) --end;
	end[1] = '\0';
	return line;
}

Bullet *mkbullet(char *line)
{
	if (line == NULL)
		return NULL;
	Bullet *new = malloc(sizeof(Bullet));
	if (new == NULL)
		return NULL;
	new->next = NULL;
	new->prev = NULL;
	new->id = 0;
	new->width = 0;
	new->x = 0;
	new->y = 0;
	new->speed = 1.0;
	char *trimmed = strtrim(line);
	if (trimmed == NULL) {
		free(new);
		return NULL;
	}
	if (config.echo)
		printf("%s\n", trimmed);
	int slen = strlen(trimmed);
	drawstr[state.drawfn](new, trimmed, slen);
	new->speed = randf(config.speed_min, config.speed_max);
	return new;
}

void bullet_destroy(List *list, Bullet *b)
{
	XDestroyWindow(state.dpy, b->id);
	if (list)
		list_erase(list, b);
	free(b);
}

void bullet_tick(Bullet *b)
{
	b->x -= b->speed;
	XMoveWindow(state.dpy, b->id, b->x, b->y);
}

bool bullet_passed(Bullet *b)
{
	return b->x - b->speed + b->width <= 0;
}

/* ------------------------------------------------------------------ */

List *mklist(void)
{
	List *new = malloc(sizeof(List));
	if (new) {
		new->head = NULL;
		new->tail = NULL;
		new->iter = NULL;
		new->len = 0;
		new->ret_iter = false;
	}
	return new;
}

List *list_append(List *list, Bullet *bullet)
{
	if (bullet == NULL)
		return NULL;
	if (list && list_full(list))
		return NULL;
	if (list == NULL && (list = mklist()) == NULL)
		return NULL;
	if (list->head == NULL)
		list->head = bullet;
	else if (list->tail == NULL) {
		if (list->len != 1)
			return NULL;
		bullet->prev = list->head;
		list->tail = bullet;
		list->head->next = list->tail;
	}
	else {
		list->tail->next = bullet;
		bullet->prev = list->tail;
		list->tail = bullet;
	}
	bullet->next = NULL;
	++list->len;
	return list;
}

Bullet *list_iter(List *list)
{
	if (list == NULL)
		return NULL;
	if (list->iter == NULL)
		list->iter = list->head;
	else if (list->ret_iter)
		list->ret_iter = false;
	else
		list->iter = list->iter->next;
	return list->iter;
}

Bullet *list_erase(List *list, Bullet *bullet)
{
	if (list == NULL || bullet == NULL)
		return NULL;
	--list->len;
	if (bullet->prev)
		bullet->prev->next = bullet->next;
	if (bullet->next)
		bullet->next->prev = bullet->prev;
	if (bullet == list->tail)
		list->tail = bullet->prev;
	if (bullet == list->head)
		list->head = bullet->next;
	if (list->tail == list->head)
		list->tail = NULL;
	if (list->iter && bullet == list->iter) {
		list->ret_iter = true;
		list->iter = list->iter->next;
	}
	return bullet;
}

int list_len(List *list)
{
	if (list == NULL)
		return -1;
	return list->len;
}

bool list_full(List *list)
{
	if (list == NULL)
		return false;
	if (config.bullet_max <= 0)
		return false;
	return list_len(list) >= config.bullet_max;
}
