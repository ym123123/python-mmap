/*
 * common.h
 *
 *  Created on: 2017Äê12ÔÂ11ÈÕ
 *      Author: ym
 */

#ifndef COMMON_H_
#define COMMON_H_

typedef enum {RET_ERR = -1, RET_OK, RET_AGAIN } ret_t;
typedef volatile unsigned long atomic_t;

#ifdef __cplusplus
extern "C" {
#endif

ret_t m_init(size_t totle, int align);
void *m_alloc(size_t size);
void m_free(void *);

void rw_rlock(atomic_t *lk);
void rw_wlock(atomic_t *lk);
void rw_unlock(atomic_t *lk);
ret_t rw_trylock(atomic_t *lk);

ret_t frame_init_pool(size_t totle, int align, int fs_len);
void * create_frame(int ip, short port);
ret_t destroy_frame(void *fs);
ret_t frame_push(void * p, const char *bytes, int len, int index);
void * get_frame_header(int ip, short port);
void put_frame_header(void * frame_header);
void *get_frame_data(void *fs, void *cursor);
void *init_frame_cursor();

char *frame_data(void *data, int *len);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H_ */
