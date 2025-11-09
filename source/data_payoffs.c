/*
 * data_payoffs.c
 *
 *  Created on: 14 mars 2017
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

char* data_payoffs_read_template 		= // 1 - data_payoffs
										"(([0-9\\,\\-]*/)+)"			// 1 - live runners
										"([\\+\\-][0-9,P-Y]*[@-I,`-i])"	// 2 - refund
										"([0-9,P-Y]*[@-I,`-i])"			// 3 - carry over
										"([0-9])"						// 4 - nb_commissions
										"(([0-9,P-Y]*[@-I,`-i])*)"		// 5 - commissions
										"([0-9]{2})"					// 6 - prices
										"(([RE$PACQr])"					// 7 - status
										"([0-9]{2})"					// 8 - winners
										"[\\?!=\\&]?"					// indicatif de l'ordre (mode_francais)
										"(([0-9\\,\\-]*/)+)"			// 9 - runner list
										"([\\.\\+][0-9]{4})?"			// propre � PMC (voir avec g�rard ".1045")
										"([\\+\\-][0-9,P-Y]*[@-I,`-i])"	// 10 - price
										"([0-9,P-Y]*[@-I,`-i])"			// 11 - winning
										"([\\+\\-][0-9,P-Y]*[@-I,`-i])"	// 12 - liability
										"([\\+\\-][0-9,P-Y]*[@-I,`-i]))*"// 13 - break amount
										"[0-9]{5}";

char* data_payoffs_write_template 		=
										"\n	refund		:		%06.2f		|"
										"	carry over	:	%06.2f\n"
										"	nb_commissions	:		%02d		"
										;

char* price_def_read_template			= // 1.1 - price definition
										"([RE$PACQr])"					// 7 - status
										"([0-9]{2})"					// 8 - winners
										"[\\?!=\\&]?"					// indicatif de l'ordre (mode_francais)
										"(([0-9\\,\\-]*/)+)"			// 9 - runner list
										"([\\.\\+][0-9]{4})?"			// propre � PMC (voir avec g�rard ".1045")
										"([\\+\\-][0-9,P-Y]*[@-I,`-i])"	// 10 - price
										"([0-9,P-Y]*[@-I,`-i])"			// 11 - winning
										"([\\+\\-][0-9,P-Y]*[@-I,`-i])"	// 12 - liability
										"([\\+\\-][0-9,P-Y]*[@-I,`-i])";// 13 - break amount

char* price_def_write_template 		=
										"	status	:	%c	|"
										"	winners	:	%02d	|"
										"	runner list	:	%s\n"
										"	price		:	%06.2f\n"
										"	winning		:	%06.2f\n"
										"	liability	:	%06.2f\n"
										"	break amount:	%06.2f\n"
										;

char* data_payoffs_write_frame(char* buff, p_itsp_str_data_payoffs ptr){

	buff = write_combo(buff, ptr->live_runners);

	buff = write_amount(buff, ptr->refund,1,0, 4);
	buff = write_amount(buff, ptr->carry_over,0,0, 4);
	buff = write_digits(buff, ptr->nb_commissions, 1, 0);
	if(ptr->nb_commissions){
		int i = ptr->nb_commissions;
		while(i--)
			buff = write_amount(buff, ptr->commissions[i],0,0, 4);
	}

	buff = write_digits(buff, ptr->nb_prices, 2, 0);

	if(ptr->nb_prices){
		int i = ptr->nb_prices;
		while(i--){
			*buff++ = ptr->price_definition[i].status;
			buff = write_digits(buff, ptr->price_definition[i].winners,2,0);
			if(ptr->price_definition[i].order_flag && MODE_FRANCAIS)
				*buff++ = ptr->price_definition[i].order_flag;

			for(int j = 0; ptr->price_definition[i].runner_list[j]; j++)
				buff = write_numeric_range(buff, ptr->price_definition[i].runner_list[j]);


			buff = write_amount(buff,  ptr->price_definition[i].price,1,0, 4);
			buff = write_amount(buff,  ptr->price_definition[i].winning,0,0, 4);
			buff = write_amount(buff,  ptr->price_definition[i].liability,1,0, 4);
			buff = write_amount(buff,  ptr->price_definition[i].break_amount,1,0, 4);
		}
	}

	return buff;
}

int data_payoffs_read_frame(char* log, p_itsp_data_struct* data, char* pool_code){

	p_itsp_str_data_payoffs ptr = (*data)->structure = malloc(sizeof(itsp_str_data_payoffs));
	p_s_s3k_pool_type p_pool = NULL;

	p_pool = fast_map_at(liste_paris, pool_code);
	if(!p_pool){
		DEBUG_TOFILE(std_err, "unknown pool code \"%s\"", pool_code);
		return FAULT;
	}

	ptr->live_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->live_runners[p_pool->dimensions] = NULL;
	int i = 0, j = 0;

	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->live_runners[i++]);


	log = read_amount(log, &ptr->refund);
	log = read_amount(log, &ptr->carry_over);
	log = read_digits(log, &ptr->nb_commissions, 1);

	if(ptr->nb_commissions){
		ptr->commissions = malloc(ptr->nb_commissions * sizeof(double));
		int i = ptr->nb_commissions;
		while(i--)
			log = read_amount(log, &ptr->commissions[i]);
	}
	log = read_digits(log, &ptr->nb_prices, 2);

	if(ptr->nb_prices){
		ptr->price_definition = malloc(ptr->nb_prices * sizeof(itsp_price_definition));
		int i = ptr->nb_prices;
		while(i--){
			ptr->price_definition[i].status = *log++;
			log = read_digits(log, &ptr->price_definition[i].winners,2);

			if(MODE_FRANCAIS)
				ptr->price_definition[i].order_flag =  *log;
			else
				ptr->price_definition[i].order_flag = 0;

			ptr->price_definition[i].runner_list = malloc((ptr->price_definition[i].winners + 1) * sizeof(int*));
			ptr->price_definition[i].runner_list[ptr->price_definition[i].winners] = NULL;

			j = 0;
			while(j < ptr->price_definition[i].winners)
				log = read_numeric_range(log, &ptr->price_definition[i].runner_list[j++]);

			log = read_amount(log, &ptr->price_definition[i].price);
			log = read_amount(log, &ptr->price_definition[i].winning);
			log = read_amount(log, &ptr->price_definition[i].liability);
			log = read_amount(log, &ptr->price_definition[i].break_amount);
		}
	}
	return OK;
}

