/*
 * itsp_remote.c
 *
 *  Created on: 30 mai 2017
 *      Author: f.baccari
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <limits.h>
 #include <i_string.h>
 #include <itsp_header.h>
 #include <itsp_frame.h>
 #include <s3k_session.h>
 #include <itsp_cnx.h>
 #include <itsp_scheduler.h>
 #include <itsp_frame_maker.h>
 #include <itsp_frame_analyser.h>
 #include <itsp_pilot.h>

socklen_t size_addr = sizeof(struct sockaddr_in);

static int cnx_ctr = 1;
static pthread_mutex_t cnx_xtr_lock = PTHREAD_MUTEX_INITIALIZER;
#define get_nu_cnx() ({\
		int out = 0;\
		pthread_mutex_lock(&cnx_xtr_lock);\
		cnx_ctr++;\
		cnx_ctr %= (INT_MAX - 1);\
		out = cnx_ctr;\
		pthread_mutex_unlock(&cnx_xtr_lock);\
		out;\
 })

static int net_fr_ctr = 101;
static pthread_mutex_t net_fr_ctr_lock = PTHREAD_MUTEX_INITIALIZER;
#define get_nu_net_fr() ({\
		int out = 0;\
		pthread_mutex_lock(&net_fr_ctr_lock);\
		net_fr_ctr++;\
		net_fr_ctr %= (INT_MAX - 1);\
		if(net_fr_ctr < 101) net_fr_ctr = 101;\
		out = net_fr_ctr;\
		pthread_mutex_unlock(&net_fr_ctr_lock);\
		out;\
 })

p_s_net_frame new_net_frame(){
		p_s_net_frame net_fr = malloc(sizeof(s_net_frame));
		memset(net_fr, 0, sizeof(s_net_frame));
		net_fr->frame_id = get_nu_net_fr();
		net_fr->date_time = time(NULL);
		return net_fr;
	}

int process_received_data(p_s_itsp_cnx cnx, char* buff, int size) {
	int out = FAULT, i = 0;
	p_s_net_frame fr_fr = NULL;
	while(i < size){
		out = FAULT; i = 0;
		if(*buff++ != STX) break;
		while((buff[i] != ETX))
			if(i++ == size) break;
		out = OK;

		fr_fr = new_net_frame();
		fr_fr->size = i;
		fr_fr->data = malloc(i + 1);
		memcpy(fr_fr->data, buff, i);
		fr_fr->data[i] = 0;

		process_cnx_event(cnx, event_receve, fr_fr);
		buff += i + 1; size -= i + 2;
	}
	return out;
}

p_s_net_frame make_net_frame(char* buff, int size) {
	p_s_net_frame fr_fr = new_net_frame();
	fr_fr->size = size + 2;
	fr_fr->data = malloc(size + 3);
	memcpy(fr_fr->data + 1, buff, size);
	fr_fr->data[0] = STX;
	fr_fr->data[size + 1] = ETX;
	fr_fr->data[size + 2] = 0;
	return fr_fr;
}

void cnx_set_phy_status(p_s_itsp_cnx cnx, phy_cnx_status_type status){
	pthread_mutex_lock(cnx->lock);
	cnx->phy_status = status;
	pthread_mutex_unlock(cnx->lock);
}

phy_cnx_status_type cnx_get_phy_status(p_s_itsp_cnx cnx){
	pthread_mutex_lock(cnx->lock);
	phy_cnx_status_type value = cnx->phy_status;
	pthread_mutex_unlock(cnx->lock);
	return value;
}

void cnx_set_logi_status(p_s_itsp_cnx cnx, itsp_cnx_status_type status){
	pthread_mutex_lock(cnx->lock);
	cnx->logi_status = status;
	pthread_mutex_unlock(cnx->lock);
}

itsp_cnx_status_type cnx_get_logi_status(p_s_itsp_cnx cnx){
	pthread_mutex_lock(cnx->lock);
	phy_cnx_status_type value = cnx->logi_status;
	pthread_mutex_unlock(cnx->lock);
	return value;
}

p_s_itsp_cnx cnx_init(){
	p_s_itsp_cnx cnx = malloc(sizeof(s_itsp_cnx));
	memset(cnx, 0 , sizeof(s_itsp_cnx));
	cnx->nu_cnx = get_nu_cnx();
	cnx->log_file = stdout;
	cnx->lock = mutex_init();
	cnx->phy_status = phy_cnx_off;
	cnx->logi_status = itsp_cnx_off;
	cnx->output_frames = fifo_init();
	cnx->working_sequences = fast_map_init(sizeof(int), sizeof(action_sequence), int_key_com);
	cnx->current_seq = 0;
	cnx->sequences_pipe = fifo_init();
	cnx->current_race = 1;
	return cnx;
}

void cnx_init_(p_s_itsp_cnx cnx){
	cnx->nu_cnx = get_nu_cnx();
	cnx->log_file = stdout;
	cnx->lock = mutex_init();
	cnx->phy_status = phy_cnx_off;
	cnx->logi_status = itsp_cnx_off;
	cnx->output_frames = fifo_init();
	cnx->working_sequences = fast_map_init(sizeof(int), sizeof(action_sequence), int_key_com);
	cnx->current_seq = 0;
	cnx->sequences_pipe = fifo_init();
	cnx->current_race = 1;
}

int cnx_phy_connect(p_s_itsp_cnx cnx){
	int out = FAULT;

	if(cnx_get_phy_status(cnx) != phy_cnx_off){
		DEBUG_TOFILE(cnx->log_file, "error : cnx->phy_status = %d", cnx->phy_status);
	}else{
		if ((cnx->socket = socket(AF_INET , SOCK_STREAM , 0)) == -1){
			DEBUG_TOFILE(cnx->log_file,"error %d on socket : %s\n", errno, strerror(errno));
		}
		else{
			TRACE(cnx->log_file, "connection attempt with remote socket %d... ", cnx->socket);
			cnx_set_phy_status(cnx, phy_cnx_attempt);
			while (((connect(cnx->socket , (struct sockaddr *)&cnx->address,
					sizeof(struct sockaddr))) == -1) && (cnx_get_phy_status(cnx) == phy_cnx_attempt)){
				DEBUG_TOFILE(cnx->log_file, "error %d in connect : %s", errno, strerror(errno));
				sleep(1);
			}

			cnx->timeout_check = time(NULL);
			out = OK;
			TRACE(cnx->log_file, " -> connected\n");
			cnx_set_phy_status(cnx, phy_cnx_on);
		}
	}

	return out;
}

p_s_itsp_cnx cnx_phy_accept(FILE* stream, int socket_){

	p_s_itsp_cnx cnx = cnx_init();

	if ((cnx->socket = accept(socket_, (struct sockaddr *)&cnx->address, (socklen_t*)&size_addr))<0) {
		DEBUG_TOFILE(stream,"accept_cnx error %d on accept : %s", errno, strerror(errno));
		free(cnx);
		return NULL;
	}

	cnx->timeout_check = time(NULL);
	cnx->phy_status = phy_cnx_server;

	return cnx;
}

#define FREE_FRAME(frame)\
	if(frame){\
		if(frame->data) free(frame->data);\
		free(frame);\
		frame = NULL;\
	}

int process_cnx_event(void* cnx_, event_type evt, p_s_net_frame frame){

	p_s_itsp_cnx cnx = cnx_;
	p_action_sequence seq = NULL;
	p_itsp_action action = NULL;
	p_itsp_frame fr = NULL;
	p_s_net_frame net_fr = NULL;
	itsp_frame_type fr_type = {0};
	int ret = 0;

	switch(evt){
	case event_error :
		DEBUG_WARN("cnx error for att \"%s\"", cnx->col_att);
		break;
	case event_connect :
		TRACE(cnx->log_file, "%s --> connected\n", (cnx->itsp_mode == mode_remote) ? "host" : cnx->col_att);
		itsp_init(cnx);
		seq = fifo_pull(cnx->sequences_pipe);
		break;
	case event_receve :
		TRACE(cnx->log_file, "%s --> %ld bites of data received : <<%s>>\n", (cnx->itsp_mode == mode_host) ? "host" : cnx->col_att, (long int)frame->size, frame->data);
		fr = malloc(sizeof(itsp_frame));
		if((ret = read_net_frame(frame, fr, cnx->log_file)) != OK)
			itsp_terminate_sequence(cnx, seq, ret, NULL);
		if(fr->header->frame_type->data == data_scan)
			log_frame(stdout, fr);
		/*if(fr->data_str && fr->data_str->structure)
			log_frame(cnx->log_file, fr);*/
		if(!(seq = fast_map_extract(cnx->working_sequences, &fr->header->sequence)))
			seq = make_sequence(cnx, fr->header->frame_type, way_in, fr->header->race_number, fr->header->pool_code, fr->header->sequence, NULL);
		break;
	case event_sent :
		if(!(seq = fast_map_extract(cnx->working_sequences, &frame->frame_id)))
			DEBUG_WARN("unexpected \"event_sent\" for att \"%s\"!\ndata : <<|%s|>>", cnx->col_att, frame->data);
		else{
			TRACE(cnx->log_file, "%s --> %ld bites of data sent\n", (cnx->itsp_mode == mode_host) ? "host" : cnx->col_att, (long int)frame->size - 2);
			if(fifo_count(seq->actions)){
				fast_map_insert(cnx->working_sequences, &seq->number, seq);
				free(seq); seq = NULL;
			}
		}
		break;
	case event_disconnect :
		DEBUG_WARN("cnx for att \"%s\" went off", cnx->col_att);
		break;
	case event_timeout :
		set_fr_type(fr_type, data_alert, msg_data, reason_no_reason);
		seq = make_sequence(cnx, &fr_type, way_out, 0, NULL, 0, NULL);
		break;
	default :
		seq = fifo_pull(cnx->sequences_pipe);
	}

	while(seq){
		if((action = fifo_pull(seq->actions))){
			switch(action->type){
			case action_receve :
				if(evt != event_receve){
					DEBUG_WARN("%s --> seq %d error : expected action_receve vs event = %d", (cnx->itsp_mode == mode_host) ? "host" : cnx->col_att, seq->number, evt);
					abort();
				}
				itsp_frame_analyze(cnx, fr, seq, action);
				break;
			case action_send :
				set_fr_type(fr_type, seq->data_type, action->message, action->reason);
				fr = malloc(sizeof(itsp_frame));
				if((ret = make_itsp_frame(cnx, seq, action, fr)) != OK){
					DEBUG_WARN("%s --> could not make %c%c-%c frame on grep \"%s\", race %d, pool \"%s\" (ret = %d)",
							(cnx->itsp_mode == mode_host) ? "host" : cnx->col_att, action->message, seq->data_type,
							 action->reason, cnx->grep, seq->race, seq->pool, ret);
					abort();
				}
				net_fr = write_net_frame(fr, cnx->log_file);

				cnx_set_frame(cnx, net_fr)
				fast_map_insert(cnx->working_sequences, &net_fr->frame_id, seq);
				free(seq); seq = NULL;
				break;
			case action_connect :
				break;
			case action_disconnect :
				break;
			default :
				DEBUG_WARN("enum var \"action->type\" out of range (%d)", action->type);
				abort();
			}
			if(fr){
				free_itsp_frame(fr);
				fr = NULL;
			}
			free(action);
			evt = 0;
		}
		else{
			fifo_free(seq->actions);
			if(seq->special_data)
				free(seq->special_data);
			free(seq);
			seq = (fast_map_count(cnx->working_sequences) < 10) ? fifo_pull(cnx->sequences_pipe) : NULL;
		}
	}
	FREE_FRAME(frame);
	return OK;
}

















