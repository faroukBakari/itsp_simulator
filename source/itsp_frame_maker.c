/*
 * itsp_frame_maker.c
 *
 *  Created on: Sep 6, 2017
 *      Author: farouk
 */


 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>
 #include <math.h>

 #include <i_string.h>
 #include <i_file.h>
 #include <s3k_structs.h>
 #include <s3k_session.h>
 #include <itsp_common.h>
 #include <s3k_commun.h>
 #include <itsp_cnx.h>
 #include <itsp_frame_maker.h>

#undef __USE_MISC
#include <sys/stat.h>

int make_itsp_header(p_itsp_frame_type type, char* source, char* grep, int race, char* pool, int seq_nb, long time, p_itsp_header header){
	memcpy(header->frame_type, type, sizeof(itsp_frame_type));
	strcpy(header->source, source ? source : "...");
	strcpy(header->event_code, grep ? grep : "...");
	strcpy(header->pool_code, pool ? pool : "...");
	header->race_number = race;
	header->ldata = 0;
	header->sequence = seq_nb;
	itsp_date_time(time, NULL,&header->time);
	header->header_log = NULL;
	return OK;
}

int make_itsp_data_alert(char*	message, p_itsp_str_alert dt_alert){
	dt_alert->type = *message++;
	dt_alert->message = malloc(strlen(message) + 1);
	strcpy(dt_alert->message, message);
	return OK;
}

int make_itsp_data_config(char* source, char* destination, char* grep, p_itsp_str_data_config dt_cfg, FILE* log_file){

	memset(dt_cfg, 0, sizeof(itsp_str_data_config));

	int *p_race = 0, j = 0;
	p_itsp_str_pool_config p_pool_conf = NULL;

	p_s_s3k_grep p_grep = NULL;
	if(!(p_grep = fast_map_at(liste_reunions, grep))){
		DEBUG_WARN("grep %s not found!", grep);
		return FAULT;
	}
	dt_cfg->mode = strcmp(source, p_grep->org_att) ? itsp_dc_mode_remote : itsp_dc_mode_host;

	dt_cfg->performance = p_grep->performance;

	itsp_date_time(p_grep->date_time, &dt_cfg->date, &dt_cfg->time);

	dt_cfg->calculation = p_grep->calculation;
	dt_cfg->close_bet_delay = p_grep->close_bet_delay;
	dt_cfg->close_cancel_delay = p_grep->close_cancel_delay;

	p_s_s3k_att_conf p_att_conf = NULL;
	char* col_att = (dt_cfg->mode == itsp_dc_mode_host) ? destination : source;

	s_s3k_att_conf col_conf = {0};
	if(!(p_att_conf = fast_map_at(p_grep->col_configs, col_att))){
		TRACE(log_file, "grep %s : no coll_conf for att %s -> making temporary one...\n", grep, col_att);

		memcpy(col_conf.att, col_att, 4);
		col_conf.abo_races = fast_map_init(sizeof(int), sizeof(s_s3k_abo_race), int_key_com);
		col_conf.pool_configs = fast_map_init(4, sizeof(itsp_str_pool_config), string_key_com);

		p_s_s3k_race p_race = NULL;
		p_s_s3k_pool p_pool = NULL;
		if((p_att_conf = fast_map_at(p_grep->col_configs, p_grep->org_att))){
			p_itsp_str_pool_config p_pool_conf = NULL;
			fast_map_data_iterate(p_pool_conf, p_att_conf->pool_configs){
				itsp_str_pool_config pool_conf = {0};
				strcpy(pool_conf.pool_code, p_pool_conf->pool_code);
				// DEFAULT POOL_CONF -> should be updated with remote feedback
				pool_conf.exchange_part = p_pool_conf->exchange_part;
				pool_conf.scan_mode = p_pool_conf->scan_mode;
				pool_conf.send_gross_pool = p_pool_conf->receive_gross_pool;
				pool_conf.receive_gross_pool = p_pool_conf->send_gross_pool;
				pool_conf.send_net_pool = p_pool_conf->receive_net_pool;
				pool_conf.receive_net_pool = p_pool_conf->send_net_pool;
				pool_conf.send_total = p_pool_conf->receive_total;
				pool_conf.receive_total = p_pool_conf->send_total;
				pool_conf.xfer_will_pay = p_pool_conf->xfer_will_pay;
				pool_conf.pool_unit = p_pool_conf->pool_unit;
				pool_conf.min_payoff = p_pool_conf->min_payoff;
				pool_conf.Break = p_pool_conf->Break;
				fast_map_insert(col_conf.pool_configs, p_pool_conf->pool_code, &pool_conf);
			}
		}
		fast_map_data_iterate(p_race, p_grep->races){
			s_s3k_abo_race abo_race = {0};
			abo_race.number = p_race->number;
			abo_race.abo_pools = sorted_set_init(4, string_key_com);
			fast_map_data_iterate(p_pool, p_race->pools){
				if(!fast_map_at(col_conf.pool_configs, p_pool->code)){
					itsp_str_pool_config pool_conf = {0};
					strcpy(pool_conf.pool_code, p_pool->code);
					// DEFAULT POOL_CONF -> should be updated with remote feedback
					pool_conf.exchange_part = 'a';
					pool_conf.scan_mode = (numeric_range_cardinal(p_pool->dimensions) > 3) ? 'P' : 'E';
					pool_conf.send_gross_pool = 'N';
					pool_conf.receive_gross_pool = 'N';
					pool_conf.send_net_pool = 'N';
					pool_conf.receive_net_pool = 'N';
					pool_conf.send_total = 'N';
					pool_conf.receive_total = 'N';
					pool_conf.xfer_will_pay = 'N';
					pool_conf.pool_unit = 1.1;
					pool_conf.min_payoff = 1.1;
					pool_conf.Break = 0.1;
					fast_map_insert(col_conf.pool_configs, p_pool->code, &pool_conf);
				}
			}
			fast_map_insert(col_conf.abo_races, &abo_race.number, &abo_race);
		}

		p_att_conf = &col_conf;
	}

	dt_cfg->race_list.races = fast_map_count(p_att_conf->abo_races);
	j = 0;
	void* not_used = NULL;
	fast_map_iterate(p_race, not_used, p_att_conf->abo_races)
		dt_cfg->race_list.race[j++] = *p_race;
	dt_cfg->nb_pools = fast_map_count(p_att_conf->pool_configs);
	j = 0;
	fast_map_data_iterate(p_pool_conf, p_att_conf->pool_configs)
		memcpy(&dt_cfg->pool_configs[j++], p_pool_conf, sizeof(itsp_str_pool_config));
	return OK;
}

