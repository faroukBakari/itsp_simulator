/*
 * i_sorted_vect.c
 *
 *  Created on: 22 juin 2017
 *      Author: f.baccari
 */

#include <i_sorted_vect.h>
#include <stdio.h>
#include <stdlib.h>
#include <i_tools.h>
#include <i_string.h>

#define sorted_vect_size(v) (v->last - v->first + v->obj_size)

#define find_next(v, element) bsearch_next(element, v->first, v->last, v->obj_size, v->cmp_func)

p_s_sorted_vect	sorted_vect_init(long int obj_size, void* cmp_func){
	p_s_sorted_vect v = malloc(sizeof(s_sorted_vect));

	v->lock 		= mutex_init();
	v->first 		= malloc(fast_default_max_nb * obj_size);
	v->last 		= v->first - obj_size;
	v->obj_size 	= obj_size;
	v->max_number 	= fast_default_max_nb;
	v->cmp_func		= cmp_func;

	memset(v->first, 0, fast_default_max_nb * obj_size);
	return v;
}

void* sorted_vect_at(p_s_sorted_vect v, int i){
	pthread_mutex_lock(v->lock);
	void* out = (v->first + v->obj_size * i <= v->last) ? v->first + v->obj_size * i : NULL;
	pthread_mutex_unlock(v->lock);
	return out;

}

void* sorted_vect_insert(p_s_sorted_vect v, void* element){

	pthread_mutex_lock(v->lock);
	long unsigned int nb = sorted_vect_count(v);
	if(nb == v->max_number){
		void* buffer = malloc(2*v->max_number*v->obj_size);
		memcpy(buffer, v->first, nb*v->obj_size);
		free(v->first);
		v->first = buffer;
		v->last = v->first + (nb - 1)*v->obj_size;
		v->max_number *= 2;
	}

	void *out = NULL, *next = find_next(v, element);
	v->last += v->obj_size;
	if(next){
		memmove(next + v->obj_size, next, v->last - next);
		memcpy(next, element, v->obj_size);
		out = next;
	}else{
		memcpy(v->last, element, v->obj_size);
		out = v->last;
	}
	pthread_mutex_unlock(v->lock);

	return out;
}

int   sorted_vect_remove(p_s_sorted_vect v, void* element){
	int out = FAULT;

	pthread_mutex_lock(v->lock);
	long unsigned int nb = sorted_vect_count(v);
	if(nb < (v->max_number / 2)){
		void* buffer = malloc(v->max_number*v->obj_size / 2);
		memcpy(buffer, v->first, nb);
		free(v->first);
		v->first = buffer;
		v->last = v->first + (nb - 1)*v->obj_size;
		v->max_number /= 2;
	}

	if((v->first <= v->last) && (v->first <= element) &&
		(element <= v->last) && !((element - v->first) % v->obj_size)){

		void *obj = bsearch_custom(element, v->first, v->last, v->obj_size, v->cmp_func);
		if(obj) {
			memmove(obj, obj + v->obj_size, v->last - obj);
			v->last -= v->obj_size;
			out = OK;
		}
	}
	pthread_mutex_unlock(v->lock);
	return out;
}

void sorted_vect_clear(p_s_sorted_vect v){
	pthread_mutex_lock(v->lock);

	v->first 		= realloc(v->first, fast_default_max_nb * v->obj_size);
	v->last 		= v->first - v->obj_size;
	v->max_number 	= fast_default_max_nb;
	memset(v->first, 0, fast_default_max_nb * v->obj_size);

	pthread_mutex_unlock(v->lock);
}

