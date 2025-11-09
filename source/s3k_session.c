/*
 * create_session.c
 *
 *  Created on: 31 mai 2017
 *      Author: f.baccari
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <i_file.h>
 #include <i_string.h>
 #include <i_fast_map.h>
 #include <itsp_translator.h>
 #include <itsp_frame.h>
 #include <s3k_structs.h>
 #include <itsp_common.h>
 #include <s3k_session.h>

p_s_fast_map liste_attributaires = NULL;
p_s_fast_map liste_connexions = NULL;
p_s_fast_map liste_reunions = NULL;
p_s_fast_map liste_paris = NULL;
int pool_typ_ctr = 1;
int grep_ctr = 1;
int att_ctr = 1;

long pool_conf_cmp_func(void* p1, void* p2){
	return (long) memcmp(p1, p2, sizeof(itsp_str_pool_config));
}

ret_code load_atts_from_log(p_itsp_sequence* sequences){
	p_itsp_sequence seq = NULL;
	p_itsp_frame fr = NULL;
	int ctr = 0, ctr1 = 0;
	ret_code out = err_default;

	if(!liste_attributaires)
		liste_attributaires = fast_map_init(4, sizeof(s_s3k_attrib), string_key_com);

	while((seq = sequences[ctr++])){
		ctr1 = 0;
		while((fr = seq->frames[ctr1++]))
			if(!fast_map_at(liste_attributaires, fr->header->source)){
				s_s3k_attrib att = {0};
				memcpy(att.code_att,fr->header->source,4);
				att.id_att = att_ctr++;
				att.att_description = NULL;
				att.jet_lag = 0;
				if((seq->data_type == data_link)&& fr->data_str && fr->data_str->structure){
					p_itsp_str_data_link data_ln = fr->data_str->structure;
					strcpy(att.itsp_version.document, data_ln->identifier.document);
					att.itsp_version.version.revision_number = data_ln->identifier.version.revision_number;
					att.itsp_version.version.version_number = data_ln->identifier.version.version_number;
					if(strlen(data_ln->text)){
						att.att_description = malloc(strlen(data_ln->text) + 1);
						strcpy(att.att_description, data_ln->text);
					}
				}else
					memset(&att.itsp_version, 0, sizeof(itsp_str_identifier));
				fast_map_insert(liste_attributaires, fr->header->source, &att);
				if(!out) out = ret_ok;
			}
	}
	return out;
}

ret_code load_pools_from_log(p_itsp_sequence* sequences){
	p_itsp_sequence seq = NULL;
	p_itsp_frame fr = NULL;
	int i = 0, ctr = 0, ctr1 = 0, ctr2 = 0;
	ret_code out = err_default;

	if(!liste_paris)
		liste_paris = fast_map_init(4, sizeof(s_s3k_pool_type), string_key_com);

	while((seq = sequences[ctr++])){

		if(seq->data_type != data_race_status)
			continue;

		ctr1 = 0;
		while((fr = seq->frames[ctr1]) && (!fr->host_flag ||
			  (fr->header->frame_type->message != msg_data) ||
			  !fr->data_str || !fr->data_str->structure))
			ctr1++;

		if(fr){
			p_itsp_str_data_race_status dt_rs = fr->data_str->structure;
			for (ctr2 = 0; ctr2 < dt_rs->pools; ctr2++){
				if(!fast_map_at(liste_paris, dt_rs->pool_def[ctr2].pool_code)){
					s_s3k_pool_type pool_ty = {0};
					pool_ty.id_pg = pool_typ_ctr++;
					pool_ty.nb_races = 1;
					i = 0;
					while(dt_rs->pool_def[ctr2].race_list[i++])
						pool_ty.nb_races++;
					pool_ty.dimensions = 0;
					memcpy(&pool_ty.code, dt_rs->pool_def[ctr2].pool_code,4);
					fast_map_insert(liste_paris, dt_rs->pool_def[ctr2].pool_code, &pool_ty);
					if(!out) out = ret_ok;
				}
			}
		}

	}
	return out;
}

ret_code load_grep_from_log(p_itsp_sequence* sequences){
	p_itsp_sequence seq = NULL;
	p_itsp_frame fr = NULL;
	int i = 0, ctr = 0, ctr1 = 0;
	ret_code out = err_default;

	if(!liste_reunions)
		liste_reunions = fast_map_init(4, sizeof(s_s3k_grep), string_key_com);

	while((seq = sequences[ctr++])){

		switch(seq->data_type){

		case data_config:
			ctr1 = 0;
			while((fr = seq->frames[ctr1++])){

				if( !fr->data_str || !fr->data_str->structure || (fr->header->frame_type->message != msg_data))
					continue;

				p_itsp_str_data_config 	dt_cfg = fr->data_str->structure;
				p_s_s3k_grep p_grep = fast_map_at(liste_reunions, fr->header->event_code);
				if(fr->host_flag && !p_grep){
					s_s3k_grep grep = {0};
					grep.nu_grep = grep_ctr++;
					memcpy(grep.name, fr->header->event_code, 4);
					memcpy(grep.org_att, fr->header->source, 4);
					grep.date_time = c_date_time_tm(&dt_cfg->date, &dt_cfg->time);
					grep.performance = dt_cfg->performance;
					grep.calculation = dt_cfg->calculation;
					grep.close_bet_delay = dt_cfg->close_bet_delay;
					grep.close_cancel_delay = dt_cfg->close_cancel_delay;
					grep.races = fast_map_init(sizeof(int), sizeof(s_s3k_race), int_key_com);
					grep.col_configs = fast_map_init(4, sizeof(s_s3k_att_conf), string_key_com);
					p_grep = fast_map_insert(liste_reunions, fr->header->event_code, &grep);
				}

				p_s_s3k_att_conf p_col_conf = NULL;
				if(!(p_col_conf = fast_map_at(p_grep->col_configs,fr->header->source))){
					s_s3k_att_conf col_conf = {{0}};
					memcpy(col_conf.att, fr->header->source, 4);
					col_conf.abo_races = fast_map_init(sizeof(int), sizeof(s_s3k_abo_race), int_key_com);
					col_conf.pool_configs = fast_map_init(4, sizeof(itsp_str_pool_config), string_key_com);
					p_col_conf = fast_map_insert(p_grep->col_configs, fr->header->source, &col_conf);
				}

				for(i = 0; i < dt_cfg->race_list.races; i++){
					if(!fast_map_at(p_col_conf->abo_races, &dt_cfg->race_list.race[i])){
						s_s3k_abo_race abo_race = {0};
						abo_race.number = dt_cfg->race_list.race[i];
						abo_race.abo_pools = sorted_set_init(4, string_key_com);
						fast_map_insert(p_col_conf->abo_races, &abo_race.number, &abo_race);
					}
				}


				for(i = 0; i < dt_cfg->nb_pools; i++)
					if(!fast_map_at(p_col_conf->pool_configs, dt_cfg->pool_configs[i].pool_code))
						fast_map_insert(p_col_conf->pool_configs, dt_cfg->pool_configs[i].pool_code, &dt_cfg->pool_configs[i]);


			}
			break;

		case data_race_status:
			ctr1 = 0;
			while((fr = seq->frames[ctr1++])){
				p_s_s3k_grep p_grep = fast_map_at(liste_reunions, fr->header->event_code);

				if(!p_grep)
					continue;

				if(fr->header->frame_type->reason == reason_end){
					p_s_s3k_race p_race = NULL;
					if((p_race = fast_map_at(p_grep->races, &fr->header->race_number)))
						p_race->start = c_time(&fr->header->time);
				}

				if(fr->header->frame_type->message != msg_data)
					continue;

				if(!fr->data_str || !fr->data_str->structure)
					continue;

				p_itsp_str_data_race_status	dt_rs = fr->data_str->structure;

				if(fr->host_flag){
					p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
					if(fr->host_flag && !p_race){
						s_s3k_race race = {0};
						race.number = fr->header->race_number;
						race.status = dt_rs->status;
						race.display = dt_rs->display;
						race.runners = fast_map_init(sizeof(int), sizeof(s_s3k_runner), int_key_com);
						race.pools =  fast_map_init(4, sizeof(s_s3k_pool), string_key_com);
						race.brackets = sorted_vect_init(sizeof(s_s3k_bracket), int_key_com);
						p_race = fast_map_insert(p_grep->races, &race.number, &race);
					}

					for(i = 1; i < dt_rs->runners + 1; i++){
						p_s_s3k_runner p_runner = fast_map_at(p_race->runners, &i);
						if(!p_runner){
							s_s3k_runner runner = {0};
							runner.number = i;
							p_runner = fast_map_insert(p_race->runners, &i, &runner);
						}
						p_runner->status = dt_rs->runner_status[i - 1];
					}

					for (i = 0; i < dt_rs->pools; i++){
						p_s_s3k_pool p_pool_def = fast_map_at(p_race->pools, dt_rs->pool_def[i].pool_code);
						if(!p_pool_def){
							s_s3k_pool pool_def = {0};
							strcpy(pool_def.code, dt_rs->pool_def[i].pool_code);
							pool_def.bets = fast_map_init(4, sizeof(s_s3k_bets), string_key_com);
							pool_def.payoffs = fast_map_init(4, sizeof(s_fast_map), string_key_com);
							pool_def.will_pay = fast_map_init(4, sizeof(s_fast_map), string_key_com);
							p_pool_def = fast_map_insert(p_race->pools, pool_def.code, &pool_def);
						}
						int j = 0;
						while(dt_rs->pool_def[i].race_list[j++])
							continue;
						if(!p_pool_def->race_list){
							p_pool_def->race_list = malloc((j + 2) * sizeof(int));
							p_pool_def->race_list[j + 1] = 0;
							while(j--)
								p_pool_def->race_list[j + 1] = dt_rs->pool_def[i].race_list[j];
							p_pool_def->race_list[0] = fr->header->race_number;
						}else{
							while(j--){
								if(p_pool_def->race_list[j + 1] != dt_rs->pool_def[i].race_list[j])
									p_pool_def->race_list[j + 1] = dt_rs->pool_def[i].race_list[j]; // Normallement ce comportement ne doit pas arriver
							}
						}

						if(p_pool_def->status != dt_rs->pool_def[i].pool_status){
							p_pool_def->status = dt_rs->pool_def[i].pool_status;
						}

						if(p_pool_def->min_bet != dt_rs->pool_def[i].min_bet){
							p_pool_def->min_bet = dt_rs->pool_def[i].min_bet;
						}

						if(p_pool_def->net_addin != dt_rs->pool_def[i].net_addin){
							p_pool_def->net_addin = dt_rs->pool_def[i].net_addin;
						}

						if(p_pool_def->gross_addin != dt_rs->pool_def[i].gross_addin){
							p_pool_def->gross_addin = dt_rs->pool_def[i].gross_addin;
						}

					}

					for (i = 0; i < dt_rs->scratch_pools; i++){
						p_s_s3k_pool_type pool_type_i = fast_map_at(liste_paris, dt_rs->scratch_info[i].pool_code);
						if(!pool_type_i) continue;

						p_s_s3k_pool pool_i = fast_map_at(p_race->pools, dt_rs->scratch_info[i].pool_code);
						if(pool_i){
							if(pool_i->scratch_info) free_combo(pool_i->scratch_info);
							pool_i->scratch_info = clone_combo(dt_rs->scratch_info[i].pool_runners);
							//il y a un travail a faire pour toutes les epreuves du paris au niveau du runner status
						}

					}
					continue;
				}

				p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, fr->header->source);
				if(!p_att_conf)
					continue;

				p_s_s3k_abo_race p_abo_race = fast_map_at(p_att_conf->abo_races, &fr->header->race_number);
				if(!p_abo_race)
					continue;

				if(!fr->host_flag){
					if(p_att_conf && p_abo_race)
						for(i = 0; i < dt_rs->pools; i++)
							sorted_set_insert(p_abo_race->abo_pools, dt_rs->pool_def[i].pool_code);
					continue;
				}

			}
			break;

		case data_results:
			ctr1 = 0;
			while((fr = seq->frames[ctr1]) && (fr->header->frame_type->message != msg_data))
				ctr1++;
			if(!fr || !fr->data_str || !fr->data_str->structure)
				continue;
			p_s_s3k_grep p_grep = fast_map_at(liste_reunions, fr->header->event_code);
			p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
			p_itsp_str_data_results dt_rslt = fr->data_str->structure;
			if(fr->host_flag && p_grep && p_race){
				if(!p_race->favorite_list){
					i = 0;
					while(dt_rslt->favorite_list[i++])
						continue;
					p_race->favorite_list = malloc(i * sizeof(int));
					while(i--)
						p_race->favorite_list[i] = dt_rslt->favorite_list[i];
				}
				if(!p_race->finish){
					p_s_s3k_result p_result = malloc(sizeof(s_s3k_result));
					memset(p_result, 0, sizeof(s_s3k_result));
					p_result->sent_atts = sorted_set_init(4, string_key_com);
					p_result->sent_offic_atts = sorted_set_init(4, string_key_com);
					p_result->finishers = fast_map_init(sizeof(int), sizeof(itsp_finish), int_key_com);
					p_result->offic_flag = (fr->header->frame_type->reason == reason_final);
					p_result->finish_time = c_time(&fr->header->time);
					for(i = 0; i < dt_rslt->nb_finshers; i++)
						fast_map_insert(p_result->finishers, &dt_rslt->finishs[i].position, &dt_rslt->finishs[i]);
					p_race->finish = p_result;
				}
				else
					if(!p_race->finish->offic_flag && (fr->header->frame_type->reason == reason_final)){
						p_race->finish->offic_flag = 1;
						for(i = 0; i < dt_rslt->nb_finshers; i++){
							p_itsp_finish p_fnsh = fast_map_at(p_race->finish->finishers, &dt_rslt->finishs[i].position);
							if(!p_fnsh || memcmp(p_fnsh, &dt_rslt->finishs[i], sizeof(itsp_finish))){
								fast_map_clear(p_race->finish->finishers);
								for(i = 0; i < dt_rslt->nb_finshers; i++)
									fast_map_insert(p_race->finish->finishers, &dt_rslt->finishs[i].position, &dt_rslt->finishs[i]);
							}
						}
						continue;
					}
			}

			break;

		default:
			break;
		}
		if(!out) out = ret_ok;
	}
	return out;
}

ret_code load_winning_from_log(p_itsp_sequence* sequences){
	p_itsp_sequence seq = NULL;
	p_itsp_frame fr = NULL;
	int ctr = 0, ctr1 = 0;
	ret_code out = err_default;

	while((seq = sequences[ctr++])){
		if(seq->data_type == data_payoffs){

			if(!seq->to[0])
				continue;

			ctr1 = 0;
			while(seq->frames[ctr1] && (seq->frames[ctr1]->header->frame_type->message != msg_data))
				ctr1++;

			if(!(fr = seq->frames[ctr1]) || !fr->data_str || !fr->data_str->structure)
				continue;

			if(fr->header->frame_type->reason != reason_final)
				continue;

			p_s_s3k_grep p_grep = fast_map_at(liste_reunions, fr->header->event_code);
			if(!p_grep)
				continue;

			p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
			if(!p_race)
				continue;

			p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
			p_s_s3k_pool_type p_pool_type = fast_map_at(liste_paris, fr->header->pool_code);
			if(!p_pool || !p_pool_type)
				continue;

			p_itsp_str_data_payoffs ptr = fr->data_str->structure;

			p_s_fast_map mp_payoffs_att = fast_map_at(p_pool->payoffs, seq->to);
			if(!mp_payoffs_att){
				mp_payoffs_att = fast_map_init((p_pool_type->dimensions + 1) * sizeof(int*), sizeof(s_s3k_payoffs), combo_cmp_func);
				fast_map_insert(p_pool->payoffs, seq->to, mp_payoffs_att);
				free(mp_payoffs_att);
				mp_payoffs_att = fast_map_at(p_pool->payoffs, seq->to);
			}

			int i = 0;
			while(i < ptr->nb_prices){
				p_s_s3k_payoffs p_payoff = fast_map_at(mp_payoffs_att, ptr->price_definition[i].runner_list);
				if(!p_payoff){
					s_s3k_payoffs payoff = {0};
					strcpy(payoff.att, seq->to);
					payoff.status = ptr->price_definition[i].status;
					payoff.win_combo = clone_combo(ptr->price_definition[i].runner_list);
					payoff.order_flag = ptr->price_definition[i].order_flag;
					fast_map_insert(mp_payoffs_att, payoff.win_combo, &payoff);
					p_payoff = fast_map_at(mp_payoffs_att, payoff.win_combo);
				}

				p_payoff->least_combo_match = ptr->price_definition[i].winners;

				p_payoff->win_coef = ptr->price_definition[i].price;

				i++;
			}
			continue;
		}

		if(seq->data_type == data_will_pay){
			if(!seq->to[0])
				continue;

			ctr1 = 0;
			while(seq->frames[ctr1] && (seq->frames[ctr1]->header->frame_type->message != msg_data))
				ctr1++;

			if(!(fr = seq->frames[ctr1]) || !fr->data_str || !fr->data_str->structure)
				continue;

			p_s_s3k_grep p_grep = fast_map_at(liste_reunions, fr->header->event_code);
			if(!p_grep)
				continue;

			p_s_s3k_pool_type p_pool_type = fast_map_at(liste_paris, fr->header->pool_code);
			if(!p_pool_type)
				continue;

			int race_i = fr->header->race_number - p_pool_type->nb_races + 1;
			p_s_s3k_race p_race = fast_map_at(p_grep->races, &race_i);
			if(!p_race)
				continue;

			p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
			if(!p_pool)
				continue;

			p_itsp_str_will_pay ptr = fr->data_str->structure;

			int i = -1;
			while(p_pool->race_list[i+1])
				i++;

			if(i < 0)
				continue;

			p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &p_pool->race_list[i]);
			if(!p_race_i)
				continue;

			p_s_fast_map att_will_pay = fast_map_at(p_pool->will_pay, seq->to);
			if(!att_will_pay){
				att_will_pay = fast_map_init((p_pool_type->dimensions + 1) * sizeof(int*), ptr->columns * sizeof(itsp_will_pay_item), combo_cmp_func);
				fast_map_insert(p_pool->will_pay, seq->to, att_will_pay);
				free(att_will_pay);
				att_will_pay = fast_map_at(p_pool->will_pay, seq->to);
			}

			i = 0;
			while(i++ < ptr->rows)
				fast_map_insert(att_will_pay, ptr->will_pay_row->runner_list, ptr->will_pay_row->items);

		}
	}

	return out;
}

ret_code load_bets_from_log(p_itsp_sequence* sequences){
	p_itsp_sequence seq = NULL;
	p_itsp_frame fr = NULL;
	int ctr = 0, ctr1 = 0;
	ret_code out = err_default;
	int segz = 0, rowz = 0, columnz = 0;
	p_s_s3k_bets bets = NULL;

	while((seq = sequences[ctr++])){

		switch(seq->data_type){

		case data_pools :

			ctr1 = 0;
			while(seq->frames[ctr1] && (seq->frames[ctr1]->header->frame_type->message != msg_data))
				ctr1++;

			if(!(fr = seq->frames[ctr1]) || !fr->data_str || !fr->data_str->structure)
				continue;

			p_itsp_str_data_pools dt_pools = fr->data_str->structure;

			p_s_s3k_grep p_grep = fast_map_at(liste_reunions, fr->header->event_code);
			if(!p_grep)
				continue;

			p_s_s3k_race p_race = fast_map_at(p_grep->races, &fr->header->race_number);
			if(!p_race)
				continue;

			p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris, fr->header->pool_code);
			p_s_s3k_pool p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
			if(!p_pool_typ || !p_pool)
				continue;

			if(dt_pools->pool_data.columns){

				p_s_s3k_bets bets = NULL;
				if(!(bets = fast_map_at(p_pool->bets, fr->header->source))){
					s_s3k_bets betz = {0};
					betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
					betz.exchange_style = s_mod_pool;
					betz.amounts_type = dt_pools->pool_header.pool_mode;
					bets = fast_map_insert(p_pool->bets, fr->header->source, &betz);
				}

				if(!bets->exchange_style)
					bets->exchange_style = s_mod_pool;

				if(!p_pool->dimensions){

					if(!p_pool_typ->dimensions){
						p_pool_typ->dimensions = 1;
						if(dt_pools->pool_data.rows > 1)
							p_pool_typ->dimensions = 2;
						if(dt_pools->pool_header.segments > 1)
							p_pool_typ->dimensions = 3;
					}

					p_pool->dimensions = malloc(4 * sizeof(int));

					switch(p_pool_typ->dimensions){
						case 1:
							segz = 1;
							rowz = 1;
							columnz = fast_map_count(p_race->runners);
						break;
						case 2 :
							if(p_pool_typ->nb_races == 2){
								rowz = fast_map_count(p_race->runners);
								p_race = fast_map_at(p_grep->races, &p_pool->race_list[1]);
								columnz = fast_map_count(p_race->runners);
								p_race = fast_map_at(p_grep->races, &fr->header->race_number);
							}else{
								columnz = rowz = fast_map_count(p_race->runners);
							}
						break;
						case 3 :
							if(p_pool_typ->nb_races == 3){
								segz = fast_map_count(p_race->runners);
								p_race = fast_map_at(p_grep->races, &p_pool->race_list[1]);
								rowz = fast_map_count(p_race->runners);
								p_race = fast_map_at(p_grep->races, &p_pool->race_list[2]);
								columnz = fast_map_count(p_race->runners);
								p_race = fast_map_at(p_grep->races, &fr->header->race_number);
							}else{
								segz = columnz = rowz = fast_map_count(p_race->runners);
							}
						break;
						default :
						break;
					}

					p_pool->dimensions[3] = 0;
					p_pool->dimensions[0] = segz;
					p_pool->dimensions[1] = rowz;
					p_pool->dimensions[2] = columnz;
				}else{
					segz = p_pool->dimensions[0];
					rowz = p_pool->dimensions[1];
					columnz = p_pool->dimensions[2];
				}

				if(!bets->amounts){
					bets->amounts = malloc(((segz * rowz * columnz) + 1) * sizeof(double));
					memset(bets->amounts, 0, ((segz * rowz * columnz) + 1) * sizeof(double));
				}

				if((dt_pools->pool_data.rows != p_pool->dimensions[1]) || (dt_pools->pool_data.columns != p_pool->dimensions[2])){
					DEBUG_WARN("dimz error -> r%dc%d(%d) : bet \"%s\" dim = %dx%dx%d /%dx%dx%d (fr %d)",
									p_grep->nu_grep, p_race->number,p_pool_typ->nb_races,
									p_pool_typ->code,
									dt_pools->pool_header.segments,
									dt_pools->pool_data.rows,
									dt_pools->pool_data.columns,
									p_pool->dimensions[0],
									p_pool->dimensions[1],
									p_pool->dimensions[2],
									fr->frame_num);
					abort();
				}else{
					int i = 0, j = 0, offset = 0;
					offset = (dt_pools->pool_header.segment - 1) * rowz * columnz;
					for(i = 0 ; i < rowz; i++) for(j = 0; j < columnz; j++)
						bets->amounts[offset + (i * columnz) + j] = dt_pools->pool_data.matrix_data[i][j];
					bets->total = dt_pools->pool_data.total;
					bets->net_total = dt_pools->pool_data.net_total;
				}
			}
		break;

		case data_pool_total :

			ctr1 = 0;
			while(seq->frames[ctr1] &&(seq->frames[ctr1]->header->frame_type->message != msg_data))
				ctr1++;

			if(!(fr = seq->frames[ctr1]) || !fr->data_str || !fr->data_str->structure)
				continue;

			p_itsp_str_data_totals dt_total = fr->data_str->structure;

			p_grep = fast_map_at(liste_reunions, fr->header->event_code);
			if(!p_grep)
				continue;

			p_race = fast_map_at(p_grep->races, &fr->header->race_number);
			if(!p_race)
				continue;

			p_pool_typ = fast_map_at(liste_paris, fr->header->pool_code);
			p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
			if(!p_pool_typ || !p_pool)
				continue;

			if(!(bets = fast_map_at(p_pool->bets, fr->header->source))){
				s_s3k_bets betz = {0};
				p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, fr->header->source);
				p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, fr->header->pool_code);
				betz.exchange_style = pool_conf->scan_mode;
				betz.amounts_type = p_grep->calculation == 'N' ? 'N' : 'G';
				betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
				fast_map_insert(p_pool->bets, fr->header->source, &betz);
				bets = fast_map_at(p_pool->bets, fr->header->source);
			}

			bets->total = dt_total->live_total;
			bets->net_total = dt_total->net_total;


		break;

		case data_scan :
			ctr1 = 0;
			while(seq->frames[ctr1] && (seq->frames[ctr1]->header->frame_type->message != msg_data))
				ctr1++;

			if(!(fr = seq->frames[ctr1]) || !fr->data_str || !fr->data_str->structure)
				continue;

			p_itsp_str_data_scan dt_scan = fr->data_str->structure;

			if(!dt_scan->scan_data)
				continue;

			p_grep = fast_map_at(liste_reunions, fr->header->event_code);
			if(!p_grep)
				continue;

			p_race = fast_map_at(p_grep->races, &fr->header->race_number);
			if(!p_race)
				continue;

			p_pool_typ = fast_map_at(liste_paris, fr->header->pool_code);
			p_pool = fast_map_at(p_race->pools, fr->header->pool_code);
			if(!p_pool_typ || !p_pool)
				continue;

			if(p_pool->sub_runners && !combo_cmp_func(p_pool->sub_runners, dt_scan->scan_header->sub_runners)){
				free_combo(p_pool->sub_runners);
				p_pool->sub_runners = NULL;
			}
			if(!p_pool->sub_runners)
				p_pool->sub_runners = clone_combo(dt_scan->scan_header->sub_runners);

			if(!(bets = fast_map_at(p_pool->bets, fr->header->source))){
				s_s3k_bets betz = {0};
				betz.scans = fast_map_init(sizeof(int), sizeof(s_s3k_bet_scan), int_key_com);
				betz.exchange_style = dt_scan->scan_header->scan_mode;
				betz.amounts_type = dt_scan->scan_data->pool_header.pool_mode;
				fast_map_insert(p_pool->bets, fr->header->source, &betz);
				bets = fast_map_at(p_pool->bets, fr->header->source);
			}

			if(!bets->exchange_style)
				bets->exchange_style = dt_scan->scan_header->scan_mode;

			if(!p_pool_typ->dimensions)
				p_pool_typ->dimensions = combo_cardinal(dt_scan->scan_header->live_runners);

			p_s_s3k_bet_scan leg_scan = NULL;
			if(!(leg_scan = fast_map_at(bets->scans, &dt_scan->scan_header->leg))){
				s_s3k_bet_scan new_scan = {0};
				new_scan.leg = dt_scan->scan_header->leg;
				new_scan.combo = clone_combo(dt_scan->scan_header->scan_runners);
				fast_map_insert(bets->scans, &dt_scan->scan_header->leg, &new_scan);
				leg_scan = fast_map_at(bets->scans, &dt_scan->scan_header->leg);
			}

			bets->total = dt_scan->scan_data->pool_data.total;
			bets->net_total = dt_scan->scan_data->pool_data.net_total;
			leg_scan->scan_total = dt_scan->scan_total;
			leg_scan->live_total = dt_scan->live_total;


			if(dt_scan->scan_data->pool_data.columns){

				p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &p_pool->race_list[dt_scan->scan_header->leg - 1]);
				if(!p_race_i){
					DEBUG_WARN("race %d not found on grep %s", dt_scan->scan_header->leg, p_grep->name);
					abort();
				}

				if(bets->exchange_style == 'S'){
					segz = rowz = 1;
					columnz = 16;
				}else{
					segz = 1;
					rowz = p_pool_typ->nb_races;
					columnz = fast_map_count(p_race_i->runners);
				}

				if((segz != dt_scan->scan_data->pool_header.segments) ||
				   (rowz != dt_scan->scan_data->pool_data.rows) ||
				   (columnz != dt_scan->scan_data->pool_data.columns)){
					DEBUG_WARN("dimz error -> r%dc%d(%d) : bet \"%s\" dim = %dx%dx%d /%dx%dx%d (fr %d)\n",
									p_grep->nu_grep, p_race->number,p_pool_typ->nb_races,
									p_pool_typ->code,
									dt_scan->scan_data->pool_header.segments,
									dt_scan->scan_data->pool_data.rows,
									dt_scan->scan_data->pool_data.columns,
									segz,
									rowz,
									columnz,
									fr->frame_num);
					abort();
				}else{

					leg_scan->amounts = malloc(((segz * rowz * columnz) + 1) * sizeof(double));
					memset(leg_scan->amounts, 0, ((segz * rowz * columnz) + 1) * sizeof(double));

					leg_scan->rows = rowz;
					leg_scan->columns = columnz;

					int i = 0, j = 0;
					for(i = 0 ; i < rowz; i++)
						for(j = 0; j < columnz; j++)
							leg_scan->amounts[(i * columnz) + j] = dt_scan->scan_data->pool_data.matrix_data[i][j];
				}

			}

		break;

		default:
		break;
		}
		if(!out) out = ret_ok;
	}

	return out;
}

p_itsp_sequence* load_session_from_log(char* log_file){

	FILE* log_data = open_file(log_file, "r+");
	p_itsp_frame* frz = load_frames(log_data, std_out);
	fclose(log_data);

	FILE* itsp_log_file = fopen("frames.log", "w+");
	print_log_data(itsp_log_file,frz);
	fclose(itsp_log_file);

	itsp_log_file = fopen("res_seq_clustering.log", "w+");
	p_itsp_sequence* seqz = cluster_sequences(itsp_log_file, frz);

	free(frz);

	itsp_log_file = fopen("sequences.log", "w+");
	log_sequences(itsp_log_file,seqz);
	fclose(itsp_log_file);

	load_atts_from_log(seqz);
	load_pools_from_log(seqz);
	load_grep_from_log(seqz);
	load_bets_from_log(seqz);
	load_winning_from_log(seqz);

	p_itsp_sequence seq = NULL;
	p_itsp_frame fr = NULL;
	int i = 0;
	while((seq = seqz[i++])){
		int j = 0;
		while((fr = seq->frames[j++]))
			free_itsp_frame(fr);
		free(seq);
	}
	free(seqz);

	fprintf(std_out,"atts : \n");
	p_s_s3k_attrib p_att = NULL;
	fast_map_data_iterate(p_att, liste_attributaires){
		fprintf(std_out,"%s(%d) : %s [%s - %d.%d]\n",
				p_att->code_att,
				p_att->id_att,
				p_att->att_description,
				p_att->itsp_version.document,
				p_att->itsp_version.version.version_number,
				p_att->itsp_version.version.revision_number);
	}

	fprintf(std_out,"pools : \n");
	p_s_s3k_pool_type p_pool_ty = NULL;
	fast_map_data_iterate(p_pool_ty, liste_paris){
		fprintf(std_out,"%s(%d) : dimz = %02d\n",
				p_pool_ty->code,
				p_pool_ty->id_pg,
				p_pool_ty->dimensions);
	}

	p_s_s3k_grep p_grep = NULL;
	fast_map_data_iterate(p_grep, liste_reunions){
		fprintf(std_out,"event %s(%02d) : Host => %s\n",
				p_grep->name,
				p_grep->nu_grep,
				p_grep->org_att);

		fprintf(std_out,"prog_courses :\n");
		p_s_s3k_race p_race = NULL;
		fast_map_data_iterate(p_race, p_grep->races){
			fprintf(std_out,"race %02d(%c) - %02ld runners:", p_race->number, p_race->status, fast_map_count(p_race->runners));
			if(p_race->start){
				struct tm* t = long_2_tm(&p_race->start);
				fprintf(std_out,"[Start at %02d:%02d:%02d]", t->tm_hour, t->tm_min, t->tm_sec);
			}
			else
				fprintf(std_out,"[No start time]");
			if(p_race->finish){
				struct tm* t = long_2_tm(&p_race->finish->finish_time);
				if(fast_map_count(p_race->finish->finishers)){
					fprintf(std_out,"[finish at %02d:%02d:%02d :", t->tm_hour, t->tm_min, t->tm_sec);
					p_itsp_finish fnsh = NULL;
					fast_map_data_iterate(fnsh, p_race->finish->finishers)
						fprintf(std_out," %02d", fnsh->runner);
					fprintf(std_out,"] - %02ld pools : ", fast_map_count(p_race->pools));
				}
				else
					fprintf(std_out,"[finish at %02d:%02d:%02d : no finishers] - %02ld pools : ", t->tm_hour, t->tm_min, t->tm_sec,
							fast_map_count(p_race->pools));
			}
			else
				fprintf(std_out,"[no finish] - %02ld pools : ", fast_map_count(p_race->pools));
			p_s_s3k_pool pool = NULL;
			fast_map_data_iterate(pool, p_race->pools)
				fprintf(std_out,"%s(%c) ", pool->code, pool->status);
			fprintf(std_out,"\n");
		}

		p_s_s3k_att_conf p_att_conf =  NULL;
		fast_map_data_iterate(p_att_conf, p_grep->col_configs){
			fprintf(std_out,"Collector %s : %ld races, %ld pools_confs\n",
					p_att_conf->att,
					fast_map_count(p_att_conf->abo_races),
					fast_map_count(p_att_conf->pool_configs));
			p_s_s3k_abo_race abo_race = NULL;
			fast_map_data_iterate(abo_race, p_att_conf->abo_races){
				fprintf(std_out,"abo_race nb %d ", abo_race->number);
				char *pool = NULL;
				fprintf(std_out,"[%ld pools] :", sorted_set_count(abo_race->abo_pools));
				sorted_set_iterate(pool, abo_race->abo_pools)
					fprintf(std_out,"%s ", pool);
				fprintf(std_out,"\n");
			}
		}
	}

	p_s_s3k_bets bet = NULL;
	p_s_s3k_race p_race = NULL;
	p_s_s3k_pool pool = NULL;
	char* s = NULL;
	double *ptr = NULL;
	fast_map_data_iterate(p_grep, liste_reunions){
		fast_map_data_iterate(p_race, p_grep->races){
			fast_map_data_iterate(pool, p_race->pools){
				if(fast_map_count(pool->bets)){
					fprintf(std_out,"r%dc%d pool=\"%s\":\n",p_grep->nu_grep, p_race->number, pool->code);
					fast_map_iterate(s, bet, pool->bets){
						fprintf(std_out,"	att : \"%s\", exchange_style : %c, total : %08.2f\n", s, bet->exchange_style, bet->total);
						if((bet->amounts && pool->dimensions) || fast_map_count(bet->scans)){
							if(bet->exchange_style == s_mod_pool){
								ptr = bet->amounts;
								while(ptr - bet->amounts < pool->dimensions[0] * pool->dimensions[1] * pool->dimensions[2]){
									if(!((ptr - bet->amounts) % (pool->dimensions[2] * pool->dimensions[1]))){
										fprintf(std_out,"seg %ld :\n", 1 + (long unsigned int)(ptr - bet->amounts) / (pool->dimensions[2] * pool->dimensions[1]));
									}
									fprintf(std_out,"%09.2f  ", *ptr);
									ptr++;
									if(!((ptr - bet->amounts) % pool->dimensions[2])){
										fprintf(std_out,"\n");
									}
								}
							}else{
								p_s_s3k_bet_scan scan = NULL;
								fast_map_data_iterate(scan, bet->scans){
									fprintf(std_out,"leg %d : \n", scan->leg);
									double *ptr0 = scan->amounts;
									while(ptr0 - scan->amounts < scan->columns * scan->rows){
										fprintf(std_out,"%09.2f  ", *ptr0++);
										if(!((ptr0 - scan->amounts) % scan->columns)){
											fprintf(std_out,"\n");
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return seqz;
}
