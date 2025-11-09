/*
 * itsp_frame_analyser.c
 *
 *  Created on: 10 oct. 2017
 *      Author: f.baccari
 */


 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <math.h>
 #include <stdarg.h>

 #include <i_string.h>
 #include <i_file.h>
 #include <s3k_structs.h>
 #include <s3k_session.h>
 #include <itsp_common.h>
 #include <s3k_commun.h>
 #include <itsp_cnx.h>
 #include <itsp_scheduler.h>
 #include <itsp_pilot.h>

int analyze_itsp_header(p_itsp_header header, p_s_itsp_cnx cnx, p_action_sequence seq, p_itsp_action action){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	itsp_cnx_status_type  lg_st = cnx_get_logi_status(cnx);
	p_s_s3k_race p_race = NULL;
	p_s_s3k_pool_type p_pool_typ = 	NULL;
	p_s_s3k_pool p_pool = NULL;

	//checking step
	if(header->frame_type->data != seq->data_type)
		return itsp_terminate_sequence(cnx, seq, reason_invalid_header, NULL);
	if(header->frame_type->message != action->message){
		if(header->frame_type->message == msg_acknowledge){
			while((p_race = fifo_pull(seq->actions)))
				free(p_race);
			action->message = msg_acknowledge;
		}
		else
			return itsp_terminate_sequence(cnx, seq, reason_invalid_header, NULL);
	}
	if(header->frame_type->reason != action->reason)
		TRACE(cnx->log_file, "Warning : unexpected code_reason (received = %d vs expected = %d)\n",
			header->frame_type->reason, action->reason);

	if(itsp_cnx_config <= lg_st){

		if(strcmp(header->event_code, cnx->grep))
			return itsp_terminate_sequence(cnx, seq, reason_invalid_event, NULL);

		if((cnx->itsp_mode == mode_host) && strcmp(header->source, cnx->col_att))
			return itsp_terminate_sequence(cnx, seq, reason_invalid_source_code, NULL);

		if(strcmp(header->pool_code, seq->pool))
			return itsp_terminate_sequence(cnx, seq, reason_invalid_pool, NULL);

		if(header->race_number != seq->race)
			return itsp_terminate_sequence(cnx, seq, reason_invalid_race, NULL);

		if(itsp_cnx_sync <= lg_st){

			if(!p_grep)
				return itsp_terminate_sequence(cnx, seq, reason_invalid_event, NULL);

			if((cnx->itsp_mode == mode_remote) && strcmp(header->source, p_grep->org_att))
				return itsp_terminate_sequence(cnx, seq, reason_invalid_source_code, NULL);

			if(header->race_number && !(p_race = fast_map_at(p_grep->races, &header->race_number)))
				return itsp_terminate_sequence(cnx, seq, reason_invalid_race, NULL);

			if(p_race && strcmp(header->pool_code, "...") && strcmp(header->pool_code, "***"))
				if(!(p_pool_typ = fast_map_at(liste_paris, header->pool_code)))
					return itsp_terminate_sequence(cnx, seq, reason_invalid_pool, NULL);

			if(itsp_cnx_logic == lg_st){

				p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
				if(!p_att_conf){
					cnx_set_logi_status(cnx, itsp_cnx_config);
					return itsp_terminate_sequence(cnx, seq, reason_invalid_source_code,
							"\"%s\" - (grep \"%s\") --> no att_conf found. resting init phase",
							cnx->col_att, cnx->grep);
				}

				if(p_race && strcmp(header->pool_code, "...") && strcmp(header->pool_code, "***")){

					if(header->frame_type->data == data_will_pay){
						int race_i = header->race_number - p_pool_typ->nb_races + 1;
						if(!(p_race = fast_map_at(p_grep->races, &race_i)))
							return itsp_terminate_sequence(cnx, seq, reason_invalid_race, NULL);
					}

					if(!(p_pool = fast_map_at(p_race->pools, header->pool_code)))
						return itsp_terminate_sequence(cnx, seq, reason_invalid_pool, NULL);

					if(!fast_map_at(p_att_conf->pool_configs, header->pool_code)){
						return itsp_terminate_sequence(cnx, seq, reason_invalid_pool,
								"\"%s\" - (grep \"%s\") --> no \"%s\" pool_conf",
								cnx->col_att, cnx->grep, header->pool_code);
					}
				}
			}

		}

	}

	return OK;
}

int analyze_data_link(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	//checking step
	if((fr->header->frame_type->reason == reason_host) && (cnx->itsp_mode == mode_host))
		return itsp_terminate_sequence(cnx, seq, reason_formate_error, "Received data_link host from remote connection");
	if((fr->header->frame_type->reason == reason_remote) && (cnx->itsp_mode == mode_remote))
		return itsp_terminate_sequence(cnx, seq, reason_formate_error, "Received data_link remote from host connection");

	//processing step
	p_s_s3k_attrib p_att = fast_map_at(liste_attributaires, fr->header->source);
	p_itsp_str_data_link data_ln = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;

	if(cnx_get_logi_status(cnx) < itsp_cnx_config){

		if(!p_att){
			s_s3k_attrib att = {0};
			memcpy(att.code_att,fr->header->source,4);
			att.id_att = att_ctr++;
			att.att_description = NULL;
			att.jet_lag = 0;
			p_att = fast_map_insert(liste_attributaires, fr->header->source, &att);
		}

		if(data_ln){
			strcpy(p_att->itsp_version.document, data_ln->identifier.document);
			p_att->itsp_version.version.revision_number = data_ln->identifier.version.revision_number;
			p_att->itsp_version.version.version_number = data_ln->identifier.version.version_number;
			if(strlen(data_ln->text)){
				p_att->att_description = malloc(strlen(data_ln->text) + 1);
				strcpy(p_att->att_description, data_ln->text);
			}
		}

		TRACE(cnx->log_file, "first datalink from att \"%s\" (protocol : %s %d.%d)\n---> link message : \"%s\"\n",
				p_att->code_att,
				p_att->itsp_version.document,
				p_att->itsp_version.version.version_number,
				p_att->itsp_version.version.revision_number,
				data_ln->text ? data_ln->text : ""
			);


		if((cnx->itsp_mode == mode_host) && strcmp(cnx->col_att, fr->header->source)){
			TRACE(cnx->log_file, "updating remote att code : \"%s\" -> \"%s\"\n", cnx->col_att, fr->header->source);
			strcpy(cnx->col_att, fr->header->source);
		}

		if((cnx->itsp_mode == mode_remote) && strcmp(cnx->grep, fr->header->event_code)){
			TRACE(cnx->log_file, "updating host event code : \"%s\" -> \"%s\"\n", cnx->grep, fr->header->event_code);
			strcpy(cnx->grep, fr->header->event_code);
		}

		cnx_set_logi_status(cnx, itsp_cnx_config);
		if(cnx->itsp_mode == mode_host)
			TRACE(cnx->log_file, "begin of init phase for col_att \"%s\"\n", cnx->col_att);

	}

	if(
		(fr->header->frame_type->reason == reason_end)	&&
		(itsp_cnx_config <= cnx_get_logi_status(cnx))	&&
		(cnx_get_logi_status(cnx) < itsp_cnx_logic)
	  )
	{
		cnx_set_logi_status(cnx, itsp_cnx_logic);
		if(cnx->itsp_mode == mode_host){
			TRACE(cnx->log_file, "end of init phase for col_att \"%s\"\n", cnx->col_att);
			//itsp_collect_pools(cnx, 0, NULL, 1);
			//itsp_send_payoffs(cnx, 1, NULL, 1);
			int leg = 2;
			int race = 5;
			char* pool = "P04";

			p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
			p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
			p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
			p_s_s3k_bets bets = fast_map_at(p_pool->bets, cnx->col_att);
			p_s_s3k_bet_scan ps_scan = fast_map_at(bets->scans, &leg);
			void* sp_dt = malloc(sizeof(s_s3k_bet_scan));
			memcpy(sp_dt, ps_scan, sizeof(s_s3k_bet_scan));


			itsp_frame_type t = {data_scan, msg_request, reason_begin};
			schedule_sequence(cnx, 0, &t, way_out, race, pool, 0, sp_dt);
		}
	}

	return ret_ok;
}

int analyze_data_alert(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	//processing step
	p_itsp_str_alert data_alert = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;

	if(data_alert)
		switch(data_alert->type){
		case ' ' :
		default :
			if(data_alert->message)
				TRACE(cnx->log_file, "alert message (type '%c') receved from \"%s\" : <<%s>>\n",
					data_alert->type, fr->header->source, data_alert->message);
			break;
		}
	else
		if(fr->header->frame_type->message == msg_data)
			TRACE(cnx->log_file, "Keep alive message received from \"%s\"\n", fr->header->source);

	return ret_ok;
}