int make_itsp_data_file_header(char* destination, pi_file fp, p_itsp_str_file dt_xfile){

	memset(dt_xfile, 0, sizeof(itsp_str_file));

	strcpy(dt_xfile->file_header.destination, destination);
	strcpy(dt_xfile->file_header.name, fp->name);
	itsp_date_time(fp->attributes.st_mtime, &dt_xfile->file_header.date, &dt_xfile->file_header.time);
	/*
	#ifdef __USE_MISC
		itsp_date_time(attrib.st_mtime, &dt_xfile->file_header.date, &dt_xfile->file_header.time);
	#else
		itsp_date_time(attrib.st_mtim.tv_sec, &dt_xfile->file_header.date, &dt_xfile->file_header.time);
	#endif
	*/

	return OK;
}

int make_itsp_data_file(itsp_mode_type mode, pi_file fp, p_itsp_str_file dt_xfile){

	memset(dt_xfile, 0, sizeof(itsp_str_file));

	dt_xfile->file_header.size = fp->attributes.st_size;
	dt_xfile->file_header.segments = (dt_xfile->file_header.size % FILE_SEG_SIZE) ? (dt_xfile->file_header.size / FILE_SEG_SIZE) + 1 : dt_xfile->file_header.size / FILE_SEG_SIZE;

	if(!remaining_file_data(fp->header))
		fseek(fp->header, 0, SEEK_SET);

	dt_xfile->file_header.current_segment = (ftell(fp->header) / FILE_SEG_SIZE) + 1;

	strcpy(dt_xfile->file_header.destination, (mode == mode_host) ? ".H." : ".R.");
	strcpy(dt_xfile->file_header.name, fp->name);
	itsp_date_time(fp->attributes.st_mtime, &dt_xfile->file_header.date, &dt_xfile->file_header.time);
	/*
	#ifdef __USE_MISC
		itsp_date_time(attrib.st_mtime, &dt_xfile->file_header.date, &dt_xfile->file_header.time);
	#else
		itsp_date_time(attrib.st_mtim.tv_sec, &dt_xfile->file_header.date, &dt_xfile->file_header.time);
	#endif
	*/

	dt_xfile->file_header.segment_size = (dt_xfile->file_header.current_segment == dt_xfile->file_header.segments) ? remaining_file_data(fp->header) : FILE_SEG_SIZE;

	dt_xfile->file_data = malloc(dt_xfile->file_header.segment_size + 1);
	get_chars(dt_xfile->file_data, dt_xfile->file_header.segment_size, fp->header);
	dt_xfile->file_data[dt_xfile->file_header.segment_size] = 0;

	return OK;
}

int make_itsp_data_link(char* att, p_itsp_str_data_link dt_ln){

	memset(dt_ln, 0, sizeof(itsp_str_data_link));
	p_s_s3k_attrib p_att = fast_map_at(liste_attributaires, att);
	if(!p_att){ // for debug test only uncomment after
		DEBUG_WARN("unknown att \"%s\"", att);
		abort();
	}
	else{
		strcpy(dt_ln->identifier.document, p_att->itsp_version.document);
		dt_ln->identifier.version.revision_number = p_att->itsp_version.version.revision_number;
		dt_ln->identifier.version.version_number = p_att->itsp_version.version.version_number;
		if(p_att->att_description){
			dt_ln->text = malloc(strlen(dt_ln->text) + 1);
			strcpy(dt_ln->text, p_att->att_description);
		}
	}
	return OK;
}

