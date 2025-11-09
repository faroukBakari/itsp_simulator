/*
 * data_race_status.c
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
#include <s3k_structs.h>
#include <s3k_session.h>


	 char* data_rs_h_write_template 	=
										"	status		: %c  	|"
										"	post time	: %02d:%02d:%02d	|"
										"	display : %c\n"
										"	runners		: %02d	|"
										"	runner stat	: %s	|"
										"	brackets [%d]";


	 char* data_rs_pool_def_read		=  // 2.2 pool_definition
										"(.{3})"						// 9 - pool code
										"([OCXE])"						// 10 - pool status
										"(([0-9]{2})*)"					// 11 - race list
										"([0-9,P-Y]*[@-I,`-i])"			// 12 - min bet
										"([0-9,P-Y]*[@-I,`-i,\\.])"		// 13 - net addin
										"([0-9,P-Y]*[@-I,`-i,\\.])";	// 14 - gross addin

	 char* data_pool_def_write_t1		=
										"\n	pool code	: %s	|"
										"	pool status	: %c   		|"
										"	race list : ";


	 char* data_pool_def_write_t2		=
										"\n	min bet		: %2.2f	|"
										"	net addin	: %02.2f		|"
										"	gross addin : %2.2f\n";

	 char* data_rs_s_p_info_read		=  // 2.4 scratch pool info
										".{3}"							// 16 - pool code
										"[0-9]{2}"						// 17 - race
										"([0-9\\,\\-]*/)*";				// 18 - pool runners

	 char* data_rs_h_read_template 	=	// 2 - data_race_status
										"(.)"							// 1 - status
										"([0-9,\\.]{6})"				// 2 - post time
										"([MCPF])"						// 3 - display
										"([0-9,\\.]{2})"				// 4 - runners
										"([0-9,a-z,A-Z]*)"				// 5 - runner status
										"([0-9,\\.]{2})"				// 6 - brakets
										"([0-9\\,\\-]*/)*"				// 7 - braket status
										"([0-9,\\.]{2})" 				// 8 - nb pools
										"(.{3}"							// 9 - pool code
										"[OCXE]"						// 10 - pool status
										"([0-9]{2})*"					// 11 - race list
										"[0-9,P-Y]*[@-I,`-i]"			// 12 - min bet
										"[0-9,P-Y]*[@-I,`-i,\\.]"		// 13 - net addin
										"[0-9,P-Y]*[@-I,`-i,\\.])*"		// 14 - gross addin
										"[0-9,\\.]{2}" 					// 15 - nb scratch
										"(.{3}"							// 16 - pool code
	 									"[0-9]{2}"						// 17 - race
	 									"([0-9\\,\\-]*/)*)*"			// 18 - pool runners
	 	 	 	 	 	 	 	 	 	"[0-9]{5}";

int data_rs_read_frame(char* log, p_itsp_data_struct* data){

	int i = 0, j = 0;
	p_s_s3k_pool_type p_pool = NULL;
	p_itsp_str_data_race_status ptr = (*data)->structure = malloc(sizeof(itsp_str_data_race_status));
	memset(ptr, 0, sizeof(itsp_str_data_race_status));

	ptr->status = *log++;
	log = read_time(log, &ptr->post_time);
	ptr->display = *log++;
	/*******************runner status**********************/
	log = read_digits(log, &ptr->runners, 2);
	ptr->runner_status[ptr->runners] = 0;
	log = read_fixed(log, ptr->runner_status, ptr->runners);
	/*******************bracket status**********************/
	log = read_digits(log, &ptr->brackets, 2);
	i = ptr->brackets;
	while(i--)
		log = read_numeric_range(log, &ptr->bracket_status[i]);
	/*******************pool status**********************/
	log = read_digits(log, &ptr->pools, 2);
	i = 0;
	while(i < ptr->pools){
		log = read_fixed(log, ptr->pool_def[i].pool_code, 3);
		ptr->pool_def[i].pool_status = *log++;
		p_pool = fast_map_at(liste_paris, ptr->pool_def[i].pool_code);
		if(!p_pool){
			DEBUG_TOFILE(std_err, "unknown pool code \"%s\"", ptr->pool_def[i].pool_code);
			return FAULT;
		}
		ptr->pool_def[i].race_list = malloc((p_pool->nb_races)*sizeof(int));
		ptr->pool_def[i].race_list[p_pool->nb_races - 1] = 0;
		j = 0;
		while(j < p_pool->nb_races - 1)
			log = read_digits(log, &ptr->pool_def[i].race_list[j++], 2);

		log = read_amount(log, &ptr->pool_def[i].min_bet);
		log = read_amount(log, &ptr->pool_def[i].net_addin);
		log = read_amount(log, &ptr->pool_def[i].gross_addin);
		i++;
	}
	/*******************scratch pools**********************/
	log = read_digits(log, &ptr->scratch_pools, 2);
	i = 0;
	while(i < ptr->scratch_pools){
		log = read_fixed(log, ptr->scratch_info[i].pool_code, 3);
		log = read_digits(log, &ptr->scratch_info[i].race, 2);
		p_pool = fast_map_at(liste_paris, ptr->pool_def[i].pool_code);
		if(!p_pool){
			DEBUG_TOFILE(std_err, "unknown pool code \"%s\"", ptr->pool_def[i].pool_code);
			return FAULT;
		}
		log = read_combo(log, &ptr->scratch_info[i].pool_runners);
		i++;
	}
	return OK;
 }