int analyze_data_config(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	p_itsp_str_data_config 	dt_cfg = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
	if(!dt_cfg)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "emty data_config frame");

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = NULL;
	p_s_s3k_att_conf p_col_conf = NULL;
	p_s_s3k_abo_race p_abo_race = NULL;
	p_itsp_str_pool_config p_pool_conf = NULL;
	int i = 0;

	if(cnx_get_logi_status(cnx) != itsp_cnx_config)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "data_config exchange can only be made during init step");

	//checking step
	if(cnx->itsp_mode == mode_host){

		if(!p_grep)
			return itsp_terminate_sequence(cnx, seq, reason_invalid_event, NULL);

		if(p_grep->calculation != dt_cfg->calculation)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "The <calculation> value sent by a remote system dose not match the host value");

		if(p_grep->close_bet_delay != dt_cfg->close_bet_delay)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "The <close bet delay> value sent by a remote system dose not match the host value");

		if(p_grep->close_cancel_delay != dt_cfg->close_cancel_delay)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "The <close cancel delay> value sent by a remote system dose not match the host value");

		if(p_grep->date_time != c_date_time_tm(&dt_cfg->date, &dt_cfg->time))
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "The <date & time> values sent by a remote system dose not match the host values");

		for( i = 0; i < dt_cfg->race_list.races; i++)
			if(!fast_map_at(p_grep->races, &dt_cfg->race_list.race[i]))
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "race_list error : race %d dose not exist", dt_cfg->race_list.race[i]);

		if(!(p_col_conf = fast_map_at(p_grep->col_configs, fr->header->source))){
			s_s3k_att_conf col_conf = {0};
			memcpy(col_conf.att, fr->header->source, 4);
			col_conf.abo_races = fast_map_init(sizeof(int), sizeof(s_s3k_abo_race), int_key_com);
			col_conf.pool_configs = fast_map_init(4, sizeof(itsp_str_pool_config), string_key_com);
			p_col_conf = fast_map_insert(p_grep->col_configs, fr->header->source, &col_conf);
		}


		reiterate_1 : fast_map_data_iterate(p_abo_race, p_col_conf->abo_races){
			for(i = 0; i < dt_cfg->race_list.races;i++)
				if(dt_cfg->race_list.race[i] == p_abo_race->number)
					break;
			if(i == dt_cfg->race_list.races){
				TRACE(cnx->log_file, "att \"%s\" -> canceled subscription for race nb %d\n", fr->header->source, p_abo_race->number);
				fast_map_remove(p_col_conf->abo_races, &p_abo_race->number);
				goto reiterate_1;
			}
		}

		for(i = 0; i < dt_cfg->race_list.races; i++){
			if(!fast_map_at(p_col_conf->abo_races, &dt_cfg->race_list.race[i])){
				if(fast_map_at(p_grep->races, &dt_cfg->race_list.race[i])){
					TRACE(cnx->log_file, "att \"%s\" -> new subscription for race nb %d\n", fr->header->source, dt_cfg->race_list.race[i]);
					s_s3k_abo_race abo_race = {0};
					abo_race.number = dt_cfg->race_list.race[i];
					abo_race.abo_pools = sorted_set_init(4, string_key_com);
					fast_map_insert(p_col_conf->abo_races, &abo_race.number, &abo_race);
				}
				else
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "grep race_list error : race %d dose not exist", dt_cfg->race_list.race[i]);
			}
		}

		for(i = 0; i < dt_cfg->nb_pools; i++){
			if(!(p_pool_conf = fast_map_at(p_col_conf->pool_configs, dt_cfg->pool_configs[i].pool_code))){
				if(fast_map_at(liste_paris, dt_cfg->pool_configs[i].pool_code)){
					TRACE(cnx->log_file, "att \"%s\" -> new \"%s\" pool config\n", fr->header->source, dt_cfg->pool_configs[i].pool_code);
					fast_map_insert(p_col_conf->pool_configs, dt_cfg->pool_configs[i].pool_code, &dt_cfg->pool_configs[i]);
				}
				else
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "pool conf error : pool %s dose not exist", dt_cfg->pool_configs[i].pool_code);
			}else{

				if(p_pool_conf->exchange_part != dt_cfg->pool_configs[i].exchange_part)
					p_pool_conf->exchange_part = dt_cfg->pool_configs[i].exchange_part;

				if(p_pool_conf->scan_mode != dt_cfg->pool_configs[i].scan_mode)
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "pool conf error : pool \"%s\" scan_mode should be '%c'",
							dt_cfg->pool_configs[i].pool_code, p_pool_conf->exchange_part);

				if(double_key_com(&p_pool_conf->pool_unit, &dt_cfg->pool_configs[i].pool_unit)){
					TRACE(cnx->log_file, "att \"%s\" -> \"%s\" pool_unit update (%f -> %f)\n",
							fr->header->source, dt_cfg->pool_configs[i].pool_code, p_pool_conf->pool_unit, dt_cfg->pool_configs[i].pool_unit);
					p_pool_conf->pool_unit = dt_cfg->pool_configs[i].pool_unit;
				}

				if(double_key_com(&p_pool_conf->min_payoff, &dt_cfg->pool_configs[i].min_payoff)){
					TRACE(cnx->log_file, "att \"%s\" -> \"%s\" min_payoff update (%f -> %f)\n",
							fr->header->source, dt_cfg->pool_configs[i].pool_code, p_pool_conf->min_payoff, dt_cfg->pool_configs[i].min_payoff);
					p_pool_conf->min_payoff = dt_cfg->pool_configs[i].min_payoff;
				}

				if(double_key_com(&p_pool_conf->Break, &dt_cfg->pool_configs[i].Break)){
					TRACE(cnx->log_file, "att \"%s\" -> \"%s\" Break amount update (%f -> %f)\n",
							fr->header->source, dt_cfg->pool_configs[i].pool_code, p_pool_conf->Break, dt_cfg->pool_configs[i].Break);
					p_pool_conf->Break = dt_cfg->pool_configs[i].Break;
				}

				if(memcmp(&p_pool_conf->send_gross_pool, &dt_cfg->pool_configs[i].send_gross_pool, 7 * sizeof(when_value_type))){
					char when_old[8] = {0}, when_new[8] = {0};
					when_value_type *ptr_old = &p_pool_conf->send_gross_pool,
									*ptr_new = &dt_cfg->pool_configs[i].send_gross_pool;
					int i = 7;
					while(i--){
						when_old[i] = ptr_old[i];
						when_new[i] = ptr_new[i];
						ptr_old[i] = ptr_new[i];
					}
					TRACE(cnx->log_file, "att \"%s\" -> \"%s\" when_values update (%s -> %s)\n",
							fr->header->source, dt_cfg->pool_configs[i].pool_code, when_old, when_new);
				}

			}
		}

	}else{
		if(!p_grep){
			s_s3k_grep grep = {0};
			grep.nu_grep = grep_ctr++;
			memcpy(grep.name, fr->header->event_code, 4);
			memcpy(grep.org_att, fr->header->source, 4);
			grep.races = fast_map_init(sizeof(int), sizeof(s_s3k_race), int_key_com);
			grep.col_configs = fast_map_init(4, sizeof(s_s3k_att_conf), string_key_com);
			p_grep = fast_map_insert(liste_reunions, fr->header->event_code, &grep);
			TRACE(cnx->log_file, "new grep from org \"%s\" -> event_code : %s\n", fr->header->source, fr->header->event_code);
		}

		if(p_grep->date_time != c_date_time_tm(&dt_cfg->date, &dt_cfg->time)){
			struct tm* dt = long_2_tm(&p_grep->date_time);
			TRACE(cnx->log_file, "updating grep date_time (%02d/%02d/%04d %02d:%02d:%02d -> W%02d/%02d/%04d %02d:%02d:%02d)\n",
					dt->tm_mday,
					dt->tm_mon + 1,
					dt->tm_year + 1900,
					dt->tm_hour,
					dt->tm_min,
					dt->tm_sec,
					dt_cfg->date.day,
					dt_cfg->date.month,
					dt_cfg->date.year,
					dt_cfg->time.hour,
					dt_cfg->time.minutes,
					dt_cfg->time.seconds
					);
			p_grep->date_time = c_date_time_tm(&dt_cfg->date, &dt_cfg->time);
		}

		if(p_grep->performance != dt_cfg->performance){
			TRACE(cnx->log_file, "updating grep performance (%d ->%d)\n",
					p_grep->performance,
					dt_cfg->performance);
			p_grep->performance = dt_cfg->performance;
		}

		if(p_grep->calculation != dt_cfg->calculation){
			TRACE(cnx->log_file, "updating grep calculation (%d ->%d)\n",
					p_grep->calculation,
					dt_cfg->calculation);
			p_grep->calculation = dt_cfg->calculation;
		}

		if(p_grep->close_bet_delay != dt_cfg->close_bet_delay){
			TRACE(cnx->log_file, "updating grep close_bet_delay (%d ->%d)\n",
					p_grep->close_bet_delay,
					dt_cfg->close_bet_delay);
			p_grep->close_bet_delay = dt_cfg->close_bet_delay;
		}

		if(p_grep->close_cancel_delay != dt_cfg->close_cancel_delay){
			TRACE(cnx->log_file, "updating grep close_cancel_delay (%d ->%d)\n",
					p_grep->close_cancel_delay,
					dt_cfg->close_cancel_delay);
			p_grep->close_cancel_delay = dt_cfg->close_cancel_delay;
		}

		if(!(p_col_conf = fast_map_at(p_grep->col_configs, &cnx->col_att))){
			s_s3k_att_conf col_conf = {0};
			memcpy(col_conf.att, cnx->col_att, 4);
			col_conf.abo_races = fast_map_init(sizeof(int), sizeof(s_s3k_abo_race), int_key_com);
			col_conf.pool_configs = fast_map_init(4, sizeof(itsp_str_pool_config), string_key_com);
			p_col_conf = fast_map_insert(p_grep->col_configs, cnx->col_att, &col_conf);
			TRACE(cnx->log_file, "adding \"%s\" col_conf to grep \"%s\"\n", cnx->col_att, cnx->col_att);
		}

		for( i = 0; i < dt_cfg->race_list.races; i++){
			if(!fast_map_at(p_col_conf->abo_races, &dt_cfg->race_list.race[i])){
				if(!fast_map_at(p_grep->races, &dt_cfg->race_list.race[i])){
					s_s3k_race race = {0};
					race.number = dt_cfg->race_list.race[i];
					race.runners = fast_map_init(sizeof(int), sizeof(s_s3k_runner), int_key_com);
					race.pools =  fast_map_init(4, sizeof(s_s3k_pool), string_key_com);
					race.brackets = sorted_vect_init(sizeof(s_s3k_bracket), int_key_com);
					fast_map_insert(p_grep->races, &race.number, &race);
					TRACE(cnx->log_file, "new race nb %d for grep \"%s\"\n", dt_cfg->race_list.race[i], p_grep->name);
				}
				s_s3k_abo_race abo_race = {0};
				abo_race.number = dt_cfg->race_list.race[i];
				abo_race.abo_pools = sorted_set_init(4, string_key_com);
				fast_map_insert(p_col_conf->abo_races, &abo_race.number, &abo_race);
				TRACE(cnx->log_file, "new offer for race nb %d\n", dt_cfg->race_list.race[i]);
			}
		}


		reiterate_2 : fast_map_data_iterate(p_abo_race, p_col_conf->abo_races){
			for(i = 0; i < dt_cfg->race_list.races;i++)
				if(dt_cfg->race_list.race[i] == p_abo_race->number)
					break;
			if(i == dt_cfg->race_list.races){
				TRACE(cnx->log_file, "canceled offer for nb %d\n", p_abo_race->number);
				fast_map_remove(p_col_conf->abo_races, &p_abo_race->number);
				goto reiterate_2;
			}
		}

		reiterate_3 :fast_map_data_iterate(p_race, p_grep->races){
			for(i = 0; i < dt_cfg->race_list.races;i++)
				if(dt_cfg->race_list.race[i] == p_race->number)
					break;
			if(i == dt_cfg->race_list.races){
				TRACE(cnx->log_file, "canceled race nb %d\n", p_race->number);
				fast_map_remove(p_grep->races, &p_race->number);
				goto reiterate_3;
			}
		}

		for(i = 0; i < dt_cfg->nb_pools; i++){
			if(!(p_pool_conf = fast_map_at(p_col_conf->pool_configs, dt_cfg->pool_configs[i].pool_code))){
				if(fast_map_at(liste_paris, dt_cfg->pool_configs[i].pool_code)){
					TRACE(cnx->log_file, "new host pool config for \"%s\"\n", dt_cfg->pool_configs[i].pool_code);
					fast_map_insert(p_col_conf->pool_configs, dt_cfg->pool_configs[i].pool_code, &dt_cfg->pool_configs[i]);
				}
			}else{

				if(p_pool_conf->exchange_part != dt_cfg->pool_configs[i].exchange_part)
					p_pool_conf->exchange_part = dt_cfg->pool_configs[i].exchange_part;

				if(p_pool_conf->scan_mode != dt_cfg->pool_configs[i].scan_mode){
					TRACE(cnx->log_file, "integrating host \"%s\" scan_mode (%c -> %c)\n",
							dt_cfg->pool_configs[i].pool_code, p_pool_conf->scan_mode, dt_cfg->pool_configs[i].scan_mode);
					p_pool_conf->scan_mode = dt_cfg->pool_configs[i].scan_mode;
				}

				if(double_key_com(&p_pool_conf->pool_unit, &dt_cfg->pool_configs[i].pool_unit)){
					TRACE(cnx->log_file, "integrating  host \"%s\" pool_unit (%f -> %f)\n",
							dt_cfg->pool_configs[i].pool_code, p_pool_conf->pool_unit, dt_cfg->pool_configs[i].pool_unit);
					p_pool_conf->pool_unit = dt_cfg->pool_configs[i].pool_unit;
				}

				if(double_key_com(&p_pool_conf->min_payoff, &dt_cfg->pool_configs[i].min_payoff)){
					TRACE(cnx->log_file, "integrating  host \"%s\" min_payoff (%f -> %f)\n",
							dt_cfg->pool_configs[i].pool_code, p_pool_conf->min_payoff, dt_cfg->pool_configs[i].min_payoff);
					p_pool_conf->min_payoff = dt_cfg->pool_configs[i].min_payoff;
				}

				if(double_key_com(&p_pool_conf->Break, &dt_cfg->pool_configs[i].Break)){
					TRACE(cnx->log_file, "integrating  host \"%s\" Break amount (%f -> %f)\n",
							dt_cfg->pool_configs[i].pool_code, p_pool_conf->Break, dt_cfg->pool_configs[i].Break);
					p_pool_conf->Break = dt_cfg->pool_configs[i].Break;
				}

				if(memcmp(&p_pool_conf->send_gross_pool, &dt_cfg->pool_configs[i].send_gross_pool, 7 * sizeof(when_value_type))){
					char when_old[8] = {0}, when_new[8] = {0};
					when_value_type *ptr_old = &p_pool_conf->send_gross_pool,
									*ptr_new = &dt_cfg->pool_configs[i].send_gross_pool;
					int i = 7;
					while(i--){
						when_old[i] = ptr_old[i];
						when_new[i] = ptr_new[i];
						ptr_old[i] = ptr_new[i];
					}
					TRACE(cnx->log_file, "integrating  host \"%s\" when_values (%s -> %s)\n",
							dt_cfg->pool_configs[i].pool_code, when_old, when_new);
				}

			}
		}
	}

	cnx_set_logi_status(cnx, itsp_cnx_sync);

	return ret_ok;
}

