#include <stdlib.h>
#include "list.h"
#include "bullet.h"

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

List *append(List *list, Bullet *bullet)
{
	if (bullet == NULL)
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

Bullet *iter(List *list)
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

Bullet *erase(List *list, Bullet *bullet)
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

int len(List *list)
{
	if (list == NULL)
		return -1;
	return list->len;
}
