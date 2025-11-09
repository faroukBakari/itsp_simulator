/*
 * i_fast_map.c
 *
 *  Created on: 21 juil. 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <i_tools.h>
#include <i_string.h>
#include <i_fast_map.h>

#define map_node_size(mp) (long unsigned int)(mp->key_size + mp->data_size)

#define find_next(map, element) bsearch_next(element, map->first, map->last, map_node_size(map), map->key_comp)

p_s_fast_map fast_map_init(long unsigned int key_size, long unsigned int data_size, cmp_func_type* comp_key_func){
	p_s_fast_map mp = malloc(sizeof(s_fast_map));
	memset(mp,0,sizeof(s_fast_map));
	mp->key_size = key_size;
	mp->data_size = data_size;
	mp->key_comp = comp_key_func;
	mp->lock = mutex_init();
	mp->first = malloc(fast_map_default_max_nb * map_node_size(mp));
	mp->last = mp->first - map_node_size(mp);
	mp->max_number = fast_map_default_max_nb;
	memset(mp->first, 0, fast_map_default_max_nb * map_node_size(mp));
	return mp;
}

void* fast_map_insert(p_s_fast_map map, void* key, void* data){
	void* out = NULL;
	pthread_mutex_lock(map->lock);

	long int nb = fast_map_count(map);
	if(nb == map->max_number){
		void* buffer = malloc(2*map->max_number*map_node_size(map));
		memcpy(buffer, map->first, nb*map_node_size(map));
		free(map->first);
		map->first = buffer;
		map->last = map->first + (nb - 1) * map_node_size(map);
		map->max_number *= 2;
	}

	void *next = find_next(map, key);
	if(next){
		if(((cmp_func_type*)map->key_comp)(next, key)){
			map->last += map_node_size(map);
			memmove(next + map_node_size(map), next, map->last - (void*)next);
			memcpy(next, key, map->key_size);
		}
		memcpy(next + map->key_size, data, map->data_size);
		out = next + map->key_size;
	}
	else{
		map->last += map_node_size(map);
		memcpy(map->last, key, map->key_size);
		memcpy(map->last + map->key_size, data, map->data_size);
		out = map->last + map->key_size;
	}

	pthread_mutex_unlock(map->lock);
	return out;
}

int fast_map_remove(p_s_fast_map map, void* key){
	int out = FAULT;
	pthread_mutex_lock(map->lock);
	void *found = bsearch_custom(key, map->first, map->last, map_node_size(map), map->key_comp);
	if(found){
		memmove(found, found + map_node_size(map), map->last - found);
		map->last -= map_node_size(map);
		out = OK;

		size_t nb = fast_map_count(map);
		if(nb < map->max_number / 2){
			void* buffer = malloc(map->max_number * map_node_size(map) / 2);
			memcpy(buffer, map->first, nb * map_node_size(map));
			free(map->first);
			map->first = buffer;
			map->last = map->first + (nb - 1) * map_node_size(map);
			map->max_number /= 2;
		}
	}
	pthread_mutex_unlock(map->lock);
	return out;
}

void* fast_map_at(p_s_fast_map map, void* key){

	void* out = NULL;
	pthread_mutex_lock(map->lock);
	if(map->first <= map->last){
		out = bsearch_custom(key, map->first, map->last, map_node_size(map), map->key_comp);
		if(out)
			out += map->key_size;
	}
	pthread_mutex_unlock(map->lock);
	return out;
}

void* fast_map_extract(p_s_fast_map map, void* key){

	void* out = NULL, * extracted = NULL;
	pthread_mutex_lock(map->lock);
	if(map->first <= map->last){
		out = bsearch_custom(key, map->first, map->last, map_node_size(map), map->key_comp);
		if(out){
			extracted = malloc(map->data_size);
			memcpy(extracted, out + map->key_size, map->data_size);

			memmove(out, out + map_node_size(map), map->last - out);
			map->last -= map_node_size(map);

			size_t nb = fast_map_count(map);
			if(nb < map->max_number / 2){
				void* buffer = malloc(map->max_number * map_node_size(map) / 2);
				memcpy(buffer, map->first, nb * map_node_size(map));
				free(map->first);
				map->first = buffer;
				map->last = map->first + (nb - 1) * map_node_size(map);
				map->max_number /= 2;
			}
		}
	}
	pthread_mutex_unlock(map->lock);
	return extracted;
}

void fast_map_clear(p_s_fast_map map){
	pthread_mutex_lock(map->lock);
	free(map->first);
	map->first = malloc(fast_map_default_max_nb * map_node_size(map));
	map->last = map->first - map_node_size(map);
	map->max_number = fast_map_default_max_nb;
	memset(map->first, 0, fast_map_default_max_nb * map_node_size(map));
	pthread_mutex_unlock(map->lock);
}

void fast_map_free(p_s_fast_map map){
	pthread_mutex_lock(map->lock);
	free(map->first);
	free(map);
}
















