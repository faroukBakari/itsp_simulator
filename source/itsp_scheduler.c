/*
 * itsp_actions.c
 *
 *  Created on: 18 avr. 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <i_string.h>
#include <i_tools.h>
#include <i_fast_map.h>
#include <itsp_scheduler.h>

typedef struct int_action{
	itsp_msg_type			message;
	itsp_action_type		action;
	itsp_sequence_way		way;
}int_action, *p_int_action;

typedef struct int_sequence_action{
	itsp_data_type		type;
	itsp_msg_type		msg;
	p_int_action		actions;
	int					interruptible_flag;
	int					host_only_flag;
}int_sequence_action, *p_int_sequence_action;

static int_action pending_seq[] = {
		{msg_pending,		action_send,	way_out			},
		{msg_pending,		action_receve,	way_in			},
		{msg_request,		action_receve,	way_out			},
		{msg_request,		action_send,	way_in			},
		{msg_data,			action_send,	way_out			},
		{msg_data,			action_receve,	way_in			},
		{msg_acknowledge,	action_receve,	way_out			},
		{msg_acknowledge,	action_send,	way_in			},

		{0,					0,				0				}
};

static int_action request_seq[7] = {
		{msg_request,		action_send,	way_out			},
		{msg_request,		action_receve,	way_in			},
		{msg_data,			action_receve,	way_out			},
		{msg_data,			action_send,	way_in			},
		{msg_acknowledge,	action_send,	way_out			},
		{msg_acknowledge,	action_receve,	way_in			},

		{0,					0,				0				}
};

static struct int_action data_seq[5] = {
		{msg_data,			action_send,	way_out			},
		{msg_data,			action_receve,	way_in			},
		{msg_acknowledge,	action_receve,	way_out			},
		{msg_acknowledge,	action_send,	way_in			},

		{0,					0,				0				}
};

static  int_sequence_action all_sequences_array[] = {
	{data_link,				msg_data,		(p_int_action)data_seq,		0,0},

	{data_config,			msg_pending,	(p_int_action)pending_seq,	1,1},
	{data_config,			msg_request,	(p_int_action)request_seq,	0,1},
	{data_config,			msg_data,		(p_int_action)data_seq,		0,1},

	{data_race_status,		msg_pending,	(p_int_action)pending_seq,	1,1},
	{data_race_status,		msg_request,	(p_int_action)request_seq,	1,1},
	{data_race_status,		msg_data,		(p_int_action)data_seq,		0,1},

	{data_pools,			msg_pending,	(p_int_action)pending_seq,	1,0},
	{data_pools,			msg_request,	(p_int_action)request_seq,	0,1},
	{data_pools,			msg_data,		(p_int_action)data_seq,		0,1},

	{data_scan,				msg_pending,	(p_int_action)pending_seq,	1,0},
	{data_scan,				msg_request,	(p_int_action)request_seq,	0,0},
	{data_scan,				msg_data,		(p_int_action)data_seq,		0,0},

	{data_pool_total,		msg_request,	(p_int_action)request_seq,	0,1},
	{data_pool_total,		msg_data,		(p_int_action)data_seq,		0,1},

	{data_payoffs,			msg_data,		(p_int_action)data_seq,		0,1},

	{data_results,			msg_data,		(p_int_action)data_seq,		0,1},

	{data_will_pay,			msg_pending,	(p_int_action)pending_seq,	1,1},
	{data_will_pay,			msg_data,		(p_int_action)data_seq,		0,1},

	{data_file_transfert,	msg_pending,	(p_int_action)pending_seq,	1,0},

	{data_alert,			msg_data,		(p_int_action)data_seq,		0,0},

	{0, 					msg_data,		(p_int_action)NULL,			0,0}
};


static const size_t seq_key_size = sizeof(itsp_data_type) + sizeof(itsp_msg_type);

long seq_key_cmp(const void* k1, const void* k2){
	return (long)memcmp(k1, k2, seq_key_size);
}

static p_s_fast_map sequencer = NULL;

p_action_sequence make_sequence(p_s_itsp_cnx cnx, p_itsp_frame_type seq_type, itsp_sequence_way way, int race, char* pool, int seq_nb, void* special_data){
	int i = 0;
	p_int_action msg = NULL;
	p_action_sequence sequence = NULL;
	p_itsp_action action = NULL;
	p_int_sequence_action seq = NULL;



	if(!sequencer){
		sequencer = fast_map_init(sizeof(itsp_data_type) + sizeof(itsp_msg_type), sizeof(int_sequence_action), seq_key_cmp);
		for(i = 0; all_sequences_array[i].actions; i++)
			fast_map_insert(sequencer, &all_sequences_array[i], &all_sequences_array[i]);
	}

	if(!(seq = fast_map_at(sequencer, seq_type))){
		DEBUG_TOFILE(cnx->log_file, "no sequence for msg : %c, data : %c", seq_type->message, seq_type->data);
		return FAULT;
	}

	if(seq->host_only_flag && (way == way_out) && (cnx->itsp_mode != mode_host)){
		DEBUG_TOFILE(cnx->log_file, "sequence (msg : %c, data : %c, way : %d) is host only", seq_type->message, seq_type->data, way);
		return FAULT;
	}

	if(seq_type->data == data_scan && seq_type->reason == reason_begin && way == way_out && cnx->itsp_mode == mode_remote){
		DEBUG_TOFILE(cnx->log_file, "sequence (msg : %c, data : %c, way : %d) is host only", seq_type->message, seq_type->data, way);
		return FAULT;
	}

	if((seq_type->data == data_race_status)){
		if((cnx->current_race != race) && (seq_type->reason == reason_begin)){
			TRACE(cnx->log_file, "\"%s\" -> %s a name_race sequence... race %d is set as current race\n",
					(cnx->itsp_mode == mode_host) ? "host" : cnx->col_att, (way == way_out)? "sending" : "receiving", race);
			cnx->current_race = race;
		}else if((cnx->current_race == race)&&(seq_type->reason != reason_begin)){
			if(cnx->itsp_mode == mode_host)
				seq_type->reason = reason_begin;
		}
	}

	i = 0;
	sequence = malloc(sizeof(action_sequence));
	memset(sequence, 0, sizeof(action_sequence));
	sequence->actions = fifo_init();
	sequence->data_type = seq_type->data;
	if(way == way_out){
		cnx->current_seq++;
		cnx->current_seq += (cnx->itsp_mode == mode_host) ? (cnx->current_seq %2) : !(cnx->current_seq %2);
		cnx->current_seq %= 100;
		sequence->number = cnx->current_seq;
	}
	else
		cnx->current_seq = seq_nb;

	sequence->number = cnx->current_seq;

	sequence->race = race;
	if(!pool && (seq_type->data == data_pools) && (seq_type->reason == reason_final))
		strcpy(sequence->pool,"***");
	else
		strcpy(sequence->pool, pool ? pool : "...");
	sequence->interruptible_flag = seq->actions->action && seq->interruptible_flag;
	if(special_data)
		sequence->special_data = special_data;

	if((seq_type->data == data_race_status) && (seq_type->reason != reason_begin) && (cnx->current_race == race)){
		if(cnx->itsp_mode == mode_host) seq_type->reason = reason_begin;
	}

	while((msg = &seq->actions[i++])->message){
		if(msg->way == way){

			if(i > 2 && cnx->itsp_mode == mode_host && way == way_in && msg->message != msg_acknowledge)
				continue;  // exception sur les messages entrants en mode host

			action = malloc(sizeof(itsp_action));
			memset(action, 0, sizeof(itsp_action));
			action->message = msg->message;
			action->type = msg->action;
			if((seq_type->data == data_link) && ((seq_type->reason == reason_host) || (seq_type->reason == reason_remote))){
				action->reason = (cnx->itsp_mode == mode_host) ?
						(action->type == action_send) ? reason_host : reason_remote :
						(action->type == action_send) ? reason_remote : reason_host;
			}else
				action->reason = seq_type->reason;
			fifo_push(sequence->actions, action);
		}
	}

	return sequence;
}
