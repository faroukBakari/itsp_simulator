/*
 * i_sorted_set.c
 *
 *  Created on: 20 juil. 2017
 *      Author: f.baccari
 */


#include <stdio.h>
#include <stdlib.h>
#include <i_tools.h>
#include <i_string.h>
#include <i_sorted_set.h>
 #include<time.h>

#define sorted_set_size(v) (v->last - v->first + v->obj_size)

#define find_next(set, element) bsearch_next(element, set->first, set->last, set->obj_size, set->cmp_func)

p_s_sorted_set	sorted_set_init(long unsigned int obj_size, void* cmp_func){
	p_s_sorted_set v = malloc(sizeof(s_sorted_set));

	v->lock 		= mutex_init();
	v->first 		= malloc(fast_default_max_nb * obj_size);
	v->last 		= v->first - obj_size;
	v->obj_size 	= obj_size;
	v->max_number 	= fast_default_max_nb;
	v->cmp_func		= cmp_func;

	memset(v->first, 0, fast_default_max_nb * obj_size);
	return v;
}

void* sorted_set_insert(p_s_sorted_set set, void* element){
	pthread_mutex_lock(set->lock);
	long unsigned int nb = sorted_set_count(set);


	if(nb == (set->max_number)){
		void* buffer = malloc(2*set->max_number*set->obj_size);
		memcpy(buffer, set->first, nb*set->obj_size);
		free(set->first);
		set->first = buffer;
		set->last = set->first + (nb - 1)*set->obj_size;
		set->max_number *= 2;
	}



	void *out = NULL, *next = find_next(set, element);

	if(next){
		if(((cmp_func_type*)set->cmp_func)(element, next)){
			set->last += set->obj_size;
			memmove(next + set->obj_size, next, set->last - next);
			memcpy(next, element, set->obj_size);
		}
		out = next;
	}else{
		set->last += set->obj_size;
		memcpy(set->last, element, set->obj_size);
		out = set->last;
	}

	pthread_mutex_unlock(set->lock);
	return out;
}

void* sorted_set_at(p_s_sorted_set set, int i){
	return set->first + set->obj_size * i;
}

int   sorted_set_remove(p_s_sorted_set v, void* element){
	int out = FAULT;

	pthread_mutex_lock(v->lock);
	if(v->first <= v->last){
		void *obj = bsearch_custom(element, v->first, v->last, v->obj_size, v->cmp_func);
		if(obj) {
			memmove(obj, obj + v->obj_size, v->last - obj);
			v->last -= v->obj_size;
			out = OK;
		}
	}

	if(sorted_set_count(v) < (v->max_number / 2)){
		void* buffer = malloc((v->max_number/2)*v->obj_size);
		memcpy(buffer, v->first, sorted_set_size(v));
		v->last = buffer + (v->last - v->first);
		free(v->first);
		v->first = buffer;
		v->max_number /= 2;
	}

	pthread_mutex_unlock(v->lock);
	return out;
}

void sorted_set_clear(p_s_sorted_set v){
	pthread_mutex_lock(v->lock);

	v->first 		= realloc(v->first, fast_default_max_nb * v->obj_size);
	v->last 		= v->first - v->obj_size;
	v->max_number 	= fast_default_max_nb;
	memset(v->first, 0, fast_default_max_nb * v->obj_size);

	pthread_mutex_unlock(v->lock);
}

void sorted_set_free(p_s_sorted_set v){
	pthread_mutex_lock(v->lock);
	free(v->lock);
	free(v->first);
	free(v);
}