int analyze_data_race_status(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_itsp_str_data_race_status	dt_rs = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_att_conf p_att_conf = NULL;
	p_s_s3k_abo_race p_abo_race = NULL;
	p_s_s3k_pool p_pool_def = NULL;
	int i = 0, nb_runners = 0;

	if(cnx_get_logi_status(cnx) < itsp_cnx_sync)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate, "data_config exchange should be done first");

	if(fr->header->frame_type->reason == reason_end){
			if(cnx->itsp_mode == mode_host)
				p_race->start = c_time(&fr->header->time);
			else
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
						"\"%s\" -> Error : race %d (grep \"%s\") start should be made by host", cnx->col_att, p_race->number, p_grep->name);
	}

	if(dt_rs){
		if(!(p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att)))
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" -> Error : no att_config found in grep \"%s\"", cnx->col_att, p_grep->name);

		if(!(p_abo_race = fast_map_at(p_att_conf->abo_races, &fr->header->race_number)))
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> not subscribed", cnx->col_att, p_race->number, p_grep->name);

		if(cnx->itsp_mode == mode_remote){

			if(p_race->status != dt_rs->status){
				TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> race_status change ('%c' --> '%c') => updating\n",
						cnx->col_att, p_race->number, p_grep->name, p_race->status, dt_rs->status);
				p_race->status = dt_rs->status;
			}

			if(p_race->display != dt_rs->display){
				TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> race_display change ('%c' --> '%c') => updating\n",
						cnx->col_att, p_race->number, p_grep->name, p_race->display, dt_rs->display);
				p_race->display = dt_rs->display;
			}

			if(p_race->start != c_time(&dt_rs->post_time)){
				char buff[256] = {0}, *buff_ptr = buff;
				buff_ptr = sprintf_date_time(buff_ptr, p_race->start, 't');
				buff_ptr += sprintf(buff_ptr, " --> ");
				buff_ptr = sprintf_date_time(buff_ptr, c_time(&dt_rs->post_time), 't');
				TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> post_time change (%s) => updating\n",
						cnx->col_att, p_race->number, p_grep->name, buff);
				p_race->start = c_time(&dt_rs->post_time);
			}

			int nb_runners = fast_map_count(p_race->runners);
			if(nb_runners && nb_runners != dt_rs->runners){
				TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> nb runners change (%d --> %d) => clearing existing runners\n",
						cnx->col_att, p_race->number, p_grep->name, nb_runners, dt_rs->runners);
				fast_map_clear(p_race->runners);
			}

			for(i = 1; i < dt_rs->runners + 1; i++){
				p_s_s3k_runner p_runner = fast_map_at(p_race->runners, &i);
				if(!p_runner){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> adding new runner nb %d\n",
							cnx->col_att, p_race->number, p_grep->name, i);
					s_s3k_runner runner = {0};
					runner.number = i;
					p_runner = fast_map_insert(p_race->runners, &i, &runner);
				}
				if(p_runner->status != dt_rs->runner_status[i - 1]){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> updating runner %d status '%c' -> '%c'\n",
							cnx->col_att, p_race->number, p_grep->name, i, p_runner->status, dt_rs->runner_status[i - 1]);
					p_runner->status = dt_rs->runner_status[i - 1];
				}
			}

			reiterate_1 : fast_map_data_iterate(p_pool_def, p_race->pools){
				for (i = 0; i < dt_rs->pools; i++)
					if(!strcmp(dt_rs->pool_def[i].pool_code, p_pool_def->code))
						break;

				if(i == dt_rs->pools){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> pool \"%s\" is no longer in offer => deleting\n",
								cnx->col_att, p_race->number, p_grep->name, p_pool_def->code);
					fast_map_remove(p_race->pools, p_pool_def->code);
					sorted_set_remove(p_abo_race->abo_pools, p_pool_def->code);
					goto reiterate_1;
				}
			}

			for (i = 0; i < dt_rs->pools; i++){
				p_pool_def = fast_map_at(p_race->pools, dt_rs->pool_def[i].pool_code);
				if(!p_pool_def){
					TRACE(cnx->log_file, "\"%s\" -> adding new pool \"%s\" to race %d (grep \"%s\")\n",
							cnx->col_att, dt_rs->pool_def[i].pool_code, p_race->number, p_grep->name);
					s_s3k_pool pool_def = {0};
					strcpy(pool_def.code, dt_rs->pool_def[i].pool_code);
					pool_def.race_list = malloc(2 * sizeof(int));
					pool_def.race_list[0] = p_race->number;
					pool_def.race_list[0] = 0;
					pool_def.bets = fast_map_init(4, sizeof(s_s3k_bets), string_key_com);
					pool_def.payoffs = fast_map_init(4, sizeof(s_fast_map), string_key_com);
					pool_def.will_pay = fast_map_init(4, sizeof(s_fast_map), string_key_com);
					p_pool_def = fast_map_insert(p_race->pools, pool_def.code, &pool_def);
				}
				sorted_set_insert(p_abo_race->abo_pools, p_pool_def->code);

				if(mumeric_range_cmp(dt_rs->pool_def[i].race_list, &p_pool_def->race_list[1])){
					char buff[1024] = {0}, *buff_ptr = buff;
					buff_ptr = write_numeric_range(buff_ptr, &p_pool_def->race_list[1]);
					buff_ptr += sprintf(buff_ptr, " -> ");
					buff_ptr = write_numeric_range(buff_ptr, dt_rs->pool_def[i].race_list);
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> pool \"%s\" racelist change (%s)=> updating\n",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, buff);
					int j = numeric_range_cardinal(dt_rs->pool_def[i].race_list);
					p_pool_def->race_list = malloc((j + 2) * sizeof(int));
					p_pool_def->race_list[j + 1] = 0;
					while(j--)
						p_pool_def->race_list[j + 1] = dt_rs->pool_def[i].race_list[j];
					p_pool_def->race_list[0] = fr->header->race_number;
				}

				if(double_key_com(&p_pool_def->min_bet, &dt_rs->pool_def[i].min_bet)){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> pool \"%s\" min_bet change (%f --> %f)=> updating\n",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, p_pool_def->min_bet, dt_rs->pool_def[i].min_bet);
					p_pool_def->min_bet = dt_rs->pool_def[i].min_bet;
				}

				if(double_key_com(&p_pool_def->net_addin, &dt_rs->pool_def[i].net_addin)){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> pool \"%s\" net_addin change (%f --> %f)=> updating\n",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, p_pool_def->net_addin, dt_rs->pool_def[i].net_addin);

					p_pool_def->net_addin = dt_rs->pool_def[i].net_addin;
				}

				if(double_key_com(&p_pool_def->gross_addin, &dt_rs->pool_def[i].gross_addin)){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> pool \"%s\" gross_addin change (%f --> %f)=> updating\n",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, p_pool_def->gross_addin, dt_rs->pool_def[i].gross_addin);

					p_pool_def->gross_addin = dt_rs->pool_def[i].gross_addin;
				}

			}

			for (i = 0; i < dt_rs->scratch_pools; i++){
				p_s_s3k_pool pool_i = fast_map_at(p_race->pools, dt_rs->scratch_info[i].pool_code);
				if(!pool_i)
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> scratch info pool \"%s\" not found",
							cnx->col_att, p_race->number, p_grep->name, dt_rs->scratch_info[i].pool_code);
				if(combo_cmp_func(pool_i->scratch_info, dt_rs->scratch_info[i].pool_runners)){
					char buff1[256] = {0}, buff2[256] = {0};
					write_combo(buff1, pool_i->scratch_info);
					write_combo(buff2, dt_rs->scratch_info[i].pool_runners);
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> pool \"%s\" scratch_info change (%s --> %s)=> updating\n",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, buff1, buff2);
					if(pool_i->scratch_info) free_combo(pool_i->scratch_info);
					pool_i->scratch_info = clone_combo(dt_rs->scratch_info[i].pool_runners);
					//il y a un travail a faire pour toutes les epreuves du paris au niveau du runner status
				}

			}
		}
		else{

			if(p_race->status != dt_rs->status){
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
						"\"%s\" - race %d (grep \"%s\") -> wrong race status ('%c' vs '%c')",
						cnx->col_att, p_race->number, p_grep->name, p_race->status, dt_rs->status);
			}

			if(p_race->display != dt_rs->display){
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
						"\"%s\" - race %d (grep \"%s\") -> wrong display ('%c' vs '%c')",
						cnx->col_att, p_race->number, p_grep->name, p_race->display, dt_rs->display);
			}

			nb_runners = fast_map_count(p_race->runners);
			if(nb_runners && nb_runners != dt_rs->runners){
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
						"\"%s\" - race %d (grep \"%s\") -> wrong nb runners (%d vs %d)",
						cnx->col_att, p_race->number, p_grep->name, nb_runners, dt_rs->runners);
			}

			for(i = 1; i < dt_rs->runners + 1; i++){
				p_s_s3k_runner p_runner = fast_map_at(p_race->runners, &i);
				if(p_runner->status != dt_rs->runner_status[i - 1]){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> wrong runner %d status ('%c' vs '%c')",
							cnx->col_att, p_race->number, p_grep->name, p_runner->status, dt_rs->runner_status[i - 1]);
				}

			}

			if(p_race->start != c_time(&dt_rs->post_time)){
				char buff[256] = {0}, *buff_ptr = buff;
				buff_ptr = sprintf_date_time(buff_ptr, c_time(&dt_rs->post_time), 't');
				buff_ptr += sprintf(buff_ptr, ". should be ");
				buff_ptr = sprintf_date_time(buff_ptr, p_race->start, 't');
				TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> poste_time received from%s\n",
												cnx->col_att, p_race->number, p_grep->name, buff);

			}

			nb_runners = fast_map_count(p_race->runners);
			if(nb_runners && nb_runners != dt_rs->runners){
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
						"\"%s\" - race %d (grep \"%s\") -> wrong nb runners (%d). should be %d",
						cnx->col_att, p_race->number, p_grep->name, dt_rs->runners, nb_runners);
			}

			for(i = 1; i < dt_rs->runners + 1; i++){
				p_s_s3k_runner p_runner = fast_map_at(p_race->runners, &i);
				if(p_runner->status != dt_rs->runner_status[i - 1]){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> runner %d status did not update correctly '%c'. should be '%c'",
							cnx->col_att, p_race->number, p_grep->name, i, dt_rs->runner_status[i - 1], p_runner->status);
				}
			}

			char* abo_pool = NULL;
			reiterate_2 : sorted_set_iterate(abo_pool, p_abo_race->abo_pools){
				for (i = 0; i < dt_rs->pools; i++)
					if(!strcmp(dt_rs->pool_def[i].pool_code, abo_pool))
						break;

				if(i == dt_rs->pools){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> no subscription for pool \"%s\"\n",
								cnx->col_att, p_race->number, p_grep->name, abo_pool);
					sorted_set_remove(p_abo_race->abo_pools, abo_pool);
					goto reiterate_2;
				}

			}

			for (i = 0; i < dt_rs->pools; i++){

				if(!sorted_set_find(p_abo_race->abo_pools, dt_rs->pool_def[i].pool_code)){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" -> can't subscribe to pool \"%s\" in race %d (grep \"%s\")",
							cnx->col_att, dt_rs->pool_def[i].pool_code, p_race->number, p_grep->name);
				}

				p_pool_def = fast_map_at(p_race->pools, dt_rs->pool_def[i].pool_code);
				if(!p_pool_def){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" -> no pool \"%s\" offer in race %d (grep \"%s\")",
							cnx->col_att, dt_rs->pool_def[i].pool_code, p_race->number, p_grep->name);
				}

				if(mumeric_range_cmp(dt_rs->pool_def[i].race_list, &p_pool_def->race_list[1])){
					char buff[1024] = {0}, *buff_ptr = buff;
					buff_ptr = write_numeric_range(buff_ptr, &p_pool_def->race_list[1]);
					buff_ptr += sprintf(buff_ptr, " should be ");
					buff_ptr = write_numeric_range(buff_ptr, dt_rs->pool_def[i].race_list);
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> pool \"%s\" racelist did not update correctly (%s)",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, buff);
				}

				if(double_key_com(&p_pool_def->min_bet, &dt_rs->pool_def[i].min_bet)){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> pool \"%s\" min_bet did not update correctly (%f should be %f)",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, p_pool_def->min_bet, dt_rs->pool_def[i].min_bet);
				}

				if(double_key_com(&p_pool_def->net_addin, &dt_rs->pool_def[i].net_addin)){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> pool \"%s\" net_addin did not update correctly (%f should be %f)",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, p_pool_def->net_addin, dt_rs->pool_def[i].net_addin);
				}

				if(double_key_com(&p_pool_def->gross_addin, &dt_rs->pool_def[i].gross_addin)){
					return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
							"\"%s\" - race %d (grep \"%s\") -> pool \"%s\" gross_addin did not update correctly (%f should be %f)",
							cnx->col_att, p_race->number, p_grep->name, p_pool_def->code, p_pool_def->gross_addin, dt_rs->pool_def[i].gross_addin);
				}

			}

		}

	}

	return OK;
}