char* data_rs_write_frame(char* buff, p_itsp_str_data_race_status ptr){

	*buff++ = ptr->status;
	buff = write_time(buff, &ptr->post_time);
	*buff++ = ptr->display;
	/*******************runner status**********************/
	buff = write_digits(buff, ptr->runners, 2, 1);
	ptr->runner_status[ptr->runners] = 0;
	memcpy(buff, ptr->runner_status, ptr->runners);
	buff += ptr->runners;
	/*******************bracket status**********************/
	buff = write_digits(buff, ptr->brackets, 2, 1);
	int i = ptr->brackets;
	while(i--)
		buff = write_numeric_range(buff, ptr->bracket_status[i]);
	/*******************pool status**********************/
	buff = write_digits(buff, ptr->pools, 2, 1);
	i = 0;
	while(i < ptr->pools){
		buff = write_fixed(buff, ptr->pool_def[i].pool_code, 3);
		*buff++ = ptr->pool_def[i].pool_status;

		int j = 0;
		while(ptr->pool_def[i].race_list[j])
			buff = write_digits(buff, ptr->pool_def[i].race_list[j++],2,0);

		buff = write_amount(buff, ptr->pool_def[i].min_bet, 0, 0, 4);
		buff = write_amount(buff, ptr->pool_def[i].net_addin, 0, 1, 4);
		buff = write_amount(buff, ptr->pool_def[i].gross_addin, 0, 1, 4);

		i++;
	}
	/*******************scratch pools**********************/
	buff = write_digits(buff, ptr->scratch_pools, 2,1);
	i = 0;
	while(i < ptr->scratch_pools){
		buff = write_fixed(buff, ptr->scratch_info[i].pool_code, 3);
		buff = write_digits(buff, ptr->scratch_info[i].race, 2,0);
		buff = write_combo(buff, ptr->scratch_info[i].pool_runners);
		i++;
	}
	return buff;
 }

