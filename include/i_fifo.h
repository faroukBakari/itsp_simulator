/*
 * i_fifo.h
 *
 *  Created on: 11 avr. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_I_FIFO_H_
#define INCLUDE_I_FIFO_H_
#include <pthread.h>

#define CRITICAL_FIFO_THRESHOLD 3

typedef struct fifo_slot{
	void*				object;
	struct fifo_slot*	next;
}fifo_slot, *p_fifo_slot;

typedef struct s_fifo{
	pthread_mutex_t*		lock;
	volatile int  			number;
	volatile p_fifo_slot	begin;
	volatile p_fifo_slot	end;
}s_fifo, *p_s_fifo;

extern p_s_fifo	fifo_init();

extern void*	fifo_pull(p_s_fifo fifo);

extern void 	fifo_push(p_s_fifo fifo, void* data);

extern void 	fifo_push_prior(p_s_fifo fifo, void* data);

extern int 		fifo_count(p_s_fifo fifo);

extern void		fifo_clear(p_s_fifo fifo);

extern void		fifo_free(p_s_fifo fifo);

extern void*	fifo_show_first(p_s_fifo fifo);

#endif /* INCLUDE_I_FIFO_H_ */