int analyze_data_pools(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no data_pool exchange during init phase",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	if(!fr->header->race_number)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> data_pool frame with numm race number",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_itsp_str_data_pools	dt_pool = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, fr->header->pool_code);
	p_s_s3k_bets bets = NULL;

	if(!strcmp(fr->header->pool_code, "***") && (fr->header->frame_type->reason == reason_final)){
		if(cnx->itsp_mode == mode_host){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> PPF received",
				cnx->col_att, p_race->number, p_grep->name);
			itsp_collect_pools(cnx, fr->header->race_number, NULL, fr->header->frame_type->reason);
		}
		else
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> Warning : PPF received from !!host!!",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name);
		return OK;
	}

	if(!p_pool)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> data_pool frame with unknown pool \"%s\"",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, fr->header->pool_code);

	if(cnx->current_race != fr->header->race_number)
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> Warning : this race is not the current race",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name);

	if(p_pool_conf->scan_mode != s_mod_pool){
		TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> Warning : no \"%s\" data_pools exchange with scan_mode = '%c'",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name, fr->header->pool_code, p_pool_conf->scan_mode);
	}

	int segz = p_pool->dimensions[0], rowz = p_pool->dimensions[1], columnz = p_pool->dimensions[2], i = 0, j = 0, offset = 0;
	double seg_total = 0.0;

	switch(fr->header->frame_type->message){
	case msg_pending :
	case msg_request :
		if((cnx->itsp_mode == mode_host) &&
		   (p_grep->calculation == standard_pool_calculation) &&
		   (dt_pool->pool_header.pool_mode != gross_pool_mode))
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> Warning : grep calculation = '%c' vs received pool_mode = '%c'",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name, fr->header->pool_code, p_pool_conf->scan_mode);
		seq->special_data = malloc(sizeof(int));
		*(int*)seq->special_data = dt_pool->pool_header.segment;
		break;
	case msg_data :
		if(dt_pool->pool_header.segment != *(int*)seq->special_data)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
						"\"%s\" - race %d (grep \"%s\") -> Warning : data_pool for \"%s\" received segment is %d, expected %s",
						cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name,
								fr->header->pool_code, dt_pool->pool_header.segment, *(int*)seq->special_data);

		if(!(bets = fast_map_at(p_pool->bets, cnx->col_att))){
			s_s3k_bets betz = {0};
			betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
			betz.exchange_style = p_pool_conf->scan_mode;
			betz.amounts_type = dt_pool->pool_header.pool_mode;
			bets = fast_map_insert(p_pool->bets, cnx->col_att, &betz);
		}

		if(dt_pool->pool_data.columns){

			if(!bets->amounts){
				bets->amounts = malloc(((segz * rowz * columnz) + 1) * sizeof(double));
				memset(bets->amounts, 0, ((segz * rowz * columnz) + 1) * sizeof(double));
			}

			if(
					(dt_pool->pool_header.segments != p_pool->dimensions[0]) ||
					(dt_pool->pool_data.rows != p_pool->dimensions[1]) ||
					(dt_pool->pool_data.columns != p_pool->dimensions[2])
			  )
				return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
								"\"%s\" - race %d (grep \"%s\") -> Warning : received \"%s\" data_pool"
								" matrix dimensions error (%dx%dx%d should be %dx%dx%d)",
								cnx->itsp_mode == mode_host ? cnx->col_att : "host",
								p_race->number, p_grep->name,
								fr->header->pool_code,
								dt_pool->pool_header.segments,
								dt_pool->pool_data.rows,
								dt_pool->pool_data.columns,
								p_pool->dimensions[0],
								p_pool->dimensions[1],
								p_pool->dimensions[2]);


			offset = (dt_pool->pool_header.segment - 1) * rowz * columnz;
			for(i = 0 ; i < rowz; i++) for(j = 0; j < columnz; j++)
				bets->amounts[offset + (i * columnz) + j] = dt_pool->pool_data.matrix_data[i][j];
		}
		bets->total = dt_pool->pool_data.total;
		bets->net_total = dt_pool->pool_data.net_total;

		if(
			cnx->itsp_mode == mode_host &&
			fr->header->frame_type->reason == reason_final &&
			dt_pool->pool_header.segment == dt_pool->pool_header.segments
		  )
			bets->vtf_flag = 1;

		break;
	case msg_acknowledge :
		bets = fast_map_at(p_pool->bets, cnx->col_att);

		if(bets->amounts){
			offset = (*(int*)seq->special_data - 1) * rowz * columnz;
			for(i = 0 ; i < rowz; i++) for(j = 0; j < columnz; j++)
				seg_total +=bets->amounts[offset + (i * columnz) + j];
		}

		if(isequal_double(seg_total, dt_pool->pool_data.segment_total, TOLERANCE))
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> Warning : \"%s\" data_pool segment total mismatch (seg = %d). %f should be %f",
								cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name, p_pool->code, *(int*)seq->special_data,
										dt_pool->pool_data.segment_total, seg_total);
		break;
	}

	return OK;
}

