/*
 * i_fast_map.h
 *
 *  Created on: 21 juil. 2017
 *      Author: f.baccari
 */

#ifndef SOURCE_I_FAST_MAP_H_
#define SOURCE_I_FAST_MAP_H_

#include <i_tools.h>
#include <i_thread.h>

#define fast_map_default_max_nb 2

typedef struct s_fast_map{
	pthread_mutex_t*	lock;
	long unsigned int	key_size;
	long unsigned int	data_size;
	void* 				first;
	void*				last;
	long unsigned int	max_number;
	void*				key_comp;
} s_fast_map, *p_s_fast_map;

#define fast_map_count(mp) ((mp->last - mp->first + (mp->key_size + mp->data_size)) / (mp->key_size + mp->data_size))

extern p_s_fast_map	fast_map_init(long unsigned int key_size, long unsigned int data_size, cmp_func_type* comp_key_func);

extern void		fast_map_clear(p_s_fast_map map);
extern void* 	fast_map_at(p_s_fast_map map, void* key);
extern void*	fast_map_insert(p_s_fast_map map, void* key, void* data);
extern int 		fast_map_remove(p_s_fast_map map, void* key);
extern void*	fast_map_extract(p_s_fast_map map, void* key);

#define fast_map_iterate(key, data, map) for(void *luulhhhh_do_not_use_this = map->first; (((void*)(key = luulhhhh_do_not_use_this) <= (void*)map->last) && (data = (void*)(luulhhhh_do_not_use_this + map->key_size))); luulhhhh_do_not_use_this += (map->key_size + map->data_size))
#define fast_map_data_iterate(ptr, map) for(void* luulhhhh_do_not_use_this = map->first + map->key_size; (void*)(ptr = luulhhhh_do_not_use_this) <= (void*)(map->last) + map->key_size; luulhhhh_do_not_use_this += (map->key_size + map->data_size))

extern void		fast_map_free(p_s_fast_map map);


#endif /* SOURCE_I_FAST_MAP_H_ */
