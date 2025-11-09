/*
 * itsp_scheduler.h
 *
 *  Created on: 18 avr. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_SCHEDULER_H_
#define INCLUDE_ITSP_SCHEDULER_H_

#include <itsp_structs.h>
#include <s3k_structs.h>
#include <itsp_cnx.h>

extern p_action_sequence make_sequence(
										p_s_itsp_cnx cnx,
										p_itsp_frame_type seq_type,
										itsp_sequence_way way,
										int race,
										char* pool,
										int seq_nb,
										void* special_data
									  );

#define itsp_terminate_sequence(cnx, seq, reason_, msg_format, ...) ({\
	 void* action = NULL;\
	 if(fifo_count(seq->actions)){\
		 while((action = fifo_pull(seq->actions)))\
			 free(action);\
		 p_itsp_action neagative_ack = malloc(sizeof(itsp_action));\
		 neagative_ack->message = msg_acknowledge;\
		 neagative_ack->reason = reason_;\
		 neagative_ack->type = action_send;\
		 fifo_push(seq->actions, neagative_ack);\
	 }\
	 if(seq->special_data) free(seq->special_data);\
	 seq->special_data = NULL;\
	 if(msg_format){\
		 seq->special_data = malloc(DATA_ALERT_MAX_MSG_LENGTH);\
		 printtostring(seq->special_data, msg_format, ##__VA_ARGS__);\
		 TRACE(cnx->log_file, "Warning : %s\n", (char*)seq->special_data);\
	 }\
	 reason_;\
 })

#define schedule_sequence(cnx, priority, ...) ({\
		p_action_sequence seq_donotusethis = make_sequence(cnx, __VA_ARGS__);\
		if(priority)\
			fifo_push_prior(cnx->sequences_pipe, seq_donotusethis);\
		else\
			fifo_push(cnx->sequences_pipe, seq_donotusethis);\
	})

#endif /* INCLUDE_ITSP_SCHEDULER_H_ */