int analyze_data_totals(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no data_total exchange during init phase",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_itsp_str_data_totals	dt_tot = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, cnx->col_att);
	if(!bets){
		s_s3k_bets betz = {0};
		p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
		p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, fr->header->pool_code);
		betz.exchange_style = pool_conf->scan_mode;
		betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
		fast_map_insert(p_pool->bets, fr->header->source, &betz);
		bets = fast_map_at(p_pool->bets, cnx->col_att);
	}

	if(bets->net_total <= dt_tot->net_total)
		bets->net_total = dt_tot->net_total;
	else{
		TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> Warning : \"%s\" pool total is less then registered value (%f vs %f)\n",
			cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name, p_pool->code, bets->net_total, dt_tot->net_total);
	}

	if(bets->refund_total <= bets->total - dt_tot->live_total)
		bets->refund_total = bets->total - dt_tot->live_total;
	else{
		TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> Warning : \"%s\" pool refund_total is less then registered value (%f vs %f)\n",
			cnx->itsp_mode == mode_host ? cnx->col_att : "host", p_race->number, p_grep->name, p_pool->code, bets->refund_total, bets->total - dt_tot->live_total);
	}
	bets->refund_total = bets->total - dt_tot->live_total;
	return OK;
}

int analyze_data_payoffs(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic){
		if(fr->header->frame_type->message == msg_data)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> no payoff exchange during init phase",
					cnx->itsp_mode == mode_host ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);
		else
			return reason_inappropriate;
	}

	if(fr->header->frame_type->message == msg_data){
		if(cnx->itsp_mode == mode_host)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"host\" - race %d (grep \"%s\") -> payoffs should be sent by host",
					fr->header->race_number, cnx->grep);

		if(cnx->itsp_mode == mode_remote && fr->header->frame_type->reason == reason_begin){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> Begin of payoff sequence\n",
					cnx->col_att, fr->header->race_number, cnx->grep);
			return OK;
		}

		if(cnx->itsp_mode == mode_remote && fr->header->frame_type->reason == reason_end){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> End  of payoff sequence\n",
					cnx->col_att, fr->header->race_number, cnx->grep);
			return OK;
		}
	}

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	int i = 0;

	if(!p_race){
		if(fr->header->frame_type->message == msg_data)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> data_paoff frame with null race number",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep);
		else
			return OK;
	}

	switch(p_race->status){
	case Open_race :
	case Cancelled_race :
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> data_payoff frame with inappropriate race status (status='%c')",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_race->status);
	case Closed_race :
	case Post_Time_race :
	case Unofficial_race :
	case Official_race :
		break;
	}

	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
	p_s_s3k_pool_type p_pool_type = fast_map_at(liste_paris, fr->header->pool_code);

	if(p_pool->status == Payed_pool){
		if(fr->header->frame_type->message == msg_data)
			itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> data_payoff frame for an already payed pool (status='%c')",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->status);
		return reason_inappropriate;
	}


	if(p_pool->status != Closed_pool){
		if(fr->header->frame_type->message == msg_data)
			itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> data_payoff frame for non closed pool (status='%c')",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->status);
		return reason_inappropriate;
	}

	p_s_fast_map mp_payoffs_att = fast_map_at(p_pool->payoffs, cnx->col_att);
	p_s_s3k_payoffs p_payoff = NULL;

	if(fr->header->frame_type->message == msg_acknowledge){
		if(cnx->itsp_mode == mode_host && mp_payoffs_att)
			fast_map_data_iterate(p_payoff, mp_payoffs_att)
				p_payoff->sent_flag = 1;
		return OK;
	}

	if(!mp_payoffs_att){
		mp_payoffs_att = fast_map_init((p_pool_type->dimensions + 1) * sizeof(int*), sizeof(s_s3k_payoffs), combo_cmp_func);
		fast_map_insert(p_pool->payoffs, cnx->col_att, mp_payoffs_att);
		free(mp_payoffs_att);
		mp_payoffs_att = fast_map_at(p_pool->payoffs, cnx->col_att);
	}

	p_itsp_str_data_payoffs	dt_payoff = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;

	if(!dt_payoff)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> no data_section in data payoff frame",
			cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	re_iterate : fast_map_data_iterate(p_payoff, mp_payoffs_att){
		for(i = 0; i < dt_payoff->nb_prices; i++){
			if(!combo_cmp_func(p_payoff->win_combo, dt_payoff->price_definition[i].runner_list))
				break;
		}
		if(i == dt_payoff->nb_prices){
			char combo[256] = {0};
			write_combo(combo, p_payoff->win_combo);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" win_combo %s canceled",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code, combo);
			p_payoff = fast_map_extract(mp_payoffs_att, p_payoff->win_combo);
			free_combo(p_payoff->win_combo);
			free(p_payoff);
			goto re_iterate;
		}
	}

	i = 0;
	while(i < dt_payoff->nb_prices){
		p_payoff = fast_map_at(mp_payoffs_att, dt_payoff->price_definition[i].runner_list);
		char combo[256] = {0};
		write_combo(combo, dt_payoff->price_definition[i].runner_list);

		if(!p_payoff){
			s_s3k_payoffs payoff = {0};
			strcpy(payoff.att, cnx->col_att);
			p_payoff->win_combo = clone_combo(dt_payoff->price_definition[i].runner_list);
			p_payoff = fast_map_insert(mp_payoffs_att, payoff.win_combo, &payoff);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" combo %s new payoff entry",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,combo);
		}

		if(p_payoff->status != dt_payoff->price_definition[i].status){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" combo %s payoff status update '%c' ->'%c'",
						cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
						combo, p_payoff->status, dt_payoff->price_definition[i].status);
			p_payoff->status = dt_payoff->price_definition[i].status;
		}

		if(p_payoff->order_flag != dt_payoff->price_definition[i].order_flag){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" combo %s payoff order flag update %d ->%d",
						cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
						combo, p_payoff->order_flag, dt_payoff->price_definition[i].order_flag);
			p_payoff->order_flag = dt_payoff->price_definition[i].order_flag;
		}

		if(p_payoff->least_combo_match != dt_payoff->price_definition[i].winners){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" combo %s payoff least_combo_match update %d ->%d",
						cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
						combo, p_payoff->least_combo_match, dt_payoff->price_definition[i].winners);
			p_payoff->least_combo_match = dt_payoff->price_definition[i].winners;
		}

		if(isequal_double(p_payoff->win_coef, dt_payoff->price_definition[i].price, TOLERANCE)){
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" combo %s payoff win_coef update %f ->%f",
						cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
						combo, p_payoff->win_coef, dt_payoff->price_definition[i].price);
			p_payoff->win_coef = dt_payoff->price_definition[i].price;
		}

		i++;
	}

	return OK;
}

