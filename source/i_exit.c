/*
 * xExit.c
 *
 *  Created on: 9 janv. 2017
 *      Author: f.baccari
 */
#include <stdio.h>
#include <stdlib.h>
#define INCLUDE_MEMORY_TRACE_H_
#include <i_tools.h>

typedef struct exit_routine{
	void* func;
	void* param;
	void* previous;
}exit_routine;

exit_routine* exit_routines = NULL;

static void i_exit(){
	exit_routine* TMP;
	while(exit_routines){
		TMP = exit_routines;
		if(exit_routines->param)
			((void(*)()) exit_routines->func)(exit_routines->param);
		else {
			((void(*)()) exit_routines->func)();
		}
		exit_routines = exit_routines->previous;
		free(TMP);
	}
}

void i_at_exit(void* func, void* param){
	if(!exit_routines)
		atexit(i_exit);
	exit_routine* previous = exit_routines;
	exit_routines = (exit_routine*) malloc(sizeof(exit_routine));
	exit_routines->previous = previous;
	exit_routines->func = func;
	exit_routines->param = param;
}
