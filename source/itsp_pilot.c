/*
 * itsp_pilot.c
 *
 *  Created on: 26 oct. 2017
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

int itsp_init(p_s_itsp_cnx cnx){
	p_action_sequence seq = NULL;
	p_itsp_action action = NULL;
	itsp_frame_type fr_type = {0};

	if(cnx_get_phy_status(cnx) < phy_cnx_on){
		TRACE(cnx->log_file, "%s --> itsp_init fail. No physical connection yet!\n", cnx->col_att);
		return FAULT;
	}

	if(fifo_count(cnx->sequences_pipe) || fast_map_count(cnx->working_sequences)){
		TRACE(cnx->log_file, 	"-------------------------------------------------------------\n"
								"--------------|%s --> reseting itsp connection|--------------\n"
								"-------------------------------------------------------------\n", cnx->col_att);
		while((seq = fifo_pull(cnx->sequences_pipe))){
			action = fifo_pull(seq->actions);
			TRACE(cnx->log_file, "%s --> canceling planned sequence nb %d (%c%c-%c)", cnx->col_att, seq->number, seq->data_type, action->message, action->reason);
			do free(action);
			while((action = fifo_pull(seq->actions)));
			free(seq);
		}
		fast_map_data_iterate(seq, cnx->working_sequences){
			action = fifo_pull(seq->actions);
			TRACE(cnx->log_file, "%s --> canceling planned sequence nb %d (%c%c-%c)", cnx->col_att, seq->number, seq->data_type, action->message, action->reason);
			do free(action);
			while((action = fifo_pull(seq->actions)));
		}
		fast_map_clear(cnx->working_sequences);
	}

	cnx->current_seq = 0;

	if(cnx->itsp_mode == mode_host){
		set_fr_type(fr_type, data_link, msg_data, cnx->itsp_mode);
		schedule_sequence(cnx, 0, &fr_type, way_out, 0, NULL, 0, NULL);
		set_fr_type(fr_type, data_config, msg_pending, reason_no_reason);
		schedule_sequence(cnx, 0, &fr_type, way_out, 0, NULL, 0, NULL);
		set_fr_type(fr_type, data_config, msg_request, reason_no_reason);
		schedule_sequence(cnx, 0, &fr_type, way_out, 0, NULL, 0, NULL);
		p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
		p_s_s3k_race p_race = NULL;
		cnx->current_race = 9;//for debug only, remove after
		fast_map_data_iterate(p_race, p_grep->races){
			itsp_msg_reason reason = (p_race->number == cnx->current_race) ? reason_begin : reason_no_reason;
			set_fr_type(fr_type, data_race_status, msg_pending, reason);
			schedule_sequence(cnx, 0, &fr_type, way_out, p_race->number, NULL, 0, NULL);
			set_fr_type(fr_type, data_race_status, msg_request, reason);
			schedule_sequence(cnx, 0, &fr_type, way_out, p_race->number, NULL, 0, NULL);
		}
		set_fr_type(fr_type, data_link, msg_data, reason_end);
		schedule_sequence(cnx, 0, &fr_type, way_out, 0, NULL, 0, NULL);
	}
	cnx_set_logi_status(cnx, itsp_cnx_physic);

	return OK;
}


int itsp_reset_race(p_s_itsp_cnx cnx, int race){
	return OK;
}

int itsp_sell_race(p_s_itsp_cnx cnx, int race, char* pool, int vente_flag){
	return OK;
}

int itsp_name_race(p_s_itsp_cnx cnx, int race){
	return OK;
}

int simulate_bet(p_s_itsp_cnx cnx, int race, char* pool, char* coll_att, int** combo, double bet){
	return OK;
}

#define collect_pool(pool)({\
	p_pool = fast_map_at(p_race->pools, pool);\
	if(p_pool->status == Cancelled_pool){\
		TRACE(cnx->log_file, "%s --> info : no pool collection for \"%s\" of race %d (Cancelled_pool)\n", cnx->col_att, pool, race);\
		continue;\
	}\
	if(p_pool->status == Exchange_pool){\
		TRACE(cnx->log_file, "%s --> Warning : Exchange_pool collection is not implemented (pool \"%s\" of race %d)\n", cnx->col_att, pool, race);\
		continue;\
	}\
	p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);\
	if(p_pool_conf->scan_mode != s_mod_pool){\
		TRACE(cnx->log_file, "%s --> info : no collect_pools for \"%s\" of race %d (use scans to collect)\n", cnx->col_att, pool, race);\
		continue;\
	}\
	p_bets = fast_map_at(p_pool->bets, cnx->col_att);\
	if(p_bets && p_bets->vtf_flag){\
		TRACE(cnx->log_file, "%s --> info : final pools already collected for \"%s\" of race %d\n", cnx->col_att, pool, race);\
		continue;\
	}\
	if((p_pool_conf->receive_gross_pool == when_all_cycles && p_pool_conf->receive_net_pool == when_all_cycles) ||\
	   (final_flag && (p_pool_conf->receive_gross_pool == when_final || p_pool_conf->receive_net_pool == when_final))){\
		set_fr_type(fr_type_s, data_pools, msg_pending, final_flag ? reason_final : reason_no_reason);\
		for(int i = 1; i <= p_pool->dimensions[0]; i++){\
			seg = malloc(sizeof(int)); *seg = i;\
			schedule_sequence(cnx, 0, &fr_type_s, way_out, race, pool, 0, seg);\
		}\
	}\
	if((p_pool_conf->send_gross_pool == when_all_cycles || p_pool_conf->send_net_pool == when_all_cycles) ||\
	   (final_flag && (p_pool_conf->send_gross_pool == when_final || p_pool_conf->send_net_pool == when_final))){\
		set_fr_type(fr_type_r, data_pools, msg_request, final_flag ? reason_final : reason_no_reason);\
		for(int i = 1; i <= p_pool->dimensions[0]; i++){\
			seg = malloc(sizeof(int)); *seg = i;\
			schedule_sequence(cnx, 0, &fr_type_r, way_out, race, pool, 0, seg);\
		}\
	}\
	set_fr_type(fr_type_r, data_pool_total, msg_request, final_flag ? reason_final : reason_no_reason);\
	schedule_sequence(cnx, 0, &fr_type_r, way_out, race, pool, 0, NULL);\
 })

int itsp_collect_pools(p_s_itsp_cnx cnx, int race, char* pool, int final_flag){
	itsp_frame_type fr_type_s = {0}, fr_type_r = {0};

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic){
		TRACE(cnx->log_file, "%s --> collect_pools fail. no pool exchange during init phase!\n", cnx->col_att);
		return FAULT;
	}

	if(!race) race = cnx->current_race;

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
	p_s_s3k_abo_race p_abo_race = fast_map_at(p_att_conf->abo_races, &race);
	p_s_s3k_pool p_pool = NULL;
	p_itsp_str_pool_config p_pool_conf = NULL;
	p_s_s3k_bets p_bets = NULL;
	int * seg = NULL;

	if(!p_race){
		TRACE(cnx->log_file, "%s --> collect_pools fail. race %d not found!\n", cnx->col_att, race);
		return FAULT;
	}

	switch(p_race->status){
	case Open_race :
		if(final_flag)
			TRACE(cnx->log_file, "%s --> Warning : collect final pools while race %d is still open\n", cnx->col_att, race);
		break;
	case Cancelled_race :
		TRACE(cnx->log_file, "%s --> collect_pools fail. race %d canceled!\n", cnx->col_att, race);
		return FAULT;
	case Closed_race :
	case Post_Time_race :
	case Official_race :
	case Unofficial_race :
		if(!final_flag)
			TRACE(cnx->log_file, "%s --> info : collecting final pools for race %d (already Closed)\n", cnx->col_att, race);
		final_flag = 1;
		break;
	}

	if(cnx->itsp_mode == mode_host){
		if(pool)
			do collect_pool(pool);
			while(FAULT);
		else
			sorted_vect_iterate(pool, p_abo_race->abo_pools)
			collect_pool(pool);
	}
	else{
		TRACE(cnx->log_file, "%s --> info : pool collection should be made by host\n", cnx->col_att);
		return FAULT;
	}

	return OK;
}

int itsp_totalize_pool(char* grep, int race, char* pool){
	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = NULL;

	p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris, pool);
	if(!p_pool_typ){
		DEBUG_WARN("unknown pool_code \"%s\"", pool);
		abort();
	}

	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
	if(!p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}
	int total_combos = p_pool->dimensions[0] * p_pool->dimensions[1] * p_pool->dimensions[2];
	if(!total_combos){
		DEBUG_WARN("error in pool \"%s\" --> null dimensions", pool);
		abort();
	}
	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, p_grep->org_att);
	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);

	p_s_s3k_bets total_bets = fast_map_at(p_pool->bets, p_grep->org_att);
	if(!total_bets){
		s_s3k_bets betz = {0};
		betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
		betz.exchange_style = p_pool_conf->scan_mode;
		betz.amounts_type = p_grep->calculation == standard_pool_calculation ? gross_pool_mode : net_pool_mode;
		total_bets = fast_map_insert(p_pool->bets, p_grep->org_att, &betz);
	}

	double total = 0.0, *total_amounts = malloc((total_combos + 1) * sizeof(double));
	memset(total_amounts, 0, (total_combos + 1) * sizeof(double));

	if(!total_bets->amounts){
		total_bets->amounts = malloc((total_combos + 1) * sizeof(double));
		memset(total_bets->amounts, 0, (total_combos + 1) * sizeof(double));
	}

	fast_map_data_iterate(p_att_conf, p_grep->col_configs){

		p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);
		if(!p_pool_conf){
			DEBUG_WARN("no \"%s\" pool_conf for att \"%s\"", pool, p_att_conf->att);
			continue;
		}

		p_s_s3k_abo_race p_abo_race = fast_map_at(p_att_conf->abo_races, &race);
		if(!p_abo_race)
			continue;

		if(!sorted_vect_find(p_abo_race->abo_pools, pool))
			continue;

		p_s_s3k_bets p_bets = fast_map_at(p_pool->bets, p_att_conf->att);
		if(!p_bets)
			continue;

		if(!p_bets->total)
			continue;

		if(!p_bets->amounts)
			DEBUG_WARN("\"%s\" data_pools for att \"%s\" was not collected. ignoring...", pool, p_att_conf->att);

		//printf("att %s : ", p_att_conf->att);
		for(int i = 0 ; i < total_combos; i++){
			if(p_bets->amounts[i]){
				//printf("[%.2f += %.2f] ", total_bets->amounts[i], p_bets->amounts[i]);
				total_amounts[i] += p_bets->amounts[i];
				total += p_bets->amounts[i];

			}
		}
		//printf("\n");

	}


	return OK;
}

int itsp_race_start(char* grep, int race){
	return OK;
}

int itsp_race_end(char* grep, int race){
	return OK;
}

int itsp_race_offic(char* grep, int race){
	return OK;
}

int itsp_send_ppf(char* grep, int race){

	return OK;
}

int itsp_send_payoffs(p_s_itsp_cnx cnx, int race, char* pool, int result_flag){
	itsp_frame_type fr_type = {0};

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic){
		TRACE(cnx->log_file, "%s --> collect_pools fail. no pool exchange during init phase!\n", cnx->col_att);
		return FAULT;
	}

	if(cnx->itsp_mode != mode_host){
		TRACE(cnx->log_file, "%s --> no pool collection for remote peer\n", cnx->col_att);
		return FAULT;
	}


	if(!race) race = cnx->current_race;

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
	p_s_s3k_abo_race p_abo_race = fast_map_at(p_att_conf->abo_races, &race);
	p_s_s3k_pool p_pool = NULL;
	p_itsp_str_pool_config p_pool_conf = NULL;
	itsp_msg_reason reason = reason_no_reason;

	if(!p_race){
		TRACE(cnx->log_file, "%s --> collect_pools fail. race %d not found!\n", cnx->col_att, race);
		return FAULT;
	}

	switch(p_race->status){
	case Open_race :
		TRACE(cnx->log_file, "%s --> Error : no payoffs for opened race %d\n", cnx->col_att, race);
		return FAULT;
	case Cancelled_race :
		TRACE(cnx->log_file, "%s --> Error : no payoffs for canceled race %d\n", cnx->col_att, race);
		return FAULT;
	case Closed_race :
	case Post_Time_race :
	case Unofficial_race :
		reason = reason_final;
		break;
	case Official_race :
		reason = reason_final;
		break;
	}

	if(result_flag){
		if(!p_race->finish){
			TRACE(cnx->log_file, "%s --> Error : no results found for race %d\n", cnx->col_att, race);
			return FAULT;
		}
		if(p_race->finish->offic_flag){
			if(sorted_set_find(p_race->finish->sent_offic_atts, cnx->col_att)){
				TRACE(cnx->log_file, "%s --> Error : offic results already sent for race %d\n", cnx->col_att, race);
				result_flag = 0;
			}
		}

	}

	set_fr_type(fr_type, data_payoffs, msg_data, reason_begin);
	schedule_sequence(cnx, 0, &fr_type, way_out, race, NULL, 0, NULL);

	if(result_flag){
		set_fr_type(fr_type, data_results, msg_data, reason);
		schedule_sequence(cnx, 0, &fr_type, way_out, race, NULL, 0, NULL);
	}

	if(pool){
		p_pool = fast_map_at(p_race->pools, pool);

		if(p_pool->status == Cancelled_pool){
			TRACE(cnx->log_file, "%s --> info : no payoffs collection for \"%s\" of race %d (Cancelled_pool)\n", cnx->col_att, pool, race);
			goto payoff_end;
		}
		if(p_pool->status == Exchange_pool){
			TRACE(cnx->log_file, "%s --> Warning : Exchange_payoffs sending is not implemented (pool \"%s\" of race %d)\n", cnx->col_att, pool, race);
			goto payoff_end;
		}
		p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);
		if(p_pool_conf->scan_mode != s_mod_pool)
			goto payoff_end;

		p_s_fast_map payoff_map = fast_map_at(p_pool->payoffs, cnx->col_att);

		if(!payoff_map || !fast_map_count(payoff_map)){
			TRACE(cnx->log_file, "%s --> info : no payoffs found for pool \"%s\" of race %d\n", cnx->col_att, pool, race);
			goto payoff_end;
		}

		p_s_s3k_payoffs p_payoff = NULL;
		int new_payoffs_flag = 0;
		fast_map_data_iterate(p_payoff, payoff_map){
			if(!p_payoff->sent_flag){
				new_payoffs_flag = 1;
				break;
			}
		}

		if(new_payoffs_flag){
			set_fr_type(fr_type, data_payoffs, msg_data, reason);
			schedule_sequence(cnx, 0, &fr_type, way_out, race, pool, 0, NULL);
		}
	}else{
		sorted_vect_iterate(pool, p_abo_race->abo_pools){

			p_pool = fast_map_at(p_race->pools, pool);

			if(p_pool->status == Cancelled_pool){
				TRACE(cnx->log_file, "%s --> info : no payoffs collection for \"%s\" of race %d (Cancelled_pool)\n", cnx->col_att, pool, race);
				continue;
			}
			if(p_pool->status == Exchange_pool){
				TRACE(cnx->log_file, "%s --> Warning : Exchange_payoffs sending is not implemented (pool \"%s\" of race %d)\n", cnx->col_att, pool, race);
				continue;
			}
			p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);
			if(p_pool_conf->scan_mode != s_mod_pool)
				continue;

			p_s_fast_map payoff_map = fast_map_at(p_pool->payoffs, cnx->col_att);

			if(!payoff_map || !fast_map_count(payoff_map)){
				TRACE(cnx->log_file, "%s --> info : no payoffs for pool \"%s\" of race %d\n", cnx->col_att, pool, race);
				continue;
			}

			p_s_s3k_payoffs p_payoff = NULL;
			int new_payoffs_flag = 0;
			fast_map_data_iterate(p_payoff, payoff_map){
				if(!p_payoff->sent_flag){
					new_payoffs_flag = 1;
					break;
				}
			}

			if(new_payoffs_flag){
				set_fr_type(fr_type, data_payoffs, msg_data, reason);
				schedule_sequence(cnx, 0, &fr_type, way_out, race, pool, 0, NULL);
			}

		}
	}

	payoff_end : set_fr_type(fr_type, data_payoffs, msg_data, reason_end);
	schedule_sequence(cnx, 0, &fr_type, way_out, race, NULL, 0, NULL);

	return OK;
}

int itsp_pool_pay(char* grep, int race, char* pool, int paiem_flag){
	return OK;
}