int analyze_data_results(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no results exchange during init phase",
				cnx->itsp_mode == mode_host ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);



	if(fr->header->frame_type->message == msg_data && cnx->itsp_mode == mode_host)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"host\" - race %d (grep \"%s\") -> results should be sent by host",
					fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	int i = 0;

	if(!p_race){
		if(fr->header->frame_type->message == msg_data)
			itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> data_result frame with numm race number",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep);
		return reason_inappropriate;
	}

	if(cnx->itsp_mode == mode_host){
		sorted_set_insert(p_race->finish->sent_atts, cnx->col_att);
		if(fr->header->frame_type->reason == reason_final)
			sorted_set_insert(p_race->finish->sent_offic_atts, cnx->col_att);
	}else{
		if(p_race->status == Open_race || p_race->status == Cancelled_race)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> data_result frame for open or canceled race (status='%c')",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_race->status);
		if(!p_race->finish || !p_race->finish->offic_flag){
			if(!fr->data_str->structure)
				return FAULT;
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> integrating new data result %s",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep,
							fr->header->frame_type->reason == reason_final ? "final" : "");

			p_itsp_str_data_results dt_rslt = fr->data_str->structure;
			if(!p_race->finish){
				p_race->finish = malloc(sizeof(s_s3k_result));
				memset(p_race->finish, 0, sizeof(s_s3k_result));
				p_race->finish->sent_atts = sorted_set_init(4, string_key_com);
				p_race->finish->sent_offic_atts = sorted_set_init(4, string_key_com);
				p_race->finish->finishers = fast_map_init(sizeof(int), sizeof(itsp_finish), int_key_com);
			}

			if(!p_race->finish->offic_flag)
				p_race->finish->offic_flag = (fr->header->frame_type->reason == reason_final);
			if(!p_race->finish->finish_time)
				p_race->finish->finish_time = c_time(&fr->header->time);

			p_itsp_finish p_finish = NULL;
			for(i = 0; i < dt_rslt->nb_finshers; i++){
				if(!(p_finish = fast_map_at(p_race->finish->finishers, &dt_rslt->finishs[i].position))){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> new finisher number %d at position %d",
							cnx->col_att, fr->header->race_number, cnx->grep, dt_rslt->finishs[i].runner, dt_rslt->finishs[i].position);
					fast_map_insert(p_race->finish->finishers, &dt_rslt->finishs[i].position, &dt_rslt->finishs[i]);
				}else{
					if(dt_rslt->finishs[i].runner != p_finish->runner)
						TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> race finish update : updating runner at position %d (%d -> %d)",
								cnx->col_att, fr->header->race_number, cnx->grep,
								dt_rslt->finishs[i].position, p_finish->runner,
								dt_rslt->finishs[i].runner);
					p_finish->runner = dt_rslt->finishs[i].runner;
					p_finish->entry = dt_rslt->finishs[i].entry;
				}
			}
		}
	}
	return OK;
}

int analyze_data_will_pay(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no will_pay exchange during init phase",
				cnx->itsp_mode == mode_host ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);

	if(cnx->itsp_mode == mode_host)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"host\" - race %d (grep \"%s\") -> will_pay should be sent by host",
				fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_pool_type p_pool_type = fast_map_at(liste_paris, fr->header->pool_code);

	int race_i = fr->header->race_number - p_pool_type->nb_races + 1;
	p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &race_i);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_pool p_pool = fast_map_at(p_race_i->pools, fr->header->pool_code);


	switch(p_race_i->status){
	case Closed_race :
	case Post_Time_race :
	case Unofficial_race :
	case Official_race :
		break;
	case Open_race :
	case Cancelled_race :
	default :
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> will_pay frame with inappropriate race %d status (status='%c')",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, race_i, p_race_i->status);
	}

	if(p_pool->status != Closed_pool)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> will_pay frame for non closed pool (status='%c')",
			cnx->itsp_mode == mode_host ? cnx->col_att : "host", race_i, cnx->grep, p_pool->status);

	p_itsp_str_will_pay	dt_will_pay = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;

	if(!dt_will_pay)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> no data_section in data will_pay frame",
			cnx->itsp_mode == mode_host ? cnx->col_att : "host", race_i, cnx->grep);

	if(dt_will_pay->columns != fast_map_count(p_race->runners))
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> wrong \"%s\" will_pay column number ~> [nb_runners for race %d] (%d -> should be %d)",
			cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number,
			cnx->grep, fr->header->pool_code, race_i, dt_will_pay->columns, fast_map_count(p_race->runners));

	p_s_fast_map mp_will_pay_att = fast_map_at(p_pool->will_pay, cnx->col_att);

	if(!mp_will_pay_att){
		mp_will_pay_att = fast_map_init((p_pool_type->dimensions + 1) * sizeof(int*), dt_will_pay->columns * sizeof(itsp_will_pay_item), combo_cmp_func);
		fast_map_insert(p_pool->will_pay, cnx->col_att, mp_will_pay_att);
		free(mp_will_pay_att);
		mp_will_pay_att = fast_map_at(p_pool->will_pay, cnx->col_att);
	}

	int i = 0, **combo__;
	p_itsp_will_pay_item p_will_pay = NULL;
	re_iterate : fast_map_iterate(combo__, p_will_pay, mp_will_pay_att){
		for(i = 0; i < dt_will_pay->rows; i++){
			if(!combo_cmp_func(combo__, dt_will_pay->will_pay_row[i].runner_list))
				break;
		}
		if(i == dt_will_pay->rows){
			char combo[256] = {0};
			write_combo(combo, combo__);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" will_pay combo %s canceled",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", race_i, cnx->grep, p_pool->code, combo);
			p_will_pay = fast_map_extract(mp_will_pay_att, combo__);
			free_combo(combo__);
			free(p_will_pay);
			goto re_iterate;
		}
	}

	i = 0;
	for(i = 0; i < dt_will_pay->rows; i++){
		char combo[256] = {0};
		write_combo(combo, dt_will_pay->will_pay_row[i].runner_list);
		p_will_pay = fast_map_at(mp_will_pay_att, dt_will_pay->will_pay_row[i].runner_list);

		if(!p_will_pay){
			combo__ = clone_combo(dt_will_pay->will_pay_row[i].runner_list);
			p_will_pay = fast_map_insert(mp_will_pay_att, combo__, dt_will_pay->will_pay_row[i].items);
			free(combo__);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" new will_pay entries for combo %s",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", race_i, cnx->grep, p_pool->code, combo);
		}else{
			for(int j = 0; j < dt_will_pay->columns; j++){
				if(p_will_pay[j].winner != dt_will_pay->will_pay_row[i].items[j].winner){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" updating combo %s will_pay winner field for runner %d (%d --> %d)",
							cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
							combo, j + 1, p_will_pay[j].winner, dt_will_pay->will_pay_row[i].items[j].winner);
					p_will_pay[j].winner = dt_will_pay->will_pay_row[i].items[j].winner;
				}
				if(isequal_double(p_will_pay[j].price, dt_will_pay->will_pay_row[i].items[j].price, TOLERANCE)){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" updating combo %s will_pay price field for runner %d (%f --> %f)",
							cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
								combo, j + 1, p_will_pay[j].price, dt_will_pay->will_pay_row[i].items[j].price);
					p_will_pay[j].price = dt_will_pay->will_pay_row[i].items[j].price;
				}
				if(isequal_double(p_will_pay[j].winning, dt_will_pay->will_pay_row[i].items[j].winning, TOLERANCE)){
					TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : pool \"%s\" updating combo %s will_pay winning field for runner %d (%f --> %f)",
							cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code,
							combo, j + 1, p_will_pay[j].winning, dt_will_pay->will_pay_row[i].items[j].winning);
					p_will_pay[j].winning = dt_will_pay->will_pay_row[i].items[j].winning;
				}
			}
		}
	}

	return OK;
}

