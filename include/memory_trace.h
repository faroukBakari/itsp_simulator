/*
 * memory_trace.h
 *
 *  Created on: 31 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_MEMORY_TRACE_H_
#define INCLUDE_MEMORY_TRACE_H_

extern void * i_malloc(long unsigned int size, const char* file, const char* function);

extern void   i_free(void* addr, const char* file, const char* function);

extern void * i_calloc(long unsigned int count, long unsigned int eltsize, const char* file, const char* function);

extern void * i_realloc(void* hint, long unsigned int size, const char* file, const char* function);

#define malloc(...) i_malloc(__VA_ARGS__,__FILE__, __func__)
#define free(...) i_free(__VA_ARGS__,__FILE__, __func__)
#define calloc(...) i_calloc(__VA_ARGS__,__FILE__, __func__)
#define realloc(...) i_realloc(__VA_ARGS__,__FILE__, __func__)

#endif /* INCLUDE_MEMORY_TRACE_H_ */