int	sorted_set_union(p_s_sorted_set set1, p_s_sorted_set set2){

	void *ptr1 = set1->first, *ptr2 = set2->first, *set1_last = NULL,
			*set2_last = NULL, *ptr2_n = NULL;
	cmp_func_type *cmp_func = set1->cmp_func;
	long unsigned int obj_sz = set1->obj_size;
	int out = FAULT, cmp = 0;

	pthread_mutex_lock(set1->lock);
	pthread_mutex_lock(set2->lock);
	if((set1->obj_size == set2->obj_size) && (set1->cmp_func = set2->cmp_func)){
		if(set2->first <= set2->last){
			if(set1->first <= set1->last){

				if(cmp_func(set2->first, set1->first) < 0){
					if(!(ptr2 = find_next(set2, set1->first)))
						ptr2 = set2->last + obj_sz;
					if((ptr2 - set2->first) > (set1->max_number * obj_sz - (set1->last - set1->first + obj_sz))){
						void* buffer = malloc(2*((ptr2 - set2->first) + (set1->last + obj_sz - set1->first)));
						memcpy(buffer, set2->first, (ptr2 - set2->first));
						memcpy(buffer + (ptr2 - set2->first), set1->first, set1->last + obj_sz - set1->first);
						set1->last = buffer + (ptr2 - set2->first) + (set1->last - set1->first);
						free(set1->first);
						set1->first = buffer;
						set1->max_number = 2*sorted_set_count(set1);
					}
					else{
						memmove(set1->first + (ptr2 - set2->first), set1->first, sorted_set_size(set1));
						memcpy(set1->first, set2->first, ptr2 - set2->first);
						set1->last += ptr2 - set2->first;
					}
					ptr1 = set1->first + (ptr2 - set2->first);
					out = OK;
				}else


				if(ptr2 <= set2->last){
					cmp = cmp_func(set1->last, set2->last);
					set2_last = (cmp < 0) ? bsearch_next(set1->last, ptr2, set2->last, obj_sz, cmp_func) : set2->last;
					set1_last = (cmp > 0) ? bsearch_next(set2->last, ptr1, set1->last, obj_sz, cmp_func) : set1->last;


					while(((ptr1	= bsearch_next(ptr2, ptr1, set1_last, obj_sz, cmp_func))) &&
						  ((ptr2_n	= bsearch_next(ptr1, ptr2, set2_last, obj_sz, cmp_func)))){
						if(ptr2 < ptr2_n){
							if((ptr2_n - ptr2) > (set1->max_number * obj_sz - (set1->last - set1->first + obj_sz))){
								void* buffer = malloc(2 * set1->max_number * obj_sz + (ptr2_n - ptr2));
								memcpy(buffer, set1->first, ptr1 - set1->first);
								memcpy(buffer + (ptr1 - set1->first), ptr2, ptr2_n - ptr2);
								memcpy(buffer + (ptr1 - set1->first) + (ptr2_n - ptr2), ptr1, set1->last + obj_sz - ptr1);
								ptr1 = buffer + (ptr1 - set1->first) + (ptr2_n - ptr2);
								set1_last = buffer + (ptr2_n - ptr2) + (set1_last - set1->first);
								set1->last = buffer + (ptr2_n - ptr2) + (set1->last - set1->first);
								free(set1->first);
								set1->first = buffer;
								set1->max_number = 2 * set1->max_number + (ptr2_n - ptr2) / obj_sz;
							}else{
								memmove(ptr1 + (ptr2_n - ptr2), ptr1, set1->last + obj_sz - ptr1);
								memcpy(ptr1, ptr2, ptr2_n - ptr2);
								ptr1 += ptr2_n - ptr2;
								set1_last += ptr2_n - ptr2;
								set1->last += ptr2_n - ptr2;
							}
							ptr2 = ptr2_n;
						}else
							ptr2 += obj_sz;
					}

					ptr1 = ptr1 ? ptr1 : set1_last + obj_sz;
					ptr2 = ptr2 ? ptr2 : set2_last + obj_sz;

					if(ptr2 <= set2->last){
						if((set2->last - ptr2 + obj_sz) > (set1->max_number * obj_sz - (set1->last - set1->first + obj_sz))){
							void* buffer = malloc(2 * set1->max_number * obj_sz + (set2->last - ptr2));
							memcpy(buffer, set1->first, ptr1 - set1->first);
							memcpy(buffer + (ptr1 - set1->first), ptr2, set2->last + obj_sz - ptr2);
							memcpy(buffer + (ptr1 - set1->first) + (set2->last + obj_sz - ptr2), ptr1, set1->last + obj_sz - ptr1);
							set1->last = buffer + (set2->last + obj_sz - ptr2) + (set1->last - set1->first);
							free(set1->first);
							set1->first = buffer;
							set1->max_number = 2 * set1->max_number + (set2->last - ptr2) / obj_sz;
						}else{
							memmove(ptr1 + (set2->last + obj_sz - ptr2), ptr1, set1->last + obj_sz - ptr1);
							memcpy(ptr1, ptr2, set2->last + obj_sz - ptr2);
							set1->last += set2->last + obj_sz - ptr2;
						}
					}
				}

			}else{
				free(set1->first);
				set1->first 		= malloc(set2->max_number * set2->obj_size);
				memcpy(set1->first, set2->first, sorted_set_size(set2));
				set1->last 			= set1->first + (set2->last - set2->first);
				set1->max_number 	= set2->max_number;
			}
		}
		out = OK;
	}

	pthread_mutex_unlock(set1->lock);
	pthread_mutex_unlock(set2->lock);
	return out;
}