int analyze_scan_request(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){


	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no scan exchange during init phase",
				cnx->itsp_mode == mode_host ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);

	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris,  fr->header->pool_code);

	if(p_pool_typ->dimensions <4)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> bets for \"%s\" should be exchanged via data_pools",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep,  fr->header->pool_code);

	char* att_scan = (cnx->itsp_mode == mode_host) ? p_grep->org_att : cnx->col_att;

	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, fr->header->pool_code);

	if(fr->header->frame_type->reason == reason_begin){

		if(cnx->itsp_mode == mode_host)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> scan request begin should be sent only by host",
					cnx->itsp_mode == mode_host ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);

		p_itsp_str_scan_header	dt_scan_req_begin = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
		if(!dt_scan_req_begin)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no data_section in scan_req_begin frame",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

		if(dt_scan_req_begin->combos != 1)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> cannot handle multiple combos in scan_req_begin frame",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

		if(p_pool_conf->scan_mode != dt_scan_req_begin->scan_mode)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> wrong scan mode in scan_req_begin frame (%c should be %c)",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number,
				cnx->grep, p_pool_conf->scan_mode, dt_scan_req_begin->scan_mode);

		// prevoir traitement des scan_runners, sub_runners, live_runners et favorite_runners

		if(!p_pool->sub_runners){
			char combo[256] = {0};
			write_combo(combo, dt_scan_req_begin->sub_runners);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : updating pool \"%s\" sub_runners %s",
					cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code, combo);
			p_pool->sub_runners = clone_combo(dt_scan_req_begin->sub_runners);
		}


		int race_i = p_pool->race_list[dt_scan_req_begin->leg - 1];
		p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &race_i);

		/*switch(p_race_i->status){
		case Closed_race :
		case Post_Time_race :
		case Unofficial_race :
		case Official_race :
			break;
		case Open_race :
		case Cancelled_race :
		default :
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> scan_req_begin frame with inappropriate race %d status (status='%c')",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, race_i, p_race_i->status);
		}*/

		p_s_s3k_bets bets = fast_map_at(p_pool->bets, att_scan);
		if(!bets){
			s_s3k_bets betz = {0};
			p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, att_scan);
			p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, fr->header->pool_code);
			betz.amounts_type = gross_pool_mode;
			betz.exchange_style = pool_conf->scan_mode;
			betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
			fast_map_insert(p_pool->bets, fr->header->source, &betz);
			bets = fast_map_at(p_pool->bets, fr->header->source);
		}

		p_s_s3k_bet_scan scans = fast_map_at(bets->scans, &dt_scan_req_begin->leg);
		if(!scans){
			s_s3k_bet_scan new_scan = {0};
			new_scan.leg = dt_scan_req_begin->leg;
			if(bets->exchange_style == 'S'){
				new_scan.rows = 1;
				new_scan.columns = 16;
			}else{
				new_scan.rows = p_pool_typ->nb_races;
				new_scan.columns = fast_map_count(p_race_i->runners);
			}
			new_scan.combo = clone_combo(dt_scan_req_begin->scan_runners);
			scans = fast_map_insert(bets->scans, &dt_scan_req_begin->leg, &new_scan);
		}

		itsp_terminate_sequence(cnx, seq,fr->header->frame_type->reason, NULL);
		int *leg = malloc(sizeof(int));
		*leg = dt_scan_req_begin->leg;
		itsp_frame_type fr_type = {0};
		set_fr_type(fr_type, data_scan, msg_pending, reason_final);
		schedule_sequence(cnx, 0, &fr_type, way_out, fr->header->race_number, fr->header->pool_code, 0, leg);


	}else{

		p_itsp_str_data_pools	dt_scan_req = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
		if(!dt_scan_req)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no data_section in scan_req frame",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

		int race_i = p_pool->race_list[dt_scan_req->pool_header.leg - 1];
		p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &race_i);

		switch(p_race_i->status){
		case Closed_race :
		case Post_Time_race :
		case Unofficial_race :
		case Official_race :
			break;
		case Open_race :
		case Cancelled_race :
		default :
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> scan_req_begin frame with inappropriate race %d status (status='%c')",
					cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, race_i, p_race_i->status);
		}

		p_s_s3k_bets bets = fast_map_at(p_pool->bets, att_scan);
		if(!bets)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> bets are not yet generated => please restart scan_req_begin sequence",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

		if(dt_scan_req->pool_header.pool_mode != bets->amounts_type)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> wrong bets pool_mode (%c should be %c)",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number,
				cnx->grep, dt_scan_req->pool_header.pool_mode, bets->amounts_type);

		p_s_s3k_bet_scan scans = fast_map_at(bets->scans, &dt_scan_req->pool_header.leg);
		if(!scans)
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> scans for leg %d are not yet generated  => please restart scan_req_begin sequence",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep, dt_scan_req->pool_header.leg);

		int *leg = malloc(sizeof(int));
		*leg = dt_scan_req->pool_header.leg;
		seq->special_data = leg;

	}

	return OK;
}

int analyze_pending_scan(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no scan exchange during init phase",
				cnx->itsp_mode == mode_remote ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris,  fr->header->pool_code);

	if(p_pool_typ->dimensions <4)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> bets for \"%s\" should be exchanged via data_pools",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep,  fr->header->pool_code);

	p_itsp_str_data_pools dt_pool = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
	if(!dt_pool)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> no data_section in pending_pool frame",
			cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	int race_i = p_pool->race_list[dt_pool->pool_header.leg - 1];
	p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &race_i);

	switch(p_race_i->status){
	case Closed_race :
	case Post_Time_race :
	case Unofficial_race :
	case Official_race :
		break;
	case Open_race :
	case Cancelled_race :
	default :
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> scan_req_begin frame with inappropriate race %d status (status='%c')",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, race_i, p_race_i->status);
	}

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, (cnx->itsp_mode == mode_host) ? cnx->col_att : p_grep->org_att);
	bets->amounts_type = dt_pool->pool_header.pool_mode;

	int* leg = malloc(sizeof(int));
	*leg = dt_pool->pool_header.leg;

	if(cnx->itsp_mode == mode_host){
		itsp_frame_type fr_type = {0};
		set_fr_type(fr_type, data_scan, msg_request, reason_final);
		schedule_sequence(cnx, 0, &fr_type, way_out, fr->header->race_number, fr->header->pool_code, 0, leg);
	}else{
		seq->special_data = leg;
	}

	return OK;
}

