/*
 * data_results.c
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



char* data_results_read_template =				// 1 - data_result
												"([0-9]{2})"					// 1 - finshers
												"((([0-9]{2})"					// 2 - runner
												"([0-9,a-z,A-Z, ])"				// 3 - entry
												"([0-9]{2}))*)"					// 4 - position
												"([0-9\\,\\-]*/)"				// favorite list
												"[0-9]{5}";

char* data_results_write_template =				"	finishers	:	%d	|"
												"	favorite list	:	%s\n"
												"	finishs	:\n";

char* finish_write_template =
												"		runner	:	%d	|"
												"	entry	:	%c	|"
												"	position	:	%d\n";

char* data_results_write_frame(char* buff, p_itsp_str_data_results ptr){

	buff = write_digits(buff, ptr->nb_finshers, 2, 0);


	if(ptr->nb_finshers){
		int i = 0;
		while(i < ptr->nb_finshers){
			buff = write_digits(buff, ptr->finishs[i].runner, 2, 0);
			*buff++ = ptr->finishs[i].entry;
			buff = write_digits(buff, ptr->finishs[i].position, 2, 0);
			i++;
		}
	}
	buff = write_numeric_range(buff, ptr->favorite_list);

	return buff;

}

int data_results_read_frame(char* log, p_itsp_data_struct* data){

	p_itsp_str_data_results ptr = (*data)->structure = malloc(sizeof(itsp_str_data_results));
	log = read_digits(log, &ptr->nb_finshers, 2);
	if(ptr->nb_finshers){
		ptr->finishs = malloc(ptr->nb_finshers * sizeof(itsp_finish));
		int i = 0;
		while(i < ptr->nb_finshers){
			log = read_digits(log, &ptr->finishs[i].runner, 2);
			ptr->finishs[i].entry = *log++;
			log = read_digits(log, &ptr->finishs[i].position, 2);
			i++;
		}
	}
	log = read_numeric_range(log, &ptr->favorite_list);
	return OK;
}

char* data_results_read_from(char* buff, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(buff, data_results_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_results ptr = (*data)->structure = malloc(sizeof(itsp_str_data_results));
		read_digits(segments[1].start, &ptr->nb_finshers, 2);
		read_numeric_range(segments[7].start, &ptr->favorite_list);
		if(ptr->nb_finshers){
			ptr->finishs = malloc(ptr->nb_finshers * sizeof(itsp_finish));
			int i = ptr->nb_finshers;
			char* s = segments[2].start;
			while(i--){
				s = read_digits(s, &ptr->finishs[i].runner, 2);
				ptr->finishs[i].entry = *s++;
				s = read_digits(s, &ptr->finishs[i].position, 2);
			}
		}
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data results\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return buff;
}

int data_results_log_to(FILE* stream, p_itsp_data_struct data){
	p_itsp_str_data_results ptr = data->structure;
	char buff[1024] = {0};

	write_numeric_range(buff, ptr->favorite_list);

	fprintf(stream ,data_results_write_template,
							ptr->nb_finshers,
							buff);

	if(ptr->nb_finshers){
		int i = ptr->nb_finshers;
		while(i--){
			fprintf(stream ,finish_write_template,
										ptr->finishs[i].runner,
										ptr->finishs[i].entry,
										ptr->finishs[i].position);
		}
	}

	return OK;
}

void data_results_free_frame(p_itsp_frame frame){

	p_itsp_str_data_results ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;

	if(ptr) {
		if(ptr->favorite_list) free(ptr->favorite_list);
		if(ptr->nb_finshers && ptr->finishs) free(ptr->finishs);
		free(ptr);
	}
}
