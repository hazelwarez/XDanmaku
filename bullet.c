#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "bullet.h"
#include "list.h"

float randf(float min, float max)
{
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

int randrow(int text_height)
{
	int r = rand() % display_height + 1;
	if (r > display_height - text_height)
		return randrow(text_height);
	return r;
}

Bullet *mkbullet(char *line)
{
	if (line == NULL)
		return NULL;
	Bullet *new = malloc(sizeof(Bullet));
	if (new) {
		new->line = line;
		new->next = NULL;
		new->prev = NULL;
		new->id = 0;
		new->width = 0;
		new->x = 0;
		new->y = 0;
		new->speed = 1.0;
	}
	else {
		return NULL;
	}
	Window win;
	int slen = strlen(line);
	int text_width = XTextWidth(font_info, line, slen);
	int text_height = font_info->ascent + font_info->descent;
	int border_width = 0, border = 0;
	int y = randrow(text_height);
	win = XCreateSimpleWindow(dpy, root, display_width, y,
			text_width+10, text_height+10, border_width, border, color);
	XSizeHints size;
	size.flags = PPosition | PSize;
	size.width = text_width + 10;
	size.height = text_height + 10;
	size.x = display_width;
	size.y = y;
	XSetWMNormalHints(dpy, win, &size);
	XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &attr);
	Pixmap p = XCreatePixmap(dpy, win, text_width + 10, text_height + 10, 1);
	XDrawString(dpy, p, _gc, 0, font_info->ascent, line, slen);
	XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, p, ShapeSet);
	XMapWindow(dpy, win);
	XFreePixmap(dpy, p);
	new->id = win;
	new->width = text_width;
	new->x = display_width;
	new->y = y;
	new->speed = randf(0.60, 0.75);
	return new;
}

void destroy(List *list, Bullet *b)
{
	XDestroyWindow(dpy, b->id);
	erase(list, b);
	free(b);
}

void move(Bullet *b)
{
	b->x -= b->speed;
	XMoveWindow(dpy, b->id, b->x, b->y);
}

bool canmove(Bullet *b)
{
	return b->x - b->speed + b->width <= 0;
}

char *trim(char *line)
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

Bullet *getbullet(void)
{
	if (eof_hit)
		return NULL;
	char *line = NULL;
	ssize_t nread = 0;
	size_t llen = 0;
	nread = getline(&line, &llen, stdin);
	if (nread == EOF) {
		eof_hit = true;
		return NULL;
	}
	if (nread-- == 1)
		return NULL;
	line[nread] = '\0';
	char *trimmed = trim(line);
	if (trimmed == NULL) {
		free(line);
		return NULL;
	}
	Bullet *new = mkbullet(line);
	return new;
}