int analyze_data_scan(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq){

	if(cnx_get_logi_status(cnx) < itsp_cnx_logic)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> no data_scan exchange during init phase",
				cnx->itsp_mode == mode_host ? "host" : cnx->col_att, fr->header->race_number, cnx->grep);

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, cnx->grep);
	p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);

	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris,  fr->header->pool_code);

	if(p_pool_typ->dimensions <4)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> bets for \"%s\" should be exchanged via data_pools",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep,  fr->header->pool_code);

	p_itsp_str_data_scan dt_scan = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
	if(!dt_scan)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> no data_section in data_scan frame",
			cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	if(dt_scan->scan_header->combos != 1)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> cannot handle multiple combos in data_scan frame",
			cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep);

	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, cnx->col_att);
	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, fr->header->pool_code);

	char* att_scan = (cnx->itsp_mode == mode_host) ? cnx->col_att : p_grep->org_att;

	if(p_pool_conf->scan_mode != dt_scan->scan_header->scan_mode)
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
			"\"%s\" - race %d (grep \"%s\") -> wrong scan mode in data_scan frame (%c should be %c)",
			cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number,
			cnx->grep, p_pool_conf->scan_mode, dt_scan->scan_header->scan_mode);

	int race_i = p_pool->race_list[dt_scan->scan_header->leg - 1];
	p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &race_i);

	/*switch(p_race_i->status){
	case Closed_race :
	case Post_Time_race :
	case Unofficial_race :
	case Official_race :
		break;
	case Open_race :
	case Cancelled_race :
	default :
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> scan_req_begin frame with inappropriate race %d status (status='%c')",
				cnx->itsp_mode == mode_host ? cnx->col_att : "host", fr->header->race_number, cnx->grep, race_i, p_race_i->status);
	}*/

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, att_scan);
	if(!bets){
		s_s3k_bets betz = {0};
		p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, att_scan);
		p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, fr->header->pool_code);
		betz.amounts_type = dt_scan->scan_data->pool_header.pool_mode;
		betz.exchange_style = pool_conf->scan_mode;
		betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
		fast_map_insert(p_pool->bets, fr->header->source, &betz);
		bets = fast_map_at(p_pool->bets, fr->header->source);
	}

	p_s_s3k_bet_scan scans = fast_map_at(bets->scans, &dt_scan->scan_header->leg);
	if(!scans){
		s_s3k_bet_scan new_scan = {0};
		new_scan.leg = dt_scan->scan_header->leg;
		new_scan.combo = clone_combo(dt_scan->scan_header->scan_runners);
		if(bets->exchange_style == 'S'){
			new_scan.rows = 1;
			new_scan.columns = 16;
		}else{
			new_scan.rows = p_pool_typ->nb_races;
			new_scan.columns = fast_map_count(p_race_i->runners);
		}
		scans = fast_map_insert(bets->scans, &dt_scan->scan_header->leg, &new_scan);
	}

	int **lv_rnrs = itsp_make_live_runner(p_pool_typ, p_pool);
	if(combo_cmp_func(lv_rnrs, dt_scan->scan_header->live_runners)){
		char buff1[1024] = {0}, buff2[1024] = {0};
			write_combo(buff1, dt_scan->scan_header->live_runners);
			write_combo(buff2, lv_rnrs);
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> leg %d live_runners error (%s should be %s)",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number,
				cnx->grep, dt_scan->scan_header->leg, buff1, buff2);
	}
	free_combo(lv_rnrs);

	if(combo_cmp_func(scans->combo, dt_scan->scan_header->scan_runners)){
		char combo1[256] = {0}, combo2[256] = {0};
		write_combo(combo1, scans->combo);
		write_combo(combo2, dt_scan->scan_header->sub_runners);
		return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
				"\"%s\" - race %d (grep \"%s\") -> leg %d scan_runners error (%s should be %s)",
				cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number,
				cnx->grep, dt_scan->scan_header->leg, combo2, combo1);
	}

	if(cnx->itsp_mode == mode_remote){

		// prevoir traitement des scan_runners, sub_runners, live_runners et favorite_runners

		if(!p_pool->sub_runners){
			char combo[256] = {0};
			write_combo(combo, dt_scan->scan_header->sub_runners);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : adding pool \"%s\" sub_runners %s",
					cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code, combo);
			p_pool->sub_runners = clone_combo(dt_scan->scan_header->sub_runners);
		}

		if(combo_cmp_func(scans->combo, dt_scan->scan_header->scan_runners)){
			char combo1[256] = {0}, combo2[256] = {0};
			write_combo(combo1, scans->combo);
			write_combo(combo2, dt_scan->scan_header->sub_runners);
			TRACE(cnx->log_file, "\"%s\" - race %d (grep \"%s\") -> info : updating pool \"%s\" scan_runners %s -> %s",
					cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep, p_pool->code, combo1, combo2);
			free_combo(scans->combo);
			scans->combo = clone_combo(dt_scan->scan_header->scan_runners);
		}


	}else{

		if(combo_cmp_func(p_pool->sub_runners, dt_scan->scan_header->sub_runners)){
			char buff1[1024] = {0}, buff2[1024] = {0};
				write_combo(buff1, dt_scan->scan_header->sub_runners);
				write_combo(buff2, p_pool->sub_runners);
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> sub_runners error (%s should be %s)",
					cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number,
					cnx->grep, dt_scan->scan_header->leg, buff1, buff2);
		}

	}

	if(dt_scan->scan_data->pool_data.columns){
		int rowz = 0, columnz = 0;

		rowz = scans->rows;
		columnz = scans->columns;

		if((1 != dt_scan->scan_data->pool_header.segments) ||
		   (rowz != dt_scan->scan_data->pool_data.rows) ||
		   (columnz != dt_scan->scan_data->pool_data.columns)){
			return itsp_terminate_sequence(cnx, seq, reason_inappropriate,
					"\"%s\" - race %d (grep \"%s\") -> error : received \"%s\" scans with dimz : %dx%dx%d (should be %dx%dx%d) (fr %d)\n",
							cnx->itsp_mode == mode_remote ? cnx->col_att : "host", fr->header->race_number, cnx->grep,
							p_pool_typ->code,
							dt_scan->scan_data->pool_header.segments,
							dt_scan->scan_data->pool_data.rows,
							dt_scan->scan_data->pool_data.columns,
							1,
							rowz,
							columnz,
							fr->frame_num);
		}else{

			if(!scans->amounts){
				scans->amounts = malloc(((rowz * columnz) + 1) * sizeof(double));
				memset(scans->amounts, 0, ((rowz * columnz) + 1) * sizeof(double));
			}



			int i = 0, j = 0;
			for(i = 0 ; i < rowz; i++)
				for(j = 0; j < columnz; j++)
					scans->amounts[(i * columnz) + j] = dt_scan->scan_data->pool_data.matrix_data[i][j];

		}
	}

	scans->scan_total = dt_scan->scan_total;
	scans->live_total = dt_scan->live_total;

	bets->total = dt_scan->scan_data->pool_data.total;
	bets->net_total = dt_scan->scan_data->pool_data.net_total;

	return OK;
}

int itsp_frame_analyze(p_s_itsp_cnx cnx, p_itsp_frame fr, p_action_sequence seq, p_itsp_action action){

	int out = FAULT;

	if((fr->header->frame_type->message == msg_acknowledge)
		&& (65 <= fr->header->frame_type->reason) && (fr->header->frame_type->reason <= 90)){
		char* error__ = (fr->data_str && fr->data_str->structure) ? fr->data_str->structure : NULL;
		TRACE(cnx->log_file, "seq <%c> nb %d terminated with negative ack, reason = '%c', message : <%s>\n",
				fr->header->frame_type->data, fr->header->sequence,
				fr->header->frame_type->reason, error__ ? error__ : ""
			 );
		while((action = fifo_pull(seq->actions)))
			free(action);
		return fr->header->frame_type->reason;
	}

	if((out = analyze_itsp_header(fr->header, cnx, seq, action)) != reason_ok)
		return out;

	switch(fr->header->frame_type->data){
	case data_link :
		switch(fr->header->frame_type->message){
		case msg_data :
		case msg_acknowledge :
			return analyze_data_link(cnx, fr, seq);
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_config :
		switch(fr->header->frame_type->message){
		case msg_pending :
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			analyze_data_config(cnx, fr, seq);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_race_status :
		switch(fr->header->frame_type->message){
		case msg_pending :
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			analyze_data_race_status(cnx, fr, seq);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_pools :
		analyze_data_pools(cnx, fr, seq);
		break;
	case data_scan :
		switch(fr->header->frame_type->message){
		case msg_pending :
			analyze_pending_scan(cnx, fr, seq);
			break;
		case msg_request :
			analyze_scan_request(cnx, fr, seq);
			break;
		case msg_data :
			analyze_data_scan(cnx, fr, seq);
			break;
		case msg_acknowledge :
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_pool_total :
		switch(fr->header->frame_type->message){
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			analyze_data_totals(cnx, fr, seq);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_payoffs :
		switch(fr->header->frame_type->message){
		case msg_data :
		case msg_acknowledge :
			analyze_data_payoffs(cnx, fr, seq);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_results :
		switch(fr->header->frame_type->message){
		case msg_data :
		case msg_acknowledge :
			analyze_data_results(cnx, fr, seq);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_alert :
		switch(fr->header->frame_type->message){
		case msg_data :
		case msg_acknowledge :
			return analyze_data_alert(cnx, fr, seq);
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_will_pay :
		switch(fr->header->frame_type->message){
		case msg_pending :
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			return analyze_data_will_pay(cnx, fr, seq);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	case data_file_transfert :
		switch(fr->header->frame_type->message){
		case msg_pending :
			break;
		case msg_request :
			break;
		case msg_data :
			break;
		case msg_acknowledge :
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr->header->frame_type->data, fr->header->frame_type->message);
			abort();
		}
		break;
	}
	return OK;
}