int make_itsp_dt_rs(char* grep, int race, p_itsp_str_data_race_status ptr, itsp_msg_reason reason){

	if(race && reason != reason_end){
		int i = 0, j = 0;
		p_s_s3k_pool p_pool = NULL;
		memset(ptr, 0, sizeof(itsp_str_data_race_status));

		p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
		if(!p_grep){
			DEBUG_WARN("grep \"%s\" not found in list", grep);
			abort();
		}

		p_s_s3k_race p_race = NULL;
		if(!(p_race = fast_map_at(p_grep->races, &race))){
			DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
			abort();
		}

		ptr->status = p_race->status;
		itsp_date_time(p_race->start, NULL, &ptr->post_time);
		ptr->display = p_race->display;

		/*******************runner status**********************/
		ptr->runners = fast_map_count(p_race->runners);
		ptr->runner_status[ptr->runners] = 0;
		p_s_s3k_runner i_runner = NULL;
		fast_map_data_iterate(i_runner, p_race->runners)
			ptr->runner_status[i++] = i_runner->status;
		/*******************bracket status**********************/
		ptr->brackets = sorted_vect_count(p_race->brackets);
		p_s_s3k_bracket brk = NULL;
		i = 0;
		sorted_vect_iterate(brk, p_race->brackets)
			ptr->bracket_status[i++] = brk->runner_list;
		/*******************pool status**********************/
		ptr->pools = fast_map_count(p_race->pools);
		i = 0;
		fast_map_data_iterate(p_pool, p_race->pools){
			strcpy(ptr->pool_def[i].pool_code, p_pool->code);
			ptr->pool_def[i].pool_status = p_pool->status;

			j = 0;
			while(p_pool->race_list[j])
				j++;
			ptr->pool_def[i].race_list = malloc(j-- * sizeof(int));
			ptr->pool_def[i].race_list[j] = 0;
			while(j--)
				ptr->pool_def[i].race_list[j] = p_pool->race_list[j + 1];

			ptr->pool_def[i].min_bet = p_pool->min_bet;
			ptr->pool_def[i].net_addin = p_pool->net_addin;
			ptr->pool_def[i].gross_addin = p_pool->gross_addin;
			i++;
		}
		/*******************scratch pools**********************/
		ptr->scratch_pools = fast_map_count(p_race->pools);
		i = 0;
		fast_map_data_iterate(p_pool, p_race->pools){
			ptr->scratch_info[i].race = p_race->number;
			strcpy(ptr->scratch_info[i].pool_code, p_pool->code);
			ptr->scratch_info[i].pool_runners = clone_combo(p_pool->scratch_info);
			i++;
		}
	}
	return OK;
}

int make_itsp_data_pool(char* source, char* destination, char* grep, int race, char* pool, int segment, p_itsp_str_data_pools ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris, pool);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
	if(!p_pool_typ || !p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}

	p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, strcmp(p_grep->org_att, source) ? source : destination);
	if(!att_conf){
		DEBUG_WARN("no col_conf for att \"%s\" in grep \"%s\"", strcmp(p_grep->org_att, source) ? source : destination, grep);
		abort();
	}

	p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, pool);
	if(!pool_conf){
		DEBUG_WARN("no \"%s\" pool_conf for att \"%s\" in grep \"%s\"", pool, source, grep);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_data_pools));

	ptr->pool_header.pool_mode = p_grep->calculation == standard_pool_calculation ? gross_pool_mode : net_pool_mode;
	ptr->pool_header.segments = p_pool->dimensions ? p_pool->dimensions[0] : 1;
	if(segment && (ptr->pool_header.segments < segment)){
		DEBUG_WARN("segment number error (%d). should be in [1 - %d] for pool \"%s\"", segment, p_pool->dimensions[0], pool);
		abort();
	}
	ptr->pool_header.segment = segment ? segment : 1;

	ptr->pool_data.rows = 0;
	ptr->pool_data.columns = 0;

	ptr->pool_data.segment_total = 0.0;

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, source);
	if(bets){
		if(!bets->exchange_style) bets->exchange_style = pool_conf->scan_mode;
		if(bets->exchange_style != s_mod_pool){
			DEBUG_WARN("exchange_style for pool \"%s\" is '%c'(not data_pool 'P')", pool, bets->exchange_style);
			abort();
		}

		if(p_pool_typ->dimensions > 3){
			DEBUG_WARN("\"%s\" pool dimension exceeds 3(%d) -> cannot exchange via data_pool", pool, p_pool_typ->dimensions);
			abort();
		}

		if(bets->amounts && p_pool->dimensions && (bets->total || bets->net_total)){
			ptr->pool_data.rows = p_pool->dimensions[1];
			ptr->pool_data.columns = p_pool->dimensions[2];

			ptr->pool_data.matrix_data = malloc((ptr->pool_data.rows + 1) * sizeof(double*));
			ptr->pool_data.matrix_data[ptr->pool_data.rows] = NULL;
			int ctr = (segment - 1) * ptr->pool_data.rows * ptr->pool_data.columns;
			for(int i = 0 ; i < ptr->pool_data.rows; i++){
				ptr->pool_data.matrix_data[i] = malloc((ptr->pool_data.columns + 1) * sizeof(double));
				ptr->pool_data.matrix_data[i][ptr->pool_data.columns] = (double)0;
				for(int j = 0; j < ptr->pool_data.columns; j++){
					ptr->pool_data.matrix_data[i][j] = bets->amounts[ctr];
					ptr->pool_data.segment_total += bets->amounts[ctr++];
				}
			}
			if(!ptr->pool_data.segment_total){
				for(int i = 0 ; i < ptr->pool_data.rows; i++)
					free(ptr->pool_data.matrix_data[i]);
				free(ptr->pool_data.matrix_data);
				ptr->pool_data.matrix_data = NULL;
				ptr->pool_data.rows = ptr->pool_data.columns = 0;
			}
		}


		ptr->pool_data.total = bets->total;
		ptr->pool_data.net_total = bets->net_total;
	}

	return OK;
}