int	sorted_set_intersect(p_s_sorted_set set1, p_s_sorted_set set2){

	void *ptr1 = set1->first, *ptr2 = set2->first, *set1_last = set1->last,
		 *set2_last = set2->last, *ptr1_n = NULL;
	cmp_func_type *cmp_func = set1->cmp_func;
	long unsigned int obj_sz = set1->obj_size;
	int out = FAULT, cmp = 0;

	pthread_mutex_lock(set1->lock);
	pthread_mutex_lock(set2->lock);
	if((set1->obj_size == set2->obj_size)&&(set1->cmp_func = set2->cmp_func)){

		if((set1->first <= set1->last) && (set2->first <= set2->last)){

			if((ptr1 = find_next(set1, set2->first)) && (ptr1 != set1->first)){
				memmove(set1->first, ptr1, set1->last - ptr1 + obj_sz);
				set1->last -= ptr1 - set1->first;
				ptr1 = set1->first;
			}

			cmp = cmp_func(set1->last, set2->last);
			if(cmp < 0) set2_last = bsearch_next(set1->last, ptr2, set2->last, obj_sz, cmp_func);
			if(cmp > 0) set1_last = bsearch_next(set2->last, ptr1, set1->last, obj_sz, cmp_func);

			while((ptr2		= bsearch_next(ptr1, ptr2, set2_last, obj_sz, cmp_func)) &&
				  (ptr1_n	= bsearch_next(ptr2, ptr1, set1_last, obj_sz, cmp_func))){
				if(ptr1 < ptr1_n){
					memmove(ptr1, ptr1_n, set1->last + obj_sz -ptr1_n);
					set1->last -= ptr1_n - ptr1;
					set1_last  -= ptr1_n - ptr1;
				}
				else
					ptr1 += obj_sz;
			}

			if((ptr1 <= set1->last))
				set1->last = ptr1 - obj_sz;

			if(sorted_set_count(set1) < (set1->max_number / 2)){
				void* buffer = malloc((set1->max_number/2)*set1->obj_size);
				memcpy(buffer, set1->first, sorted_set_size(set1));
				set1->last = buffer + (set1->last - set1->first);
				free(set1->first);
				set1->first = buffer;
				set1->max_number /= 2;
			}

		}
		else{
			set1->first 		= realloc(set1->first, fast_default_max_nb * set1->obj_size);
			set1->last 			= set1->first - set1->obj_size;
			set1->max_number 	= fast_default_max_nb;
			memset(set1->first, 0, fast_default_max_nb * set1->obj_size);
		}
		out = OK;

	}

	pthread_mutex_unlock(set1->lock);
	pthread_mutex_unlock(set2->lock);
	return out;
}

int	sorted_set_extract(p_s_sorted_set set1, p_s_sorted_set set2){
	void *ptr1 = set1->first, *ptr2 = set2->first, *set1_last = set1->last, *set2_last = set2->last, *ptr1_n = NULL;
	cmp_func_type *cmp_func = set1->cmp_func;
	long unsigned int obj_sz = set1->obj_size;
	int out = FAULT, cmp = 0;

	pthread_mutex_lock(set1->lock);
	pthread_mutex_lock(set2->lock);

	if((set1->obj_size == set2->obj_size)&&(set1->cmp_func = set2->cmp_func)){

		if((set1->first <= set1->last) && (set2->first <= set2->last)){

			cmp = cmp_func(set1->last, set2->last);
			if(cmp < 0) set2_last = bsearch_next(set1->last, ptr2, set2->last, obj_sz, cmp_func);
			if(cmp > 0) set1_last = bsearch_next(set2->last, ptr1, set1->last, obj_sz, cmp_func);

			while((ptr2		= bsearch_next(ptr1, ptr2, set2_last, obj_sz, cmp_func)) &&
				  (ptr1		= bsearch_next(ptr2, ptr1, set1_last, obj_sz, cmp_func))){

				ptr1_n = ptr1;
				while((ptr1_n <= set1_last) && (ptr2 <= set2_last) && !cmp_func(ptr1_n,ptr2)){
					ptr1_n += obj_sz;
					ptr2 += obj_sz;
				}

				if(ptr1 < ptr1_n){
					memmove(ptr1, ptr1_n, set1->last + obj_sz - ptr1_n);
					set1->last -= ptr1_n - ptr1;
					set1_last -= ptr1_n - ptr1;
				}


			}
		}
		out = OK;
	}
	pthread_mutex_unlock(set1->lock);
	pthread_mutex_unlock(set2->lock);
	return out;
}

p_s_sorted_set	sorted_set_clone(p_s_sorted_set set){

	p_s_sorted_set v = malloc(sizeof(s_sorted_set));
	v->lock 		= mutex_init();
	v->first 		= malloc(set->max_number * set->obj_size);

	pthread_mutex_lock(set->lock);

	memcpy(v->first, set->first, sorted_set_size(set));
	v->last 		= v->first + (set->last - set->first);
	v->obj_size 	= set->obj_size;
	v->max_number 	= set->max_number;
	v->cmp_func		= set->cmp_func;

	pthread_mutex_unlock(set->lock);


	return v;
}






















