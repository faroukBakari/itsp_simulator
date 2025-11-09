/*
 * data_total.c
 *
 *  Created on: 13 mars 2017
 *      Author: f.baccari
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>


char* data_totals_read_template 	= 	// pool total
										"([0-9,P-Y]*[@-I,`-i])"			// 1 - live total
										"([0-9,P-Y]*[@-I,`-i])"			// 2 - net total
										"[0-9]{5}";

char* data_totals_write_template =
										"	live total	:	%06.2f	|"
										"	net total	:	%06.2f\n";

char* data_totals_write_frame(char* buff, p_itsp_str_data_totals ptr){
	buff = write_amount(buff, ptr->live_total, 0, 0, 4);
	buff = write_amount(buff, ptr->net_total, 0, 0, 4);
	return buff;

}

int data_totals_read_frame(char* log, p_itsp_data_struct* data){
	p_itsp_str_data_totals ptr = (*data)->structure = malloc(sizeof(itsp_str_data_totals));
	log = read_amount(log, &ptr->live_total);
	log = read_amount(log, &ptr->net_total);
	return OK;
}

char* data_totals_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, data_totals_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_totals ptr = (*data)->structure = malloc(sizeof(itsp_str_data_totals));
		read_amount(segments[1].start, &ptr->live_total);
		read_amount(segments[2].start, &ptr->net_total);
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data total\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

int data_totals_write_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_data_totals ptr = data->structure;

	fprintf(stream ,data_totals_write_template,
					ptr->live_total,
					ptr->net_total);

	return OK;
}

void data_totals_free_frame(p_itsp_frame frame){
	p_itsp_str_data_totals ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;
	if(ptr) free(ptr);

}