int itsp_make_dt_pool_header(char* att, char* grep, int race, char* pool, int leg, int segment, p_itsp_str_data_pools ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
	if(!p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}

	p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, att);
	if(!att_conf){
		DEBUG_WARN("no col_conf for att \"%s\" in grep \"%s\"",att, grep);
		abort();
	}

	p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, pool);
	if(!pool_conf){
		DEBUG_WARN("no \"%s\" pool_conf for att \"%s\" in grep \"%s\"", pool, att, grep);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_data_pools));

	ptr->pool_header.pool_mode = p_grep->calculation == net_pool_calculation  ?
					(
						strcmp(p_grep->org_att, att) ?
								gross_pool_mode
								: pool_conf->send_net_pool != when_never ? net_pool_mode : gross_pool_mode
					)
					: gross_pool_mode;
	ptr->pool_header.segments = p_pool->dimensions ? p_pool->dimensions[0] : 1;
	ptr->pool_header.segment = segment;
	ptr->pool_header.leg = leg;

	return OK;
}

int itsp_make_dt_scan_ack(char* att, char* grep, int race, char* pool, int leg, p_itsp_str_data_pools ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
	if(!p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}

	p_s_s3k_att_conf att_conf = fast_map_at(p_grep->col_configs, att);
	if(!att_conf){
		DEBUG_WARN("no col_conf for att \"%s\" in grep \"%s\"",att, grep);
		abort();
	}

	p_itsp_str_pool_config pool_conf = fast_map_at(att_conf->pool_configs, pool);
	if(!pool_conf){
		DEBUG_WARN("no \"%s\" pool_conf for att \"%s\" in grep \"%s\"", pool, att, grep);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_data_pools));

	//exception --> for data scan ack
	p_s_s3k_bets bets = fast_map_at(p_pool->bets, att);
	if(!bets){
		DEBUG_WARN("no bets found => scans were not correctly integrated");
		abort();
	}
	p_s_s3k_bet_scan scans = fast_map_at(bets->scans, &leg);
	if(!scans){
		DEBUG_WARN("no scans found => scans were not correctly integrated");
		abort();
	}

	ptr->pool_data.segment_total = scans->scan_total;

	return OK;
}

int make_itsp_dt_ack_pool(char* destination, char* grep, int race, char* pool, int segment, p_itsp_str_data_pools ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris, pool);
	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
	if(!p_pool_typ || !p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_data_pools));

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, destination);
	if(bets){
		if(bets->exchange_style != s_mod_pool){
			DEBUG_WARN("exchange_style for pool \"%s\" is '%c'(not data_pool 'P')", pool, bets->exchange_style);
			abort();
		}

		if(p_pool_typ->dimensions > 3){
			DEBUG_WARN("\"%s\" pool dimension exceeds 3(%d) -> cannot exchange via data_pool", pool, p_pool_typ->dimensions);
			abort();
		}

		if(bets->amounts && p_pool->dimensions && (bets->total || bets->net_total)){
			if(!segment || (segment > p_pool->dimensions[0])){
				DEBUG_WARN("segment number error (%d). should be in [1 - %d] for pool \"%s\"", segment, p_pool->dimensions[0], pool);
				abort();
			}

			int ctr = (segment - 1) * p_pool->dimensions[1] * p_pool->dimensions[2];
			for(int i = 0 ; i < p_pool->dimensions[1]; i++)
				for(int j = 0; j < p_pool->dimensions[2]; j++)
					ptr->pool_data.segment_total += bets->amounts[ctr++];

		}
	}

	return OK;
}

int make_itsp_data_total(char* source, char* grep, int race, char* pool, p_itsp_str_data_totals ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool p_pool = fast_map_at(p_race->pools, pool);
	if(!p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_data_pools));

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, source);
	if(bets){
		ptr->live_total = bets->total - bets->refund_total;
		ptr->net_total  = bets->net_total;
	}

	return OK;
}

