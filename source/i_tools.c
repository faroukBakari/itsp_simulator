/*
 * i_tools.c
 *
 *  Created on: 16 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <i_tools.h>

FILE* std_out = NULL;
FILE* std_err = NULL;

int ACTIVE_DEBUG = 0;
long char_key_com(const void* k1, const void* k2){
	return (long)(*(const char*)k1 - *(const char*)k2);
}

long short_key_com(const void* k1, const void* k2){
	return (long)(*(const short*)k1 - *(const short*)k2);
}

long int_key_com(const void* k1, const void* k2){
	return (long)(*(const int*)k1 - *(const int*)k2);
}

long long_key_com(const void* k1, const void* k2){
	return (long)(*(const long*)k1 - *(const long*)k2);
}

long _12_key_com(const void* k1, const void* k2){
	return (long)memcmp(k1, k2, 12);
}

long _16_key_com(const void* k1, const void* k2){
	return (long)memcmp(k1, k2, 16);
}

long _24_key_com(const void* k1, const void* k2){
	return (long)memcmp(k1, k2, 24);
}

long _32_key_com(const void* k1, const void* k2){
	return (long)memcmp(k1, k2, 32);
}

long ullint_key_cmp(const void* k1, const void* k2){
	return (long)memcmp(k1, k2, sizeof(unsigned long long int));
}

long ptr_key_com(const void* k1, const void* k2){
	return (long)(*(const void**)k1 - *(const void**)k2);
}

long string_key_com(const void* k1, const void* k2){
	return (long)strcmp((const char*)k1, (const char*)k2);
}

long double_key_com(const void* k1, const void* k2){
	return ((*(const double *)k1 > *(const double *)k2)) - ((*(const double *)k1 < *(const double *)k2));
}

void DEBUG(FILE* stream, const char* format, ...){
	if(ACTIVE_DEBUG){
		va_list ap;
		va_start (ap, format);
		vfprintf(stream, format, ap);
		 va_end (ap);
		fflush(stdout);
	}
}

void* bsearch_next(void* element, void* first, void* last, long unsigned int obj_sz, cmp_func_type *cmp_func){
	void *pivot = NULL;
	signed long int cmp = 0, delta = 0;
	if(first <= last){
		if(cmp_func(element, last) < 0){
			if(cmp_func(element, first) <= 0)
				pivot = first;
			else{
				do{
					delta = obj_sz * (((last - first) / obj_sz) / 2);
					pivot = first + delta;
					if((cmp = cmp_func(pivot, element)))
						if(cmp > 0) last = pivot;
						else first = pivot;
					else break;
				}while(delta);
				pivot = cmp ? last : pivot;
			}
		}
		else
			pivot = cmp_func(element, last) ? NULL : last;
	}
	return pivot;
}

void* bsearch_custom(void* element, void* first, void* last, long unsigned int obj_sz, cmp_func_type *cmp_func){
	void *pivot = NULL;
	signed long int cmp = 0, delta = 0;
	if((first <= last) && (cmp_func(first, element) <= 0) && (cmp_func(element, last) <= 0)){
		do{
			delta = obj_sz * (((last - first) / obj_sz) / 2);
			pivot = first + delta;
			if((cmp = cmp_func(pivot, element)))
				if(cmp > 0)
					last = pivot;
				else
					first = pivot;
			else
				return pivot;
		}while(delta);
		if(!cmp_func(element, last))
			return last;
	}
	return NULL;
}

void swap_(long unsigned int* a, long unsigned int*b){
	if(a!=b){
		*a^=*b;*b^=*a;*a^=*b;
	}
}

void printtostring(char* buff, const char* format, ... ) {
    va_list args;
    va_start( args, format );
    vsprintf( buff, format, args );
    va_end( args );
}
