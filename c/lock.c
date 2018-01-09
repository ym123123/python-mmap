/*
 * lock.c
 *
 *  Created on: 2017Äê12ÔÂ11ÈÕ
 *      Author: ym
 */

#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#define atomic_cmp_set(lk, old, set)\
	__sync_bool_compare_and_swap(lk, old, set)

void rw_rlock(atomic_t *lk)
{
	int num = 100;
	unsigned long tmp;

	for (;;)
	{
		tmp = *lk;

		if (tmp != -1 && atomic_cmp_set(lk, tmp, tmp + 1))
			return;

		__asm__("pause");

		if (num-- == 0)
		{
			usleep(10);
			num = 100;
		}
	}
}
void rw_wlock(atomic_t *lk)
{
	unsigned long tmp;
	int num = 100;

	for (;;)
	{
		tmp = *lk;
		if (tmp == 0 && atomic_cmp_set(lk, 0, -1))
		{
			break;
		}
		__asm__("pause");

		if (num-- == 0)
		{
			usleep(10);
			num = 100;
		}
	}
}

void rw_unlock(atomic_t *lk)
{
	unsigned long tmp;
	int num = 100;
	if (*lk == -1)
	{
		atomic_cmp_set(lk, -1 , 0);
		return;
	}

	for (;;)
	{
		tmp = *lk;

		if (atomic_cmp_set(lk, tmp, tmp - 1))
			return;

		__asm__("pause");

		if (num-- == 0)
		{
			usleep(10);
			num = 100;
		}
	}
}

ret_t rw_trylock(atomic_t *lk)
{
	if (*lk == 0 && atomic_cmp_set(lk, 0, -1))
		return RET_OK;
	return RET_ERR;
}