char* data_rs_read_from(char* log, p_itsp_data_struct* data){
	char tmp[6];
	char *s = NULL;
	memset(tmp,0,6);
	int i = 0, j = 0;
	const sub_str* segments = NULL;

	segments = match_expr(log, data_rs_h_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_race_status ptr = (*data)->structure = malloc(sizeof(itsp_str_data_race_status));
		memset(ptr, 0, sizeof(itsp_str_data_race_status));
		s = segments[1].start;
		ptr->status = *s++;
		s = read_time(s, &ptr->post_time);
		ptr->display = *s++;
		/*******************runner status**********************/
		s = read_digits(s, &ptr->runners, 2);
		ptr->runner_status[ptr->runners] = 0;
		memcpy(ptr->runner_status, s, ptr->runners);
		s += ptr->runners;
		/*******************bracket status**********************/
		s = read_digits(s, &ptr->brackets, 2);
		i = ptr->brackets;
		while(i--)
			s = read_numeric_range(s, &ptr->bracket_status[i]);
		/*******************pool status**********************/
		s = read_digits(s, &ptr->pools, 2);
		i = 0;
		while(i < ptr->pools){
			segments = match_expr(s, data_rs_pool_def_read, 0);
			if(segments){
				read_fixed(segments[1].start, ptr->pool_def[i].pool_code, 3);
				ptr->pool_def[i].pool_status = *segments[2].start;
				if((segments[3].end - segments[3].start) % 2){
					printf("Match error : pool def\n");
					free((*data)->structure);
					(*data)->structure = NULL;
					return NULL;
				}
				j = (segments[3].end - segments[3].start) / 2;
				ptr->pool_def[i].race_list = malloc((j+1)*sizeof(int));
				ptr->pool_def[i].race_list[j] = 0;
				s = segments[3].start;
				j = 0;
				while(j < (segments[3].end - segments[3].start) / 2)
					s = read_digits(s,&ptr->pool_def[i].race_list[j++],2);
				read_amount(segments[5].start, &ptr->pool_def[i].min_bet);
				read_amount(segments[6].start, &ptr->pool_def[i].net_addin);
				read_amount(segments[7].start, &ptr->pool_def[i].gross_addin);
				s = segments[0].end;
			}
			else{
				fprintf(std_err, "Match error : pool def nb:%d\n", ptr->pools-i);
				free((*data)->structure);
				(*data)->structure = NULL;
				return NULL;
			}
			i++;
		}
		/*******************scratch pools**********************/
		s = read_digits(s, &ptr->scratch_pools, 2);
		i = ptr->scratch_pools;
		while(i--){
			segments = match_expr(s, data_rs_s_p_info_read, 0);
			if(segments){
				s = segments->start;
				s = read_fixed(s, ptr->scratch_info[i].pool_code, 3);
				s = read_digits(s, &ptr->scratch_info[i].race, 2);
				s = read_combo(s, &ptr->scratch_info[i].pool_runners);
			}
			else{
				fprintf(std_err, "Match error : scratch pools nb:%d\n", ptr->scratch_pools-i);
				free((*data)->structure);
				(*data)->structure = NULL;
				return NULL;
			}
		}
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : race status\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

int data_rs_log_to(FILE* stream, p_itsp_data_struct data){
	p_itsp_str_data_race_status ptr = data->structure;
	int* p = NULL, i = 0;

	fprintf(stream ,data_rs_h_write_template,
							ptr->status,
							ptr->post_time.hour, ptr->post_time.minutes, ptr->post_time.seconds,
							ptr->display,
							ptr->runners,
							(ptr->runners > 0) ? ptr->runner_status : "		",
							ptr->brackets);
	i = ptr->brackets;
	if(i) {
		char buff[256] = {0}, *c_ptr = buff;
		fprintf(stream , " : ");
		while(i--){
			c_ptr = write_numeric_range(c_ptr, ptr->bracket_status[i]);
			fprintf(stream , "(B%02d) -> %s ", i + 1, buff);
		}
	}

	fprintf(stream ,"\n");

	i = ptr->pools;
	if(i) {
		fprintf(stream ,"	pools (%02d)  :", ptr->pools);
		while(i--){
			fprintf(stream ,data_pool_def_write_t1,
								ptr->pool_def[i].pool_code,
								ptr->pool_def[i].pool_status);
			p = ptr->pool_def[i].race_list;
			while(*p)
				fprintf(stream ,"%d ",*p++);
			fprintf(stream ,data_pool_def_write_t2,
								ptr->pool_def[i].min_bet,
								ptr->pool_def[i].net_addin,
								ptr->pool_def[i].gross_addin);
		}
	}

	i = ptr->scratch_pools;
	if(i){
		fprintf(stream ,"\n	scratch pools (%02d) :\n", ptr->scratch_pools);
		while(i--){
			char buff[256] = {0};
			write_combo(buff, ptr->scratch_info[i].pool_runners);

			fprintf(stream , "	pool code	: %s	|"
							"	race		: %02d		|"
							"	scratch info : %s\n",
							ptr->scratch_info[i].pool_code,
							ptr->scratch_info[i].race,
							buff);
		}
	}
	return OK;
}

void data_rs_free_frame(p_itsp_frame frame){
	p_itsp_str_data_race_status ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;
	if(ptr) {
		int i = 0;
		while(ptr->bracket_status[i])
			free(ptr->bracket_status[i++]);

		i = 0;
		while(i < ptr->pools)
			free(ptr->pool_def[i++].race_list);

		i = 0;
		while(i < ptr->scratch_pools)
			free_combo(ptr->scratch_info[i++].pool_runners);

		free(ptr);
	}
}






















