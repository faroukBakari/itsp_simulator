/*
 * memory_trace.c
 *
 *  Created on: 31 mars 2017
 *      Author: f.baccari
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <i_tools.h>
#include <i_exit.h>
#include <i_string.h>

static FILE* memory_trace = NULL;

typedef struct mem_block{
	void* addr;
	long unsigned int size;
	struct mem_block* next;
}mem_block, *p_mem_block;

static mem_block root = {0};


static inline p_mem_block find_previous_blk(void* addr){
	p_mem_block runner = &root, previous = NULL;
	while((runner)&&(runner->addr < addr)){
		previous = runner;
		runner = runner->next;
	}
	return previous;
}

static inline void* add_block(void* addr, long unsigned int size){
	p_mem_block new_block = malloc(sizeof(mem_block));
	new_block->addr = addr;
	new_block->size = size;
	p_mem_block prev = find_previous_blk(addr);
	if(!prev){
		new_block->next = &root;
	}
	else{
		new_block->next = prev->next;
		prev->next = new_block;
	}
	return addr;
}

static inline void* remove_block(void* addr){
	if(!memory_trace){
		fprintf(std_err,"empty block list! aborting...");
		abort();
	}
	p_mem_block prev = find_previous_blk(addr), tmp = NULL;
	if(!prev){
		root = *(root.next);
		free(root.next);
	}
	else{
		if(prev->next){
			tmp =prev->next->next;
			free(prev->next);
		}
		prev->next = tmp;
	}
	return addr;
}

static inline long unsigned int get_block_size(void* addr){
	p_mem_block prev = find_previous_blk(addr);
	if(!prev && (root.addr == addr))
		return root.size;
	prev = prev->next;
	if(prev && (prev->addr == addr))
		return prev->size;
	fprintf(std_err,"get_block_size error");
	return FAULT;
}


/*********************************************************************/

void * i_malloc(long unsigned int size, const char* file, const char* function){
	if(!memory_trace){
		memory_trace = fopen("memory_trace.txt","w+");
		fprintf(memory_trace,"File;Function;Memory\n");
		i_at_exit(fclose, memory_trace);
	}
	fprintf(memory_trace,"%s;%s;%d\n", file, function, (int)size);
	return add_block(malloc(size),size);
}

void i_free(void* addr, const char* file, const char* function){
	fprintf(memory_trace,"%s;%s;%d\n", file, function, -(int)get_block_size(addr));
	remove_block(addr);
	free(addr);
}

void * i_calloc(long unsigned int count, long unsigned int eltsize, const char* file, const char* function){
	return i_malloc(count * eltsize, file, function);
}

void * i_realloc(void* hint, long unsigned int size, const char* file, const char* function){
	if(hint){
		fprintf(memory_trace,"%s;%s;%d\n", file, function, -(int)get_block_size(hint));
		remove_block(hint);
	}
	fprintf(memory_trace,"%s;%s;%d\n", file, function, (int)size);
	return add_block(realloc(hint, size),size);
}
