/*
 * i_thread.h
 *
 *  Created on: 4 avr. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_I_THREAD_H_
#define INCLUDE_I_THREAD_H_

#include<pthread.h>

typedef enum thread_state_enum{
	th_created,
	th_running,
	th_killed,
	th_finshed,
	th_joined,
	th_error
}thread_state_enum;

typedef struct thread_state{
	volatile thread_state_enum	state;
	pthread_mutex_t* 			lock;
}thread_state;

typedef struct s_thread{
	void* (*function)();
	void* param;
	/*****************/
	pthread_t parrent;
	pthread_t self;
	pthread_attr_t attribute;
	thread_state state[1];
}s_thread, *p_s_thread;

extern pthread_mutex_t*		mutex_init();

extern p_s_thread 			_create_thread(void* function, void* param, pthread_t parrent);

extern int 					run_thread(p_s_thread new_thread);

extern thread_state_enum 	get_th_state(p_s_thread th);

extern int 					join_thread(p_s_thread th);

extern int					force_stop_thread(p_s_thread th);

/*************************************************************************/

#define create_thread(func, param) _create_thread(func,param,pthread_self())

#endif /* INCLUDE_I_THREAD_H_ */
