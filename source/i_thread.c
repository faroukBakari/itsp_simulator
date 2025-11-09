/*
 * i_thread.c
 *
 *  Created on: 4 avr. 2017
 *      Author: f.baccari
 */


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<i_string.h>
#include<i_tools.h>
#include<pthread.h>
#include<i_thread.h>
#include<i_exit.h>
#include<errno.h>


inline pthread_mutex_t* mutex_init(){
	pthread_mutex_t mut_int_thread = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t* out = malloc(sizeof(pthread_mutex_t));
	memcpy(out, &mut_int_thread, sizeof(pthread_mutex_t));
	return out;
}

void set_th_state(p_s_thread th, thread_state_enum state){
	pthread_mutex_lock(th->state->lock);
	th->state->state = state;
	pthread_mutex_unlock(th->state->lock);
}

inline thread_state_enum get_th_state(p_s_thread th){
	pthread_mutex_lock(th->state->lock);
	thread_state_enum state = th->state->state;
	pthread_mutex_unlock(th->state->lock);
	return state;
}

void* thread_wrapper(void* th_v){
	p_s_thread th = th_v;
	set_th_state(th,th_running);
	void* out = th->function(th->param);
	set_th_state(th,th_finshed);
	return out;
}

p_s_thread _create_thread(void* function, void* param, pthread_t parrent){
	int odd = 0;
	p_s_thread new_thread = malloc(sizeof(s_thread));

	new_thread->function = function;
	new_thread->param = param;

	new_thread->parrent = parrent;
	new_thread->self = 0;
	new_thread->state->lock = mutex_init();
	set_th_state(new_thread, th_created);
	pthread_attr_init(&new_thread->attribute);
	pthread_attr_setdetachstate(&new_thread->attribute, PTHREAD_CREATE_JOINABLE);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &odd);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &odd);
	return new_thread;
}

int run_thread(p_s_thread th){
	int ret = 0;
	if(get_th_state(th) != th_running){
		if(!(ret = pthread_create(&th->self, &th->attribute, thread_wrapper, th))){
			i_at_exit(join_thread, th);
			return OK;
		}else
			fprintf(std_out, "run_thread Error %d on pthread_create\n", ret);
	}
	else
		fprintf(std_out, "run_thread Error : already running\n");

	fflush(std_out);
	return FAULT;
}

int join_thread(p_s_thread th){
	thread_state_enum th_st = get_th_state(th);
	if(th_st == th_running){
		if(pthread_join(th->self,NULL)) {
			set_th_state(th, th_error);
			fprintf(std_out, "Error joining thread\n");
			fflush(std_out);
		}
		set_th_state(th, th_joined);
		return OK;
	}

	if(th_st == th_finshed){
		set_th_state(th, th_joined);
		return OK;
	}

	return FAULT;
}


int force_stop_thread(p_s_thread th){
	int ret = 0;
	if(get_th_state(th) != th_running)
		return FAULT;
	if((ret = pthread_cancel(th->self))){
		fprintf(std_out, "force_stop_thread Error %d on pthread_cancel\n", ret);
		fflush(std_out);
		set_th_state(th, th_error);
	}
	else
		set_th_state(th, th_killed);
	return ret;
}
