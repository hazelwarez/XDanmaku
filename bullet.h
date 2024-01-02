#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

typedef struct bullet Bullet;

#include "list.h"

extern GC _gc;
extern Display *dpy;
extern int scr;
extern XSizeHints *size_hints;
extern XWMHints *wm_hints;
extern XClassHint *class_hints;
extern XTextProperty window_name;
extern XFontStruct *font_info;
extern Window root;
extern XSetWindowAttributes attr;
extern int display_width, display_height;
extern char *font;
extern char *prog;
extern bool eof_hit;

extern int color;

struct bullet {
	Window id;
	float x, speed;
	int y, width;
	char *line;
	Bullet *next;
	Bullet *prev;
};

Bullet *mkbullet(char *line);
Bullet *getbullet(void);
void destroy(List *list, Bullet *b);
void move(Bullet *b);
bool canmove(Bullet *b);
