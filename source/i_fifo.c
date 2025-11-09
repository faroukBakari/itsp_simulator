/*
 * i_fifo.c
 *
 *  Created on: 11 avr. 2017
 *      Author: f.baccari
 */


#include<stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<i_tools.h>
#include<i_fifo.h>
#include<i_thread.h>


p_s_fifo fifo_init(){
	p_s_fifo f = malloc(sizeof(s_fifo));
	memset(f, 0, sizeof(s_fifo));
	f->lock = mutex_init();
	return f;
}

inline int fifo_count(p_s_fifo fifo){
	int nb;
	pthread_mutex_lock(fifo->lock);
	nb = fifo->number;
	pthread_mutex_unlock(fifo->lock);
	return nb;
}


void fifo_push(p_s_fifo fifo, void* data){

	p_fifo_slot new_slot = malloc(sizeof(fifo_slot));
	new_slot->next = NULL;
	new_slot->object = data;

	pthread_mutex_lock(fifo->lock);
	if(fifo->number++ > 0){
		if(fifo->number > CRITICAL_FIFO_THRESHOLD){
			pthread_mutex_unlock(fifo->lock);
			fifo->end->next = new_slot;
			fifo->end 		= new_slot;
		}
		else{
			fifo->end->next = new_slot;
			fifo->end 		= new_slot;
			pthread_mutex_unlock(fifo->lock);
		}
	}
	else{
		fifo->begin	= new_slot;
		fifo->end	= new_slot;
		pthread_mutex_unlock(fifo->lock);
	}
	return;

}

void fifo_push_prior(p_s_fifo fifo, void* data){

	p_fifo_slot new_slot = malloc(sizeof(fifo_slot));
	new_slot->object = data;

	pthread_mutex_lock(fifo->lock);
	if(fifo->number++ > 0){
		new_slot->next	= fifo->begin;
		fifo->begin		= new_slot;
	}else{
		new_slot->next	= NULL;
		fifo->begin		= new_slot;
		fifo->end		= new_slot;
	}
	pthread_mutex_unlock(fifo->lock);
	return;

}


void* fifo_pull(p_s_fifo fifo){

	void* data = NULL;
	p_fifo_slot tmp = NULL;

	pthread_mutex_lock(fifo->lock);
	if(fifo->number > 0){
		if(fifo->number-- > CRITICAL_FIFO_THRESHOLD){
			pthread_mutex_unlock(fifo->lock);
			tmp 		= fifo->begin;
			data 		= tmp->object;
			fifo->begin = tmp->next;
		}
		else{
			tmp 		= fifo->begin;
			data 		= tmp->object;
			fifo->begin = tmp->next;
			pthread_mutex_unlock(fifo->lock);
		}
		free(tmp);
	}
	else
		pthread_mutex_unlock(fifo->lock);
	return data;
}

void* fifo_show_first(p_s_fifo fifo){
	void* out = NULL;
	pthread_mutex_lock(fifo->lock);
	if(fifo->number > 0)
		out = fifo->begin->object;
	pthread_mutex_unlock(fifo->lock);
	return out;
}

void fifo_clear(p_s_fifo fifo){
	pthread_mutex_lock(fifo->lock);
	p_fifo_slot tmp = NULL;
	while((tmp = fifo->begin)){
		fifo->begin = fifo->begin->next;
		free(tmp);
	}
	fifo->begin = NULL;
	fifo->end = NULL;
	fifo->number = 0;
	pthread_mutex_unlock(fifo->lock);
}

void fifo_free(p_s_fifo fifo){
	pthread_mutex_lock(fifo->lock);
	p_fifo_slot tmp = NULL;
	while((tmp = fifo->begin)){
		fifo->begin = fifo->begin->next;
		free(tmp);
	}
	free(fifo->lock);
	free(fifo);
}