int make_itsp_data_payoff(char* grep, int race, char* pool, char* destination, p_itsp_str_data_payoffs ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
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

	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, destination);
	if(!p_att_conf){
		DEBUG_WARN("no att_conf for att \"%s\" on grep \"%s\"", destination, grep);
		abort();
	}

	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);
	if(!p_pool_conf){
		DEBUG_WARN("no \"%s\" pool_conf for att \"%s\"", pool, destination);
		abort();
	}


	memset(ptr, 0, sizeof(itsp_str_data_payoffs));


	ptr->live_runners = malloc((p_pool_typ->dimensions + 1) * sizeof(int*));
	memset(ptr->live_runners, 0, (p_pool_typ->dimensions + 1) * sizeof(int*));
	int k = 0, i = 0, j = 0;
	p_s_s3k_race p_race_i = NULL;
	p_s_s3k_runner runner_j = NULL;
	if(p_pool_typ->nb_races == 1){
		fill_live_runners_mono :
		j = 0;
		fast_map_data_iterate(runner_j, p_race->runners){
			if(('1' <= runner_j->status) && (runner_j->status <= '9')){
				if(ptr->live_runners[i])
					ptr->live_runners[i][j] = runner_j->number;
				j++;
			}
		}
		if(!ptr->live_runners[0]){
			ptr->live_runners[0] = malloc((j + 1) * sizeof(int));
			memset(ptr->live_runners[0], 0, (j + 1) * sizeof(int));
			goto fill_live_runners_mono;
		}

		if(p_pool_typ->dimensions > 1){
			i = 1;
			while(i < p_pool_typ->dimensions)
				ptr->live_runners[i++] = ptr->live_runners[0];
		}

	}
	else{
		if(p_pool_typ->dimensions == p_pool_typ->nb_races){
			while(p_pool->race_list[k]){
				p_race_i = fast_map_at(p_grep->races, &p_pool->race_list[k]);
				fill_live_runners_multi :
				j = 0;
				fast_map_data_iterate(runner_j, p_race_i->runners){
					if(('1' <= runner_j->status) && (runner_j->status <= '9')){
						if(ptr->live_runners[i])
							ptr->live_runners[i][j] = runner_j->number;
						j++;
					}
				}
				if(!ptr->live_runners[i]){
					ptr->live_runners[i] = malloc((j + 1) * sizeof(int));
					memset(ptr->live_runners[i], 0, (j + 1) * sizeof(int));
					goto fill_live_runners_multi;
				}
				i++;
			}
		}
		else{
			DEBUG_WARN("Unhandled dimensions case for pool \"%s\" where p_pool_typ->dimensions(%d) != p_pool_typ->nb_races(%d)", pool, p_pool_typ->dimensions, p_pool_typ->nb_races);
			abort();
		}
	}

	double total = 0.0;
	p_s_s3k_bets p_bets = fast_map_at(p_pool->bets, destination);
	if(p_bets){
		ptr->refund = p_bets->refund_total;
		total = p_bets->total;
	}

	ptr->carry_over = p_pool->jackpot;

	ptr->nb_commissions = p_pool_conf->nb_commissions;

	if(ptr->nb_commissions){
		ptr->commissions = malloc(ptr->nb_commissions * sizeof(double));
		memset(ptr->commissions, 0, ptr->nb_commissions * sizeof(double));
		int i = ptr->nb_commissions;
		while(i--){
			ptr->commissions[i] = p_pool_conf->commissions[i].rate * total;
			switch(p_pool_conf->commissions[i].rounding){
			case 'U' :
				ptr->commissions[i] = ceil(ptr->commissions[i] * 10000.0) / 10000.0;
				break;
			case 'D' :
				ptr->commissions[i] = floor(ptr->commissions[i] * 10000.0) / 10000.0;
				break;
			case 'R' :
				ptr->commissions[i] = rint(ptr->commissions[i] * 10000.0) / 10000.0;
				break;
			default :
				DEBUG_WARN("Unknown rounding mode for pool \"%s\" for att \"%s\"", pool, destination);
				abort();
			}
		}
	}

	p_s_fast_map p_payoffs_att = fast_map_at(p_pool->payoffs, destination);
	if(p_payoffs_att){
		ptr->nb_prices = fast_map_count(p_payoffs_att);
		ptr->price_definition = malloc((ptr->nb_prices + 1) * sizeof(itsp_price_definition));
		memset(ptr->price_definition, 0, (ptr->nb_prices + 1) * sizeof(itsp_price_definition));
		p_s_s3k_payoffs p_payoff = NULL;
		i = 0;
		fast_map_data_iterate(p_payoff, p_payoffs_att){
			//A revoir avec les differents types de payoffs
			ptr->price_definition[i].order_flag = p_payoff->order_flag;
			ptr->price_definition[i].price = p_payoff->win_coef;
			ptr->price_definition[i].runner_list = clone_combo(p_payoff->win_combo);
			ptr->price_definition[i].status = p_payoff->status;
			ptr->price_definition[i].winners = p_payoff->least_combo_match;
			switch(p_payoff->status){
			case price_normal :
			case price_refund :
			case price_exchange :
			case price_pay_exchange :
			case price_alternate :
			case price_itq_consolation :
			case price_itq_pay_exch :
			case price_partial_refund :
				ptr->price_definition[i].winning = 0.0;
				ptr->price_definition[i].liability = 0.0;
				ptr->price_definition[i].break_amount = 0.0;
				break;
			default :
				TRACE(stderr, "combo : ");
				print_combo(stderr, p_payoff->win_combo);
				DEBUG_WARN("\n--> Unknown price status '%c' for pool \"%s\" for att \"%s\"", p_payoff->status, pool, destination);
				abort();
			}
			i++;
		}
	}
	return OK;
}

