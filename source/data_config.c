/*
 * data_config.c
 *
 *  Created on: 9 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>


//										// 1 - data_config
char* data_config_read_template 	=	"([HR])" 						// 1 - mode
										"([SN])"						// 2 - calculation
										"([0-9]{2}[0-9]{2}[0-9]{4})"	// 3 - date
										"([0-9]{2}[0-9]{2}[0-9]{2})"	// 4 - time
										"([0-9]{4})"					// 5 - performance
										"([0-9]{2})"					// 6 - close bet delay
										"([0-9]{2})"					// 7 - close cancel delay
										"([0-9]{2})"					// 8 - nb_races
										"([0-9]+)"						// 9 - race list
										"([0-9]{2})";					// 10 - nb_pools

char* data_config_write_template 	="	mode : %c			|"
										"	calculation : %c\n"
										"	date : %02d-%02d-%04d	|"
										"	time : %02d:%02d:%02d\n"
										"	close bet delay : %d	|"
										"	close cancel delay : %d\n"
										"	performance :	%d	|"
										"	races : [";

//										// 1.1 - pool_config
char* pool_config_read_template 	= "^(.{3})"						// 1 - pool code
										"([a-z])"						// 2 - exchange part
										"([ELSXQACKP])"					// 3 - scan mode
										"([NFA]{7})"					// 4 - when values
										"(([0-9,P-Y]*[@-I,`-i]){3})"	// 5 - pool unit, min payoff & break
										"([0-9])"						// 7 - nb_commissions
										"([0-9,P-Y]*[@-I,`-i][UDR])*";	// 8 - commissions*/

int data_config_read_frame(char* log, p_itsp_data_struct* data){

	int i = 0, j = 0;

	p_itsp_str_data_config ptr = (*data)->structure = malloc(sizeof(itsp_str_data_config));
	ptr->mode = *log++;
	ptr->calculation = *log++;
	//date
	log = read_date(log, &ptr->date);
	//time
	log = read_time(log, &ptr->time);
	//perf
	log = read_digits(log, &ptr->performance, 4);
	//close_bet_delay
	log = read_digits(log, &ptr->close_bet_delay, 2);
	//close_cancel_delay
	log = read_digits(log, &ptr->close_cancel_delay, 2);
	//race_list
	log = read_digits(log, &ptr->race_list.races, 2);


	i = 0;
	while(i < ptr->race_list.races)
		log = read_digits(log, &ptr->race_list.race[i++], 2);

	//pool_config_list
	log = read_digits(log, &ptr->nb_pools, 2);

	for(i = 0 ; i < ptr->nb_pools ; i++){

		log = read_fixed(log, ptr->pool_configs[i].pool_code, 3);
		ptr->pool_configs[i].exchange_part = *log++;
		ptr->pool_configs[i].scan_mode = *log++;

		ptr->pool_configs[i].send_gross_pool 	= *log++;
		ptr->pool_configs[i].receive_gross_pool = *log++;
		ptr->pool_configs[i].send_net_pool 		= *log++;
		ptr->pool_configs[i].receive_net_pool 	= *log++;
		ptr->pool_configs[i].send_total 		= *log++;
		ptr->pool_configs[i].receive_total 		= *log++;
		ptr->pool_configs[i].xfer_will_pay 		= *log++;

		log = read_amount(log, &ptr->pool_configs[i].pool_unit);
		log = read_amount(log , &ptr->pool_configs[i].min_payoff);
		log  = read_amount(log , &ptr->pool_configs[i].Break);
		log = read_digits(log, &ptr->pool_configs[i].nb_commissions, 1);
		for(j = 0 ; j < ptr->pool_configs[i].nb_commissions ; j++){
			log  = read_amount(log , &ptr->pool_configs[i].commissions[j].rate);
			ptr->pool_configs[i].commissions[j].rounding = *log ++;
		}
	}
	return OK;
}

