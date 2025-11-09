/*
 * i_sorted_set.h
 *
 *  Created on: 20 juil. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_I_SORTED_SET_H_
#define INCLUDE_I_SORTED_SET_H_

#include <i_thread.h>

extern clock_t diff;

#define fast_default_max_nb 2

typedef struct s_sorted_set{
	pthread_mutex_t*	lock;
	void* 				first;
	void* 				last;
	long unsigned int	obj_size;
	long unsigned int	max_number;
	void*				cmp_func;
}s_sorted_set, *p_s_sorted_set;


#define sorted_set_count(set) (((set->last - set->first) + set->obj_size) / set->obj_size)

#define sorted_set_find(set, key) bsearch_custom(key, set->first, set->last, set->obj_size, set->cmp_func)

extern p_s_sorted_set	sorted_set_init(long unsigned int obj_size, void* cmp_func);

extern void*			sorted_set_insert(p_s_sorted_set set, void* element);
extern void*			sorted_set_at(p_s_sorted_set set, int i);
extern int				sorted_set_remove(p_s_sorted_set set, void* element);
extern void 			sorted_set_clear(p_s_sorted_set set);
extern int				sorted_set_intersect(p_s_sorted_set set1, p_s_sorted_set set2);
extern int				sorted_set_union(p_s_sorted_set set1, p_s_sorted_set set2);
extern int				sorted_set_extract(p_s_sorted_set set1, p_s_sorted_set set2);
extern p_s_sorted_set	sorted_set_clone(p_s_sorted_set set);

#define sorted_set_iterate(ptr, set) for(void* luulhhhh_do_not_use_this = set->first; (void*)(ptr = luulhhhh_do_not_use_this) <= (void*)set->last; luulhhhh_do_not_use_this += set->obj_size)

extern void 			sorted_set_free(p_s_sorted_set set);

#endif /* INCLUDE_I_SORTED_SET_H_ */