char* data_payoffs_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, data_payoffs_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_payoffs ptr = (*data)->structure = malloc(sizeof(itsp_str_data_payoffs));

		int* buff[256] = {NULL}, i = 0;
		char* tmp = sub2str(segments[1]), *ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->live_runners = malloc((i + 1) * sizeof(int*));
		ptr->live_runners[i] = NULL;
		memcpy(ptr->live_runners, buff, i * sizeof(int*));

		read_amount(segments[3].start, &ptr->refund);
		read_amount(segments[4].start, &ptr->carry_over);
		read_digits(segments[5].start, &ptr->nb_commissions, 1);
		if(ptr->nb_commissions){
			ptr->commissions = malloc(ptr->nb_commissions * sizeof(double));
			int i = ptr->nb_commissions;
			char* s = segments[6].start;
			while(i--)
				s = read_amount(s, &ptr->commissions[i]);
		}
		read_digits(segments[8].start, &ptr->nb_prices, 2);
		if(ptr->nb_prices){
			ptr->price_definition = malloc(ptr->nb_prices * sizeof(itsp_price_definition));
			int i = ptr->nb_prices;
			char* s = segments[8].end;
			while(i--){
				segments = match_expr(s, price_def_read_template, 0);
				if(segments){
					ptr->price_definition[i].status = *segments[1].start;
					read_digits(segments[2].start, &ptr->price_definition[i].winners,2);
					if(segments[2].end != segments[3].start){
						MODE_FRANCAIS = 1;
						ptr->price_definition[i].order_flag =  *segments[2].end;
					}else
						ptr->price_definition[i].order_flag = 0;

					memset(buff, 0, 256 * sizeof(int*));
					int j = 0;
					tmp = sub2str(segments[3]); ptr_tmp = tmp;
					while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[j++])))
						continue;
					free(tmp);
					ptr->price_definition[i].runner_list = malloc((j + 1) * sizeof(int*));
					ptr->price_definition[i].runner_list[j] = NULL;
					memcpy(ptr->price_definition[i].runner_list, buff, j * sizeof(int*));

					read_amount(segments[6].start, &ptr->price_definition[i].price);
					read_amount(segments[7].start, &ptr->price_definition[i].winning);
					read_amount(segments[8].start, &ptr->price_definition[i].liability);
					read_amount(segments[9].start, &ptr->price_definition[i].break_amount);
				}
				else{
					fprintf(std_out, "Match error : "
							"data payoffs::price_definition nb:%d/%d\nbuff:\"%s\"\n",
							ptr->nb_prices - i,
							ptr->nb_prices,
							s);

					free(ptr->commissions);
					free(ptr->price_definition);
					free(ptr);
					(*data)->structure = NULL;
					return NULL;
				}
				s = segments[0].end;
			}
		}
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data payoffs\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

int data_payoffs_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_data_payoffs ptr = data->structure;

	{
		char buff[256] = {0}, *buff_ptr = buff;
		for(int i = 0; ptr->live_runners[i]; i++)
			buff_ptr = write_numeric_range(buff_ptr, ptr->live_runners[i]);
		fprintf(stream ,"	live runners	:	%s\n",buff);
	}


	fprintf(stream ,data_payoffs_write_template,
						ptr->refund,
						ptr->carry_over,
						ptr->nb_commissions);

	if(ptr->nb_commissions){
		int i = ptr->nb_commissions - 1;
		fprintf(stream ,"	|	commissions : %06.2f",ptr->commissions[ptr->nb_commissions - 1]);
		while(i--){
			fprintf(stream ," - %06.2f",ptr->commissions[i]);
		}
	}
	fprintf(stream ,"\n");

	if(ptr->nb_prices){
		fprintf(stream ,"	prices (%02d)	:\n",ptr->nb_prices);
		int i = ptr->nb_prices;
		while(i--){
			char buff[256] = {0}, *buff_ptr = buff;
			for(int j = 0; ptr->price_definition[i].runner_list[j]; j++)
				buff_ptr = write_numeric_range(buff_ptr, ptr->price_definition[i].runner_list[j]);

			fprintf(stream ,price_def_write_template,
						ptr->price_definition[i].status,
						ptr->price_definition[i].winners,
						buff,
						ptr->price_definition[i].price,
						ptr->price_definition[i].winning,
						ptr->price_definition[i].liability,
						ptr->price_definition[i].break_amount);
			fprintf(stream ,"\n");
		}
	}

	return OK;
}

void data_payoffs_free_frame(p_itsp_frame frame){

	p_itsp_str_data_payoffs ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;

	if(ptr) {
		if(ptr->live_runners) free_combo(ptr->live_runners);
		if(ptr->nb_commissions && ptr->commissions) free(ptr->commissions);
		int i = 0;
		while( i < ptr->nb_prices){
			if(ptr->price_definition[i].runner_list)
				free_combo(ptr->price_definition[i].runner_list);
			i++;
		}
		free(ptr->price_definition);
		free(ptr);
	}
}







