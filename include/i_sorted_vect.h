/*
 * i_sorted_vect.h
 *
 *  Created on: 22 juin 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_I_SORTED_VECT_H_
#define INCLUDE_I_SORTED_VECT_H_

#include <i_thread.h>

#define fast_default_max_nb 2

typedef struct s_sorted_vect{
	pthread_mutex_t*	lock;
	void* 				first;
	void* 				last;
	long int			obj_size;
	long int 			max_number;
	void*				cmp_func;
}s_sorted_vect, *p_s_sorted_vect;


#define sorted_vect_count(v) (((v->last - v->first) / v->obj_size) + 1)

#define sorted_vect_find(v, key) bsearch_custom(key, v->first, v->last, v->obj_size, v->cmp_func)


extern p_s_sorted_vect	sorted_vect_init(long int obj_size, void* cmp_func);

extern void*			sorted_vect_insert(p_s_sorted_vect v, void* element);

extern int				sorted_vect_remove(p_s_sorted_vect v, void* element);

extern void* 			sorted_vect_at(p_s_sorted_vect v, int i);

extern void 			sorted_vect_clear(p_s_sorted_vect v);

#define sorted_vect_iterate(ptr, map) for(void* luulhhhh_do_not_use_this = map->first; (void*)(ptr = luulhhhh_do_not_use_this) <= (void*)map->last; luulhhhh_do_not_use_this += map->obj_size)

#endif /* INCLUDE_I_SORTED_VECT_H_ */