char* data_config_read_from(char* log, p_itsp_data_struct* data){
	char tmp[6], *s = NULL, *data_start = NULL;
	memset(tmp,0,6);
	int i = 0, j = 0;
	const sub_str* segments = NULL;

	segments = match_expr(log, data_config_read_template, 0);
	if(segments){
		data_start = segments[0].start;
		p_itsp_str_data_config ptr = (*data)->structure = malloc(sizeof(itsp_str_data_config));
		ptr->mode = *segments[1].start;
		ptr->calculation = *segments[2].start;
		//date
		read_date(segments[3].start, &ptr->date);
		//time
		read_time(segments[4].start, &ptr->time);
		//close_bet_delay
		memcpy(tmp,segments[6].start,2); 		ptr->close_bet_delay	= atoi(tmp);
		//close_cancel_delay
		memcpy(tmp,segments[7].start,2); 		ptr->close_cancel_delay	= atoi(tmp);
		//race_list
		memcpy(tmp,segments[8].start,2); 		ptr->race_list.races	= atoi(tmp);
		//perf
		memcpy(tmp,segments[5].start,4); 		ptr->performance		= atoi(tmp);
		i = ptr->race_list.races;
		memset(tmp,0,6);
		while(i--){
			memcpy(tmp,segments[9].start + 2*i,2);
			ptr->race_list.race[i]	= atoi(tmp);
		}
		//pool_config_list
		memcpy(tmp,segments[10].start,2); 		ptr->nb_pools			= atoi(tmp);
		s = segments[10].end;
		for(i = 0 ; i < ptr->nb_pools ; i++){
			segments = match_expr(s, pool_config_read_template, 0);
			if(!segments) {
				fprintf(std_err, "Match error : pool config list nb:%d\n", i);
				free((*data)->structure);
				(*data)->structure = NULL;
				return NULL;
			}
			memcpy(ptr->pool_configs[i].pool_code,segments[1].start,3);
			ptr->pool_configs[i].pool_code[3] = 0;
			ptr->pool_configs[i].exchange_part = *segments[2].start;
			ptr->pool_configs[i].scan_mode = *segments[3].start;

			ptr->pool_configs[i].send_gross_pool 	= *(segments[4].start);
			ptr->pool_configs[i].receive_gross_pool = *(segments[4].start+1);
			ptr->pool_configs[i].send_net_pool 		= *(segments[4].start+2);
			ptr->pool_configs[i].receive_net_pool 	= *(segments[4].start+3);
			ptr->pool_configs[i].send_total 		= *(segments[4].start+4);
			ptr->pool_configs[i].receive_total 		= *(segments[4].start+5);
			ptr->pool_configs[i].xfer_will_pay 	= *(segments[4].start+6);

			s = segments[5].start;
			s = read_amount(s, &ptr->pool_configs[i].pool_unit);
			s = read_amount(s, &ptr->pool_configs[i].min_payoff);
			s = read_amount(s, &ptr->pool_configs[i].Break);
			tmp[0] = segments[7].start[0]; tmp[1] = 0; ptr->pool_configs[i].nb_commissions = atoi(tmp);
			s = segments[7].end;
			for(j = 0 ; j < ptr->pool_configs[i].nb_commissions ; j++){
				s = read_amount(s, &ptr->pool_configs[i].commissions[j].rate);
 				ptr->pool_configs[i].commissions[j].rounding = *s++;
			}
		}
		(*data)->data_log = sub2str(SUB_STRING(data_start, segments[0].end + 5));
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data config\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

char* data_config_write_frame(char* buff, p_itsp_str_data_config ptr){
	*buff++ = ptr->mode;
	*buff++ = ptr->calculation;

	buff = write_date(buff, &ptr->date);
	buff = write_time(buff, &ptr->time);
	buff += sprintf(buff, "%04d%02d%02d%02d",
						ptr->performance,
						ptr->close_bet_delay,
						ptr->close_cancel_delay,
						ptr->race_list.races
				  );

	for(int i = 0; i < ptr->race_list.races; i++)
		buff += sprintf(buff, "%02d", ptr->race_list.race[i]);
	buff += sprintf(buff, "%02d", ptr->nb_pools);

	for(int i = 0 ; i < ptr->nb_pools ; i++){
		buff += sprintf(buff, "%s", ptr->pool_configs[i].pool_code);
		*buff++ = ptr->pool_configs[i].exchange_part;
		*buff++ = ptr->pool_configs[i].scan_mode;

		buff+= sprintf(buff, "%c%c%c%c%c%c%c",
								ptr->pool_configs[i].send_gross_pool,
								ptr->pool_configs[i].receive_gross_pool,
								ptr->pool_configs[i].send_net_pool,
								ptr->pool_configs[i].receive_net_pool,
								ptr->pool_configs[i].send_total,
								ptr->pool_configs[i].receive_total,
								ptr->pool_configs[i].xfer_will_pay
					  );
		buff = write_amount(buff, ptr->pool_configs[i].pool_unit, 0, 1, 4);
		buff = write_amount(buff, ptr->pool_configs[i].min_payoff, 0, 1, 4);
		buff = write_amount(buff, ptr->pool_configs[i].Break, 0, 1, 4);

		buff = write_digits(buff, ptr->pool_configs[i].nb_commissions, 1, 0);

		for(int j = 0 ; j < ptr->pool_configs[i].nb_commissions ; j++){
			buff = write_amount(buff, ptr->pool_configs[i].commissions[j].rate, 0, 0, 4);
			*buff++ = ptr->pool_configs[i].commissions[j].rounding;
		}
	}
	return buff;
}

char* pool_config_write_template 	="	pool code	: %s	|"
										"	exchange part	: %c\n"
										"	scan mode	: %c		|"
										"	when values 	: %c-%c-%c-%c-%c-%c-%c\n"
										"	pool unit	: %2.1f	|"
										"	min payoff		: %2.1f\n"
										"	break amount: %2.1f	|"
										"	commissions (%02d): ";

int data_config_log_to(FILE* stream, p_itsp_data_struct frame){
	p_itsp_str_data_config ptr = frame->structure;
	int i = 0, j = 0;

	fprintf(stream ,data_config_write_template,
			ptr->mode,
			ptr->calculation,
			ptr->date.day, ptr->date.month, ptr->date.year,
			ptr->time.hour, ptr->time.minutes, ptr->time.seconds,
			ptr->close_bet_delay,
			ptr->close_cancel_delay,
			ptr->performance);
	fprintf(stream , "%02d", ptr->race_list.race[0]);
	for (i = 1 ; i < ptr->race_list.races ; i++)
		fprintf(stream , "-%02d", ptr->race_list.race[i]);
	fprintf(stream , "]\n");
	fprintf(stream , "	pools (%02d) :\n",ptr->nb_pools);
	for (i = 0 ; i < ptr->nb_pools ; i++){
		fprintf(stream , pool_config_write_template,
				ptr->pool_configs[i].pool_code,
				ptr->pool_configs[i].exchange_part,
				ptr->pool_configs[i].scan_mode,
				ptr->pool_configs[i].send_gross_pool,
				ptr->pool_configs[i].receive_gross_pool,
				ptr->pool_configs[i].send_net_pool,
				ptr->pool_configs[i].receive_net_pool,
				ptr->pool_configs[i].send_total,
				ptr->pool_configs[i].receive_total,
				ptr->pool_configs[i].xfer_will_pay,
				ptr->pool_configs[i].pool_unit,
				ptr->pool_configs[i].min_payoff,
				ptr->pool_configs[i].Break,
				ptr->pool_configs[i].nb_commissions);
		fprintf(stream , "%f(%c)", ptr->pool_configs[i].commissions[0].rate,
						 ptr->pool_configs[i].commissions[0].rounding);
		for (j = 1 ; j < ptr->pool_configs[i].nb_commissions ; j++)
			fprintf(stream , " - %f(%c)", ptr->pool_configs[i].commissions[j].rate,
							 ptr->pool_configs[i].commissions[j].rounding);
		fprintf(stream , "\n");
	}

	return OK;
}

void data_config_free_frame(p_itsp_frame frame){
	p_itsp_str_data_config ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;
	if(ptr) free(ptr);
}