int make_itsp_data_will_pay(char* grep, int race, char* pool, char* destination, p_itsp_str_will_pay ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_pool_type p_pool_typ = fast_map_at(liste_paris, pool);
	if(!p_pool_typ){
		DEBUG_WARN("unknown pool_code \"%s\"", pool);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	race -= p_pool_typ->nb_races - 1;

	p_s_s3k_race p_race_i = fast_map_at(p_grep->races, &race);
	if(!p_race_i){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	p_s_s3k_pool p_pool = fast_map_at(p_race_i->pools, pool);
	if(!p_pool){
		DEBUG_WARN("pool \"%s\" not found in race %d of grep \"%s\"", pool, race, grep);
		abort();
	}

	p_s_fast_map att_will_pay = fast_map_at(p_pool->will_pay, destination);
	if(!att_will_pay){
		DEBUG_WARN("no \"%s\" will_pay for pool \"%s\" of R%dC%d", destination, pool, p_grep->nu_grep, p_race->number);
		abort();
	}


	memset(ptr, 0, sizeof(itsp_str_will_pay));

	ptr->rows = fast_map_count(att_will_pay);
	ptr->columns = fast_map_count(p_race->runners);

	ptr->will_pay_row = malloc(ptr->rows * sizeof(itsp_will_pay_row));

	p_itsp_will_pay_row p_row = NULL;
	int ** combo = NULL, i = 0;

	fast_map_iterate(combo, p_row, att_will_pay){
		ptr->will_pay_row[i].runner_list = clone_combo(combo);
		ptr->will_pay_row[i].items = malloc(ptr->columns * sizeof(itsp_will_pay_item));
		memcpy(ptr->will_pay_row[i].items, p_row, ptr->columns * sizeof(itsp_will_pay_item));
		i++;
	}

	return OK;
}

int make_itsp_scan_req_begin(char* grep, int race, char* pool, char* destination, p_s_s3k_bet_scan ps_scan, p_itsp_str_scan_header ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
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

	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, destination);
	if(!p_att_conf){
		DEBUG_WARN("no att_conf for att \"%s\" on grep \"%s\"", destination, grep);
		abort();
	}

	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);
	if(!p_pool_conf){
		DEBUG_WARN("no \"%s\" pool_conf for att \"%s\"", pool, destination);
		abort();
	}

	if(combo_cardinal(ps_scan->combo) != p_pool_typ->dimensions){
		char combo__[256] = {0};
		write_combo(combo__, ps_scan->combo);
		DEBUG_WARN("passed scan combo %s dose not match pool \"%s\" dimensions -> %d", combo__, pool, p_pool_typ->dimensions);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_scan_header));

	ptr->scan_mode = p_pool_conf->scan_mode;
	ptr->combos = 1;

	ptr->scan_runners = clone_combo(ps_scan->combo);
	ptr->live_runners = clone_combo(p_pool->scratch_info);
	ptr->leg = ps_scan->leg;

	ptr->favorite_runners = itsp_make_favorite_runners(p_grep, p_pool_typ, p_pool, ptr->leg);

	if(!p_pool->sub_runners)
		p_pool->sub_runners = make_empty_combo(p_pool_typ->nb_races);
	ptr->sub_runners = clone_combo(p_pool->sub_runners);

	return OK;

}

int make_itsp_data_scan(char* grep, int race, char* pool, char* source, int leg, p_itsp_str_data_scan dt_scan){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
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

	p_s_s3k_bets bets = fast_map_at(p_pool->bets, source);
	if(!bets){
		DEBUG_WARN("no \"%s\" bets for att \"%s\" in race %d of grep \"%s\"", pool, source, race, grep);
		abort();
	}

	p_s_s3k_bet_scan ps_scan = fast_map_at(bets->scans, &leg);
	if(!ps_scan){
		DEBUG_WARN("no \"%s\" scans for att \"%s\" in race %d of grep \"%s\"", pool, source, race, grep);
		abort();
	}

	p_s_s3k_att_conf p_att_conf = fast_map_at(p_grep->col_configs, source);
	if(!p_att_conf){
		DEBUG_WARN("no att_conf for att \"%s\" on grep \"%s\"", source, grep);
		abort();
	}

	p_itsp_str_pool_config p_pool_conf = fast_map_at(p_att_conf->pool_configs, pool);
	if(!p_pool_conf){
		DEBUG_WARN("no \"%s\" pool_conf for att \"%s\"", pool, source);
		abort();
	}

	if(p_pool_conf->scan_mode == s_mod_pool){
		DEBUG_WARN("bets for \"%s\" should be exchanged via data_pools", pool);
		abort();
	}

	p_itsp_str_scan_header ptr = dt_scan->scan_header = malloc(sizeof(itsp_str_scan_header));

	memset(ptr, 0, sizeof(itsp_str_scan_header));

	ptr->scan_mode = p_pool_conf->scan_mode;
	ptr->combos = 1;
	ptr->scan_runners = clone_combo(ps_scan->combo);
	ptr->live_runners = itsp_make_live_runner(p_pool_typ, p_pool);
	ptr->leg = leg;

	ptr->favorite_runners = itsp_make_favorite_runners(p_grep, p_pool_typ, p_pool, leg);

	if(!p_pool->sub_runners)
		p_pool->sub_runners = make_empty_combo(p_pool_typ->nb_races);
	ptr->sub_runners = clone_combo(p_pool->sub_runners);


	p_itsp_str_data_pools ptr1 = dt_scan->scan_data = malloc(sizeof(itsp_str_data_pools));

	memset(ptr1, 0, sizeof(itsp_str_data_pools));

	ptr1->pool_header.pool_mode = bets->amounts_type;
	ptr1->pool_header.segments = 1;
	ptr1->pool_header.segment = 1;
	ptr1->pool_header.leg = leg;

	ptr1->pool_data.segment_total = 0.0;

	ptr1->pool_data.total = bets->total;
	ptr1->pool_data.net_total = (p_grep->calculation == 'N') ? bets->net_total : 0;

	dt_scan->scan_total = ps_scan->scan_total;
	dt_scan->live_total = ps_scan->live_total;


	ptr1->pool_data.rows = ps_scan->rows;
	ptr1->pool_data.columns = ps_scan->columns;

	if(ps_scan->columns*ps_scan->rows){
		ptr1->pool_data.matrix_data = malloc(ptr1->pool_data.rows * sizeof(double*));
		int ctr = 0;
		for(int i = 0 ; i < ptr1->pool_data.rows; i++){
			ptr1->pool_data.matrix_data[i] = malloc(ptr1->pool_data.columns * sizeof(double));
			for(int j = 0; j < ptr1->pool_data.columns; j++){
				ptr1->pool_data.matrix_data[i][j] = ps_scan->amounts[ctr];
				ptr1->pool_data.segment_total += ps_scan->amounts[ctr];
				ctr++;
			}
		}
	}

	return OK;
}

