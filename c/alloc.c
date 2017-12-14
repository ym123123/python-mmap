//============================================================================
// Name        : test.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include "common.h"

struct
{
	size_t totle;
	atomic_t lock;
	int align;
	int bits_len;
	uint8_t *bits;
	uint8_t *datas;
} *m_core;

#define BIT_IS_NO_ZERO(bits, i)\
	(bits[(i) / 8] & (0x1 << ((i) % 8)))
#define BIT_SET_ZERO(bits, i)\
	bits[(i) / 8] &= ~(0x1 << ((i) % 8))
#define BIT_SET_NO_ZERO(bits, i)\
	bits[(i) / 8] |= 0x1 << ((i) % 8)

static void m_des()
{
	if (m_core)
		munmap(m_core, m_core->totle);
}

ret_t m_init(size_t totle, int align)
{
	int plen;

	if (m_core)
		return RET_OK;

	plen = sizeof(*m_core) + totle / align / 8 + 1;
	plen = ((plen / align) + ((plen % align)?1:0)) * align;

	m_core = mmap(NULL, totle + plen,
		      PROT_READ|PROT_WRITE,
		      MAP_ANON|MAP_SHARED, -1, 0);

	if (m_core == NULL)
		return RET_ERR;

	m_core->totle = totle + plen;
	m_core->bits = (uint8_t *)m_core + sizeof(*m_core);
	m_core->bits_len = totle / align;

	m_core->align = align;
	m_core->datas = (uint8_t *)m_core + plen;
	m_core->lock = 0;

	atexit(m_des);
	return RET_OK;
}

void *m_alloc(size_t size)
{
	int i, j, len;
	void *ret;
	size += sizeof(long);

	len = size / m_core->align + ((size % m_core->align)?1:0);

	rw_wlock(&m_core->lock);

	for (i = 0; i < m_core->bits_len - len; i++)
	{
		for (j = 0; j < len; j++)
		{
			if (BIT_IS_NO_ZERO(m_core->bits, i + j))
				break;
		}

		if (j != len)
			i += j;
		else
			break;
	}

	if (i == m_core->bits_len - len)
	{
		ret = NULL;
	}
	else
	{
		for (j = 0;j < len; j++)
		{
			BIT_SET_NO_ZERO(m_core->bits, i + j);
		}

		ret = m_core->datas + i * m_core->align;
		*(long *)ret = len;
		ret = (uint8_t *)ret + sizeof(long);
	}

	rw_unlock(&m_core->lock);
	return ret;
}

void m_free(void *ret)
{
	uint8_t *p = ret;
	int len, i, j;
	rw_rlock(&m_core->lock);

	p = p - sizeof(long);
	len = *(long *)p;
	i = (size_t)(p - m_core->datas) / m_core->align;

	for (j = 0; j < len; j++)
	{
		BIT_SET_ZERO(m_core->bits, i + j);
	}

	rw_unlock(&m_core->lock);
}
