#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

#define init_queue(queue)\
  queue.prev = queue.next = &queue

typedef struct QUEUE
{
  struct QUEUE *prev;
  struct QUEUE *next;
} queue_t;

typedef struct
{
  queue_t queue;
  atomic_t use;
  int len;
  char data[0];
} frame_t;

typedef struct
{
  queue_t frame_queue;
  atomic_t info_rwlock;
  unsigned int is_user:1;
  unsigned int is_des:1;
  unsigned int user_count:16;
  int ip;
  short int port;
} frame_info_t;

typedef struct
{
  int fs_len;
  char frames[0];
} fs_t;

fs_t *fs;

ret_t frame_init_pool(size_t totle, int align, int fs_len)
{
  frame_info_t *frames;

  if (fs != NULL)
    return RET_OK;

  m_init(totle, align);

  fs = m_alloc(fs_len * sizeof(frame_info_t) + sizeof(fs_t));

  if (fs == NULL)
    return RET_ERR;

  memset(fs, 0, fs_len * sizeof(frame_info_t) + sizeof(fs_t));
  fs->fs_len = fs_len;

  return RET_OK;
}

void * create_frame(int ip, short port)
{
  int i;
  void *ret = 0;
  frame_info_t *frames;
  frames = (frame_info_t *)fs->frames;

  for (i = 0; i < fs->fs_len && ret == NULL; i++)
  {
    if (rw_trylock(&frames[i].info_rwlock) == RET_ERR)
    {
      continue;
    }

    if (!frames[i].is_user)
    {
      frames[i].is_user = 1;
      frames[i].is_des = 0;
      frames[i].user_count = 0;
      frames[i].ip = ip;
      frames[i].port = port;
      ret = &frames[i];
      init_queue(frames[i].frame_queue);
    }

    rw_unlock(&frames[i].info_rwlock);
  }

  return (void *)ret;
}

ret_t destroy_frame(void *fs)
{
  frame_t *frame;
  queue_t *q;
  ret_t ret;
  frame_info_t *frames = fs;

  frames->is_des = 1;
  rw_wlock(&frames->info_rwlock);
  if (frames->user_count == 0)
  {
    frames->is_user = 0;

    for (q = frames->frame_queue.next; q != &frames->frame_queue; )
    {
    	frame = (frame_t *)q;
    	q = q->next;
    	m_free(frame);
    }
    ret = RET_OK;
  }
  else
  {
	  ret = RET_AGAIN;
  }
  rw_unlock(&frames->info_rwlock);

  return ret;
}

static inline void frame_force_del()
{
  int i;
  queue_t *q;
  frame_t *frame;
  frame_info_t *frames;
  for (i = 0; i < fs->fs_len; i++)
  {
    frames = ((frame_info_t *)fs->frames) + i;

    if (frames->is_user == 0)
      continue;

    if (rw_trylock(&frames->info_rwlock) == RET_ERR)
    {
    	if (rw_trylock(&frames->info_rwlock) == RET_ERR)
    		continue;
    }

    //最多删除前两个， 防止删除数据过多
    for (q = frames->frame_queue.next; q != &frames->frame_queue;)
    {
      frame = (frame_t *)q;
      q = frame->queue.next;

      if (frame->use)
      {
    	  continue;
      }

      frame->queue.prev->next = frame->queue.next;
      frame->queue.next->prev = frame->queue.prev;

      m_free(frame);
    }

    rw_unlock(&frames->info_rwlock);
  }
}

ret_t frame_push(void * p, const char *bytes, int len, int index)
{
  frame_info_t *frames = (frame_info_t *)( p);
  frame_t *frame = m_alloc(sizeof(frame_t) + len);

  if (frame == NULL)
  {
    frame_force_del();
    frame = m_alloc(sizeof(frame_t) + len);

    if (frame == NULL)
      return RET_AGAIN;
  }

  memcpy(frame->data, bytes, len);
  init_queue(frame->queue);
  frame->len = len;
  frame->use = 0;
  rw_wlock(&frames->info_rwlock);
  frame->queue.next = &frames->frame_queue;
  frame->queue.prev = frames->frame_queue.prev;
  frame->queue.next->prev = &frame->queue;
  frame->queue.prev->next = &frame->queue;

  if (frames->is_des == 1)
  {
    rw_unlock(&frames->info_rwlock);
    return RET_ERR;
  }

  rw_unlock(&frames->info_rwlock);

  return RET_OK;
}

void * get_frame_header(int ip, short port)
{
  int i;
  frame_info_t *frames;
  frames = (frame_info_t *)fs->frames;

  for (i = 0; i < fs->fs_len; i++)
  {
    rw_rlock(&frames[i].info_rwlock);

    if (frames[i].is_user && frames[i].ip == ip && frames[i].port == port)
    {
      frames[i].user_count++;
      rw_unlock(&frames[i].info_rwlock);
      return (void *)&frames[i];
    }

    rw_unlock(&frames[i].info_rwlock);
  }

  return NULL;
}

void put_frame_header(void * frame_header)
{
  frame_info_t *info =  frame_header;

  rw_wlock(&info->info_rwlock);
  info->user_count--;

  if (info->user_count == 0)
  {
    info->is_des = 1;
  }

  rw_unlock(&info->info_rwlock);
}

void *init_frame_cursor()
{
	return NULL;
}

void *get_frame_data(void *fs, void *no)
{
	frame_info_t *frames = fs;
	frame_t *frame = no;
	queue_t *q;
	rw_rlock(&frames->info_rwlock);

	do
	{
		if (frames->is_des || frames->frame_queue.next == &frames->frame_queue)
		{
			if (frame != NULL)
			{
				rw_unlock(&frame->use);
				frame = NULL;
			}

			break;
		}
		if (frame == NULL)
		{
			frame = (frame_t *)frames->frame_queue.next;
			rw_rlock(&frame->use);
			break;
		}
		else
		{
			if (frame->queue.next == &frames->frame_queue)
			{
				frame = NULL + RET_AGAIN;
			}
			else
			{
				rw_unlock(&frame->use);
				frame = (frame_t *)frame->queue.next;
				rw_rlock(&frame->use);
			}

			break;
		}


	} while (0);

	rw_unlock(&frames->info_rwlock);

	return frame;
}

char *frame_data(void *data, int *len)
{
	frame_t *frame = data;

	if (data - NULL == RET_AGAIN)
		return NULL;

	*len = frame->len;
	return frame->data;
}