int make_itsp_data_result(char* grep, int race, p_itsp_str_data_results ptr){

	p_s_s3k_grep p_grep = fast_map_at(liste_reunions, grep);
	if(!p_grep){
		DEBUG_WARN("grep \"%s\" not found in list", grep);
		abort();
	}

	p_s_s3k_race p_race = fast_map_at(p_grep->races, &race);
	if(!p_race){
		DEBUG_WARN("race %d not found in grep \"%s\"", race, grep);
		abort();
	}

	memset(ptr, 0, sizeof(itsp_str_data_payoffs));

	ptr->nb_finshers = fast_map_count(p_race->finish->finishers);

	if(ptr->nb_finshers){
		ptr->finishs = malloc(ptr->nb_finshers * sizeof(itsp_finish));
		p_itsp_finish p_fnsh = NULL;
		int i = ptr->nb_finshers;
		fast_map_data_iterate(p_fnsh, p_race->finish->finishers)
			memcpy(&ptr->finishs[--i], p_fnsh, sizeof(itsp_finish));
	}

	int i = 0;
	while(p_race->favorite_list[i++])
		continue;
	ptr->favorite_list = malloc(i * sizeof(int));
	while(i--)
		ptr->favorite_list[i] = p_race->favorite_list[i];

	return OK;
}

