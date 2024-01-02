#pragma once

typedef struct list List;

#include <stdbool.h>
#include "bullet.h"

struct list {
	Bullet *head;
	Bullet *tail;
	Bullet *iter;
	bool ret_iter;
	int len;
};

List *mklist(void);
List *append(List *list, Bullet *bullet);
Bullet *iter(List *list);
Bullet *erase(List *list, Bullet *bullet);
int len(List *list);
