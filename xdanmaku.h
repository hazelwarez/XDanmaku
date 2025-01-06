#pragma once

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/shape.h>

typedef struct bullet Bullet;
typedef struct list List;

extern struct xdanmaku_config {
	char *font;
	bool echo;
	int fg_color;
	int bg_color;
	int stroke;
	int delay;
	float speed_min;
	float speed_max;
	int bullet_max;
	int bullet_max_per_second;
	int line_max;
	int padding_top;
	int padding_bottom;
} config;

extern struct xdanmaku_state {
	Display *dpy;
	int scr;
	XSizeHints *size_hints;
	XFontStruct *font_info;
	Window root;
	XSetWindowAttributes attr;
	int scr_width;
	int scr_height;
	int scr_depth;
	XftFont *xftfont;
	char *prog;
	GC gc;
	Pixmap pix;
	GC cgc;
	Pixmap cpix;
	XftColor xftc;
	Colormap cmap;
	Visual *vis;
	int drawfn;
	XRenderColor mask_fg;
	time_t this_second;
	int bullets_created_this_second;
} state;

struct list {
	Bullet *head;
	Bullet *tail;
	Bullet *iter;
	bool ret_iter;
	int len;
};

struct bullet {
	Window id;
	float x, speed;
	int y, width, len;
	Bullet *next;
	Bullet *prev;
};

void xdanmaku_init(int argc, char **argv);
void xdanmaku_fail(char *fmt, ...);
void xdanmaku_findfont(char *font);
void xdanmaku_clean(void);

void xftdrawstr(Bullet *b, char *line, int n);
void xdrawstr(Bullet *b, char *line, int n);

Bullet *mkbullet(char *line);
void bullet_destroy(List *list, Bullet *b);
void bullet_tick(Bullet *b);
bool bullet_passed(Bullet *b);

List *mklist(void);
List *list_append(List *list, Bullet *bullet);
Bullet *list_iter(List *list);
Bullet *list_erase(List *list, Bullet *bullet);
int list_len(List *list);
bool list_full(List *list);