int make_itsp_frame(p_s_itsp_cnx cnx, p_action_sequence seq, p_itsp_action action, p_itsp_frame fr){

	itsp_frame_type fr_type__ = {seq->data_type, action->message, action->reason}, *fr_type = &fr_type__;
	int seq_num = seq->number, race = seq->race;
	char *pool = seq->pool;
	void* special_data = seq->special_data;

	p_s_s3k_grep p_grep = NULL;
	char *source = NULL, *destination = NULL;
	int out = FAULT;

	p_grep = fast_map_at(liste_reunions, cnx->grep);
	if(!p_grep){
		DEBUG_TOFILE(cnx->log_file, "no grep with code \"%s\" found on the grep list", cnx->grep);
		return out;
	}

	if(cnx->itsp_mode == mode_remote){
		source = cnx->col_att;
		destination = p_grep->org_att;
	}else{
		source = p_grep->org_att;
		destination = cnx->col_att;
	}

	memset(fr, 0, sizeof(itsp_frame));
	fr->header = malloc(sizeof(itsp_header));
	memset(fr->header, 0, sizeof(itsp_header));

	if((fr_type->message == msg_acknowledge)
		&& (65 <= fr_type->reason) && (fr_type->reason <= 90)){
		out = make_itsp_header(fr_type, source, cnx->grep, race, pool, seq_num, 0, fr->header);
		if(special_data){
			char* msg = special_data;
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(strlen(msg) + 1);
			strcpy(fr->data_str->structure, msg);
		}
		return out;
	}

	switch(fr_type->data){
	case data_link :
		if(fr_type->reason != reason_end) fr_type->reason = cnx->itsp_mode;
		out = make_itsp_header(fr_type, source, cnx->grep, 0, NULL, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_data :
		case msg_acknowledge :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_link));
			make_itsp_data_link(source, fr->data_str->structure);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_config :
		out = make_itsp_header(fr_type, source, cnx->grep, 0, NULL, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_pending :
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_config));
			out = make_itsp_data_config(source, destination, cnx->grep, fr->data_str->structure, cnx->log_file);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_race_status :
		out = make_itsp_header(fr_type, source, cnx->grep, race, NULL, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_pending :
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			if(fr_type->reason != reason_end){
				fr->data_str = malloc(sizeof(itsp_data_struct));
				memset(fr->data_str, 0, sizeof(itsp_data_struct));
				fr->data_str->frame_type = fr->header->frame_type;
				fr->data_str->structure = malloc(sizeof(itsp_str_data_race_status));
				out = make_itsp_dt_rs(cnx->grep, race, fr->data_str->structure, fr->header->frame_type->reason);
			}
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_pools :
		out = make_itsp_header(fr_type, source, cnx->grep, race, pool, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_pending :
			if((fr_type->reason != reason_final) || pool){
				fr->data_str = malloc(sizeof(itsp_data_struct));
				memset(fr->data_str, 0, sizeof(itsp_data_struct));
				fr->data_str->frame_type = fr->header->frame_type;
				fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
				out = itsp_make_dt_pool_header(source, cnx->grep, race, pool, 1, *(int*)special_data, fr->data_str->structure);
			}
			break;
		case msg_request :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
			out = itsp_make_dt_pool_header(destination, cnx->grep, race, pool, 1, *(int*)special_data, fr->data_str->structure);
			break;
		case msg_acknowledge :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
			out = make_itsp_dt_ack_pool(destination, cnx->grep, race, pool, *(int*)special_data, fr->data_str->structure);
			break;
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
			out = make_itsp_data_pool(source, destination, cnx->grep, race, pool, *(int*)special_data, fr->data_str->structure);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_scan :
		out = make_itsp_header(fr_type, source, cnx->grep, race, pool, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_pending :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
			itsp_make_dt_pool_header(source, cnx->grep, race, pool, *(int*)special_data, 1, fr->data_str->structure);
			break;
		case msg_request :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			if(fr_type->reason == reason_begin){
				fr->data_str->structure = malloc(sizeof(itsp_str_scan_header));
				out = make_itsp_scan_req_begin(cnx->grep, race, pool, destination, special_data, fr->data_str->structure);
			}
			else{
				fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
				out = itsp_make_dt_pool_header(destination, cnx->grep, race, pool, *(int*)special_data, 1, fr->data_str->structure);
			}
			break;
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_scan));
			out = make_itsp_data_scan(cnx->grep, race, pool, source, *(int*)special_data, fr->data_str->structure);
			break;
		case msg_acknowledge :
			if(action->reason != reason_begin){
				if(special_data){
					fr->data_str = malloc(sizeof(itsp_data_struct));
					memset(fr->data_str, 0, sizeof(itsp_data_struct));
					fr->data_str->frame_type = fr->header->frame_type;
					fr->data_str->structure = malloc(sizeof(itsp_str_data_pools));
					out = itsp_make_dt_scan_ack(destination, cnx->grep, race, pool, *(int*)special_data, fr->data_str->structure);
				}
			}
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_pool_total :
		out = make_itsp_header(fr_type, source, cnx->grep, race, pool, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_totals));
			out = make_itsp_data_total(source, cnx->grep, race, pool, fr->data_str->structure);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_payoffs :
		out = make_itsp_header(fr_type, source, cnx->grep, race, pool, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_data :
			if((fr_type->reason != reason_begin) && (fr_type->reason != reason_end)){
				fr->data_str = malloc(sizeof(itsp_data_struct));
				memset(fr->data_str, 0, sizeof(itsp_data_struct));
				fr->data_str->frame_type = fr->header->frame_type;
				fr->data_str->structure = malloc(sizeof(itsp_str_data_payoffs));
				out = make_itsp_data_payoff(cnx->grep, race, pool, destination, fr->data_str->structure);
			}
			break;
		case msg_acknowledge :
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_results :
		out = make_itsp_header(fr_type, source, cnx->grep, race, NULL, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_data_results));
			out = make_itsp_data_result(cnx->grep, race, fr->data_str->structure);
			break;
		case msg_acknowledge :
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_alert :
		out = make_itsp_header(fr_type, source, cnx->grep, 0, NULL, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_data :
		case msg_acknowledge :
			if(special_data){
				fr->data_str = malloc(sizeof(itsp_data_struct));
				memset(fr->data_str, 0, sizeof(itsp_data_struct));
				fr->data_str->frame_type = fr->header->frame_type;
				fr->data_str->structure = malloc(sizeof(itsp_str_alert));
				out = make_itsp_data_alert((char*)special_data, fr->data_str->structure);
			}
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_will_pay :
		out = make_itsp_header(fr_type, source, cnx->grep, race, pool, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_pending :
		case msg_request :
		case msg_acknowledge :
			break;
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_will_pay));
			out = make_itsp_data_will_pay(cnx->grep, race, pool, destination, fr->data_str->structure);
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	case data_file_transfert :
		out = make_itsp_header(fr_type, source, cnx->grep, 0, NULL, seq_num, 0, fr->header);
		switch(fr_type->message){
		case msg_pending :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_file));
			out = make_itsp_data_file_header((cnx->itsp_mode == mode_host) ? ".H." : ".R.", special_data, fr->data_str->structure);
			break;
		case msg_request :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_file));
			out = make_itsp_data_file_header((cnx->itsp_mode == mode_host) ? ".R." : ".H.", special_data, fr->data_str->structure);
			break;
		case msg_data :
			fr->data_str = malloc(sizeof(itsp_data_struct));
			memset(fr->data_str, 0, sizeof(itsp_data_struct));
			fr->data_str->frame_type = fr->header->frame_type;
			fr->data_str->structure = malloc(sizeof(itsp_str_file));
			out = make_itsp_data_file(cnx->itsp_mode, special_data, fr->data_str->structure);
			break;
		case msg_acknowledge :
			break;
		default :
			DEBUG_WARN("unhandled frame_type : D:%c M:%c", fr_type->data, fr_type->message);
			abort();
		}
		break;
	}

	return out;
}














