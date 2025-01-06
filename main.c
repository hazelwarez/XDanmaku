#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "xdanmaku.h"

#define NAME "XDanmaku"
#define VERSION "2025.01.06"
#define loop while (true)
#define STDIN STDIN_FILENO
#define fail xdanmaku_fail

void help(void)
{
	printf("usage: %s [OPTIONS]\n", state.prog);
	printf("  -fn  font to use for all text\n");
	printf("  -pt  padding above the highest bullet (default: 0)\n");
	printf("  -pb  padding below the lowest bullet (default: 0)\n");
	printf("  -fg  text foreground color (default: #%x)\n", config.fg_color);
	printf("  -bg  text background color (default: nil)\n");
	printf("  -st  text stroke, in pixels (default: nil)\n");
	printf("  -sn  minimum speed, in subpixels, per tick (default: %.2f)\n", config.speed_min);
	printf("  -sx  maximum speed, in subpixels, per tick (default: %.2f)\n", config.speed_max);
	printf("  -lx  maximum line length, in bytes, per bullet (default: %d)\n", config.line_max);
	printf("  -bx  maximum number of bullets at a time (default: nil)\n");
	printf("  -bsx maximum number of bullets to be created per second (default: nil)\n");
	printf("  -e   echo each new input line as it will be drawn (default: false)\n");
	printf("  -d   delay between stdin polling, in microseconds (default: %d)\n", config.delay);
	printf("  -h   print this help message and exit\n");
	printf("  -v   print version and exit\n");
	exit(0);
}

void version(void)
{
	printf("%s %s\n", NAME, VERSION);
	exit(0);
}

#define ifopt(s) if (!strcmp(argv[i], (s)))
#define elifopt(s) else ifopt(s)
#define getargint(s, v) if (!argv[++i]) \
	fail("%s: %s not provided an argument\n", state.prog, (s)); \
	else \
	v = strtol(argv[i], NULL, 0);
#define getargfloat(s, v) if (!argv[++i]) \
	fail("%s: %s not provided an argument\n", state.prog, (s)); \
	else \
	v = strtof(argv[i], NULL);
#define getargcolor(s, v) if (!argv[++i]) \
	fail("%s: %s not provided an argument\n", state.prog, (s)); \
	else if (*argv[i] == '#') \
		v = strtol(++argv[i], NULL, 16); \
	else \
		v = strtol(argv[i], NULL, 16);
#define getargstr(s, v) if (!argv[++i]) \
	fail("%s: %s not provided an argument\n", state.prog, (s)); \
	else \
	v = argv[i];

void parseargs(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i) {
		ifopt("-v")
			version();
		elifopt("-h")
			help();
		elifopt("-e")
			config.echo = true;
		elifopt("-pt") {
			getargint("-pt", config.padding_top);
		}
		elifopt("-pb") {
			getargint("-pb", config.padding_bottom);
		}
		elifopt("-d") {
			getargint("-d", config.delay);
		}
		elifopt("-sn") {
			getargfloat("-sn", config.speed_min);
		}
		elifopt("-sx") {
			getargfloat("-sx", config.speed_max);
		}
		elifopt("-lx") {
			getargint("-lx", config.line_max);
		}
		elifopt("-bx") {
			getargint("-bx", config.bullet_max);
		}
		elifopt("-bsx") {
			getargint("-bsx", config.bullet_max_per_second);
		}
		elifopt("-fg") {
			getargcolor("-fg", config.fg_color);
		}
		elifopt("-bg") {
			getargcolor("-bg", config.bg_color);
		}
		elifopt("-fn") {
			getargstr("-fn", config.font);
			xdanmaku_findfont(config.font);
		}
		elifopt("-st") {
			getargint("-st", config.stroke);
		}
	}

	if (config.delay < 0)
		fail("%s: -d delay must be >= 0 microseconds; %d given\n", state.prog, config.delay);
	if (config.speed_min <= 0.0)
		fail("%s: -sn minimum speed must be >= 0 subpixels; %.2f given\n", state.prog, config.speed_min);
	if (config.speed_max <= 0.0)
		fail("%s: -sx maximum speed must be >= 0 subpixels; %.2f given\n", state.prog, config.speed_max);
	if (config.speed_max < config.speed_min)
		fail("%s: -sx maximum speed (%.2f) must be >= minimum speed (%.2f)\n",
				state.prog, config.speed_max, config.speed_min);
}

char *getbulletstr(void)
{
	char *line = NULL;
	ssize_t nread = 0;
	size_t llen = 0;
	nread = getline(&line, &llen, stdin);
	if (nread == EOF)
		return NULL;
	if (nread-- == 1)
		return NULL;
	if (nread >= config.line_max)
		line[config.line_max-1] = '\0';
	else
		line[nread] = '\0';
	return line;
}

char *pollfds(fd_set *fds)
{
	if (!feof(stdin) && FD_ISSET(STDIN, fds))
		return getbulletstr();
	return NULL;
}

void reselect(fd_set *fds)
{
	struct timeval tv = { .tv_sec = 0, .tv_usec = config.delay };
	FD_ZERO(fds);
	FD_SET(STDIN, fds);
	if (select(STDIN + 1, fds, NULL, NULL, &tv) == -1)
		fail("%s: select() error\n", state.prog);
}

int main(int argc, char **argv)
{
	xdanmaku_init(argc, argv);
	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	parseargs(argc, argv);
	List *list = mklist();
	fd_set fds;
	Bullet *b = NULL;

	loop {
		reselect(&fds);
		if (!feof(stdin)) {
			char *bstr = pollfds(&fds);
			if (bstr && !list_full(list)) {
				Bullet *new = mkbullet(bstr);
				List *ok = list_append(list, new);
				if (new && !ok)
					free(new);
			}
			if (bstr)
				free(bstr);
		}
		else if (feof(stdin) && !list_len(list))
			break;
		else
			usleep(config.delay);

		int llen = list_len(list);
		while ((b = list_iter(list)))
			if (bullet_passed(b))
				bullet_destroy(list, b);
			else
				bullet_tick(b);

		if (llen)
			XFlush(state.dpy);
	}

	xdanmaku_clean();
}
