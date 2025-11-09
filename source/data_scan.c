/*
 * data_scan.c
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
#include <s3k_structs.h>
#include <s3k_session.h>


char* srb_read_template				=	// 1 - scan_request
										"([ELSXQACKP])"											// 1 - scan mode
										"([0-9]{2})"											// 2 - combos
										"[\\?!=\\&]?(([0-9\\,\\-]*/)+)([\\.\\+][0-9]{4})?\\."	// 3 - scan runners
										"(([0-9\\,\\-]*/)+)\\."									// 4 - live runners
										"(([0-9\\,\\-]*/)+)\\."									// 5 - favorite runners
										"(([0-9\\,\\-]*/)+)"									// 6 - sub runners
										"([0-9]{2})?"											// 7 - leg
										"[0-9]{5}";

char* srb_write_template			= 	"leg %02d	:\n "
										"	scan mode		:	%c			|	"
										"combos			:	%02d\n	"
										"scan runners	:	%s%s	|	"
										"live runners	:	%s\n	"
										"favorite runners:	%s%s		|	"
										"sub runners		:	%s\n";

char* scan_request_begin_write_frame(char* buff, p_itsp_str_scan_header ptr){

	*buff++ = ptr->scan_mode;
	buff = write_digits(buff, ptr->combos, 2, 0);

	for(int j = 0; ptr->scan_runners[j]; j++)
		buff = write_numeric_range(buff, ptr->scan_runners[j]);
	*buff++ = '.';
	for(int j = 0; ptr->live_runners[j]; j++)
		buff = write_numeric_range(buff, ptr->live_runners[j]);
	*buff++ = '.';
	for(int j = 0; ptr->favorite_runners[j]; j++)
		buff = write_numeric_range(buff, ptr->favorite_runners[j]);
	*buff++ = '.';
	for(int j = 0; ptr->sub_runners[j]; j++)
		buff = write_numeric_range(buff, ptr->sub_runners[j]);

	if(ptr->leg)
		buff = write_digits(buff, ptr->leg, 2, 0);

	return buff;
}

int scan_request_begin_read_frame(char* log, p_itsp_data_struct* data, char* pool_code){

	p_itsp_str_scan_header ptr = (*data)->structure = malloc(sizeof(itsp_str_scan_header));
	p_s_s3k_pool_type p_pool = NULL;


	ptr->scan_mode = *log++;
	log = read_digits(log, &ptr->combos, 2);

	p_pool = fast_map_at(liste_paris, pool_code);
	if(!p_pool){
		DEBUG_TOFILE(std_err, "unknown pool code \"%s\"", pool_code);
		return FAULT;
	}




	ptr->scan_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->scan_runners[p_pool->dimensions] = NULL;

	int i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->scan_runners[i++]);

	if(*log++ != '.'){
		DEBUG_TOFILE(std_err, "frame format error");
		return FAULT;
	}

	ptr->live_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->live_runners[p_pool->dimensions] = NULL;

	i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->live_runners[i++]);

	if(*log++ != '.'){
		DEBUG_TOFILE(std_err, "frame format error");
		return FAULT;
	}

	ptr->favorite_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->favorite_runners[p_pool->dimensions] = NULL;

	i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->favorite_runners[i++]);

	if(*log++ != '.'){
		DEBUG_TOFILE(std_err, "frame format error");
		return FAULT;
	}

	ptr->sub_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->sub_runners[p_pool->dimensions] = NULL;

	i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->sub_runners[i++]);

	if(strlen(log) > CHECKSUM_SIZE)
		read_digits(log, &ptr->leg, 2);
	else
		ptr->leg = 0;

	return OK;
}

char* scan_request_begin_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, srb_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_scan_header ptr = (*data)->structure = malloc(sizeof(itsp_str_scan_header));

		ptr->scan_mode = *segments[1].start;
		read_digits(segments[2].start, &ptr->combos, 2);

		int* buff[256] = {NULL}, i = 0;
		char* tmp = sub2str(segments[10]), *ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->sub_runners = malloc((i + 1) * sizeof(int*));
		ptr->sub_runners[i] = NULL;
		memcpy(ptr->sub_runners, buff, i * sizeof(int*));

		memset(buff, 0, 256 * sizeof(int));
		i = 0;
		tmp = sub2str(segments[3]);
		ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->scan_runners = malloc((i + 1) * sizeof(int*));
		ptr->scan_runners[i] = NULL;
		memcpy(ptr->scan_runners, buff, i * sizeof(int*));

		memset(buff, 0, 256 * sizeof(int));
		i = 0;
		tmp = sub2str(segments[6]);
		ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->live_runners = malloc((i + 1) * sizeof(int*));
		ptr->live_runners[i] = NULL;
		memcpy(ptr->live_runners, buff, i * sizeof(int*));

		memset(buff, 0, 256 * sizeof(int));
		i = 0;
		tmp = sub2str(segments[8]);
		ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->favorite_runners = malloc((i + 1) * sizeof(int*));
		ptr->favorite_runners[i] = NULL;
		memcpy(ptr->favorite_runners, buff, i * sizeof(int*));

		if(segments[12].start != segments[12].end)
			read_digits(segments[12].start, &ptr->leg, 2);
		else
			ptr->leg = 0;
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : scan request begin\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

int scan_request_begin_log_to(FILE* stream, p_itsp_data_struct data){

		p_itsp_str_scan_header ptr = data->structure;

		char live[256] = {0}, scan[256] = {0},
			sub[256] = {0}, fav[256] = {0},
			*buff_ptr = NULL;

		buff_ptr = live;
		for(int i = 0; ptr->live_runners[i]; i++)
			buff_ptr = write_numeric_range(buff_ptr, ptr->live_runners[i]);
		buff_ptr = scan;
		for(int i = 0; ptr->scan_runners[i]; i++)
			buff_ptr = write_numeric_range(buff_ptr, ptr->scan_runners[i]);
		buff_ptr = sub;
		for(int i = 0; ptr->sub_runners[i]; i++)
			buff_ptr = write_numeric_range(buff_ptr, ptr->sub_runners[i]);
		buff_ptr = fav;
		for(int i = 0; ptr->favorite_runners[i]; i++)
			buff_ptr = write_numeric_range(buff_ptr, ptr->favorite_runners[i]);

		fprintf(stream ,srb_write_template,
						ptr->leg,
						ptr->scan_mode,
						ptr->combos,
						scan, (strlen(scan)>8) ? "" : "\t",
						live,
						fav, (strlen(fav)>8) ? "" : "\t",
						sub);

	return OK;
}

char* data_scan_read_template			=	// 1 - data_scan
										"([ELSXQACKP])"					// 1 - scan mode
										"([0-9]{2})"					// 2 - combos
										"(([0-9\\,\\-]*/)+)\\."			// 3 - scan runners
										"(([0-9\\,\\-]*/)+)\\."			// 4 - live runners
										"(([0-9\\,\\-]*/)+)\\."			// 5 - favorite runners
										"(([0-9\\,\\-]*/)+)"			// 6 - sub runners
										"([0-9]{2})"					// 7 - leg
										"([GN])"						// 8 - pool mode
										"([0-9]{2})"					// 9 - segments
										"([0-9]{2})"					// 10 - segment
										"([0-9]{2})"					// 11 - rows
										"([0-9]{2})"					// 12 - columns
										"(([0-9,P-Y]*[@-I,`-i])*)"		// 13 - data matrix (amounts)
										"([0-9,P-Y]*[@-I,`-i])"			// 14 - segment total
										"([0-9,P-Y]*[@-I,`-i])"			// 15 - total
										"([0-9,P-Y]*[@-I,`-i])"			// 16 - net total
										"([0-9,P-Y]*[@-I,`-i])"			// 16 - scan total
										"([0-9,P-Y]*[@-I,`-i])"			// 16 - live total
										"[0-9]{5}";

char* data_scan_write_template 			=
										"leg %02d	:\n "
										"	scan mode		:	%c				|	"
										"combos			:	%02d\n	"
										"scan runners	:	%s%s		|	"
										"live runners	:	%s\n	"
										"favorite runners:	%s%s		|	"
										"sub runners		:	%s\n"
										"	pool mode	:	%c		|"
										"	segment	: %02d/%02d		|"
										"	matrix dims	: %d x %d\n"
										"	seg. total	: %06.2f	|"
										"	total	: %06.2f	|"
										"	net total	: %06.2f\n"
										"	scan total	:	%06.2f	|"
										"	live total	:	%06.2f\n";

char* data_scan_write_frame(char* buff, p_itsp_str_data_scan ptr){
	int i=0, j=0;
	double row_cumul = 0;


	/************************SCAN HEADER******************************/
	p_itsp_str_scan_header h_ptr = ptr->scan_header;
	*buff++ = h_ptr->scan_mode;
	buff = write_digits(buff, h_ptr->combos, 2, 0);

	for(int j = 0; h_ptr->scan_runners[j]; j++)
		buff = write_numeric_range(buff, h_ptr->scan_runners[j]);
	*buff++ = '.';
	for(int j = 0; h_ptr->live_runners[j]; j++)
		buff = write_numeric_range(buff, h_ptr->live_runners[j]);
	*buff++ = '.';
	for(int j = 0; h_ptr->favorite_runners[j]; j++)
		buff = write_numeric_range(buff, h_ptr->favorite_runners[j]);
	*buff++ = '.';
	for(int j = 0; h_ptr->sub_runners[j]; j++)
		buff = write_numeric_range(buff, h_ptr->sub_runners[j]);

	buff = write_digits(buff, h_ptr->leg, 2, 0);

	/************************DATA SCAN******************************/
	p_itsp_str_data_pools d_ptr = ptr->scan_data;
	*buff++ = d_ptr->pool_header.pool_mode;
	buff = write_digits(buff, d_ptr->pool_header.segments, 2, 0);
	buff = write_digits(buff, d_ptr->pool_header.segment, 2, 0);
	buff = write_digits(buff, d_ptr->pool_data.rows, 2, 0);
	buff = write_digits(buff, d_ptr->pool_data.columns, 2, 0);

	if(d_ptr->pool_data.rows){
		for(i = 0 ; i < d_ptr->pool_data.rows; i++){
			row_cumul = 0;
			for(j = 0; j < d_ptr->pool_data.columns; j++){
				buff = write_amount(buff, d_ptr->pool_data.matrix_data[i][j], 0, 0, 4);
				row_cumul += d_ptr->pool_data.matrix_data[i][j];
			}
			buff = write_amount(buff, row_cumul, 0, 0, 4);
		}
	}

	buff = write_amount(buff, d_ptr->pool_data.segment_total, 0, 0, 4);
	buff = write_amount(buff, d_ptr->pool_data.total, 0, 0, 4);
	buff = write_amount(buff, d_ptr->pool_data.net_total, 0, 0, 4);

	buff = write_amount(buff, ptr->scan_total, 0, 0, 4);
	buff = write_amount(buff, ptr->live_total, 0, 0, 4);

	return buff;

}

int data_scan_read_frame(char* log, p_itsp_data_struct* data, char* pool_code){
	int i=0, j=0;
	double row_cumul = 0, row_total = 0;
	p_s_s3k_pool_type p_pool = NULL;

	p_itsp_str_data_scan p = (*data)->structure = malloc(sizeof(itsp_str_data_scan));

	/************************SCAN HEADER******************************/
	p_itsp_str_scan_header ptr = p->scan_header = malloc(sizeof(itsp_str_scan_header));

	ptr->scan_mode = *log++;
	log = read_digits(log, &ptr->combos, 2);

	p_pool = fast_map_at(liste_paris, pool_code);
	if(!p_pool){
		DEBUG_TOFILE(std_err, "unknown pool code \"%s\"", pool_code);
		return FAULT;
	}

	ptr->scan_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->scan_runners[p_pool->dimensions] = NULL;

	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->scan_runners[i++]);

	if(*log++ != '.'){
		DEBUG_TOFILE(std_err, "frame format error");
		return FAULT;
	}

	ptr->live_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->live_runners[p_pool->dimensions] = NULL;

	i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->live_runners[i++]);

	if(*log++ != '.'){
		DEBUG_TOFILE(std_err, "frame format error");
		return FAULT;
	}

	ptr->favorite_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->favorite_runners[p_pool->dimensions] = NULL;

	i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->favorite_runners[i++]);

	if(*log++ != '.'){
		DEBUG_TOFILE(std_err, "frame format error");
		return FAULT;
	}

	ptr->sub_runners = malloc((p_pool->dimensions + 1) * sizeof(int*));
	ptr->sub_runners[p_pool->dimensions] = NULL;

	i = 0;
	while(i < p_pool->dimensions)
		log = read_numeric_range(log, &ptr->sub_runners[i++]);

	log = read_digits(log, &ptr->leg, 2);




	/************************DATA SCAN******************************/
	p_itsp_str_data_pools d_ptr = p->scan_data = malloc(sizeof(itsp_str_data_pools));
	d_ptr->pool_header.pool_mode = *log++;
	log = read_digits(log, &d_ptr->pool_header.segments, 2);
	log = read_digits(log, &d_ptr->pool_header.segment, 2);
	log = read_digits(log, &d_ptr->pool_data.rows, 2);
	log = read_digits(log, &d_ptr->pool_data.columns, 2);

	if(d_ptr->pool_data.rows){
		d_ptr->pool_data.matrix_data = malloc(d_ptr->pool_data.rows * sizeof(double*));
		for(i = 0 ; i < d_ptr->pool_data.rows; i++){
			d_ptr->pool_data.matrix_data[i] = malloc(d_ptr->pool_data.columns * sizeof(double));
			row_cumul = 0;
			for(j = 0; j < d_ptr->pool_data.columns; j++){
				log = read_amount(log, &d_ptr->pool_data.matrix_data[i][j]);
				row_cumul += d_ptr->pool_data.matrix_data[i][j];
			}
			log = read_amount(log, &row_total);
		}
	}

	log = read_amount(log, &d_ptr->pool_data.segment_total);
	log = read_amount(log, &d_ptr->pool_data.total);
	log = read_amount(log, &d_ptr->pool_data.net_total);

	log = read_amount(log, &p->scan_total);
	log = read_amount(log, &p->live_total);

	return OK;
}

char* data_scan_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;
	char *s = NULL;
	int i=0, j=0;
	double row_cumul = 0, row_total = 0;

	segments = match_expr(log, data_scan_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_scan p = (*data)->structure = malloc(sizeof(itsp_str_data_scan));

		/************************SCAN HEADER******************************/
		p_itsp_str_scan_header ptr = p->scan_header = malloc(sizeof(itsp_str_scan_header));
		ptr->scan_mode = *segments[1].start;
		read_digits(segments[2].start, &ptr->combos, 2);

		int* buff[256] = {NULL};
		char* tmp = sub2str(segments[9]), *ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->sub_runners = malloc((i + 1) * sizeof(int*));
		ptr->sub_runners[i] = NULL;
		memcpy(ptr->sub_runners, buff, i * sizeof(int*));

		memset(buff, 0, 256 * sizeof(int));
		i = 0;
		tmp = sub2str(segments[3]);
		ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->scan_runners = malloc((i + 1) * sizeof(int*));
		ptr->scan_runners[i] = NULL;
		memcpy(ptr->scan_runners, buff, i * sizeof(int*));

		memset(buff, 0, 256 * sizeof(int));
		i = 0;
		tmp = sub2str(segments[5]);
		ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->live_runners = malloc((i + 1) * sizeof(int*));
		ptr->live_runners[i] = NULL;
		memcpy(ptr->live_runners, buff, i * sizeof(int*));

		memset(buff, 0, 256 * sizeof(int));
		i = 0;
		tmp = sub2str(segments[7]);
		ptr_tmp = tmp;
		while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[i++])))
			continue;
		free(tmp);
		ptr->favorite_runners = malloc((i + 1) * sizeof(int*));
		ptr->favorite_runners[i] = NULL;
		memcpy(ptr->favorite_runners, buff, i * sizeof(int*));

		read_digits(segments[11].start, &ptr->leg, 2);

		/************************DATA SCAN******************************/
		p_itsp_str_data_pools d_ptr = p->scan_data = malloc(sizeof(itsp_str_data_pools));
		d_ptr->pool_header.pool_mode = *segments[12].start;
		read_digits(segments[13].start, &d_ptr->pool_header.segments, 2);
		read_digits(segments[14].start, &d_ptr->pool_header.segment, 2);
		read_digits(segments[15].start, &d_ptr->pool_data.rows, 2);
		read_digits(segments[16].start, &d_ptr->pool_data.columns, 2);

		if(d_ptr->pool_data.rows){
			s = segments[17].start;
			d_ptr->pool_data.matrix_data = malloc(d_ptr->pool_data.rows * sizeof(double*));
			for(i = 0 ; i < d_ptr->pool_data.rows; i++){
				d_ptr->pool_data.matrix_data[i] = malloc(d_ptr->pool_data.columns * sizeof(double));
				row_cumul = 0;
				for(j = 0; j < d_ptr->pool_data.columns; j++){
					s = read_amount(s, &d_ptr->pool_data.matrix_data[i][j]);
					row_cumul += d_ptr->pool_data.matrix_data[i][j];
				}
				s = read_amount(s, &row_total);
			}
		}

		s = read_amount(segments[19].start, &d_ptr->pool_data.segment_total);
		s = read_amount(s, &d_ptr->pool_data.total);
		s = read_amount(s, &d_ptr->pool_data.net_total);

		s = read_amount(s, &p->scan_total);
		s = read_amount(s, &p->live_total);
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data scan final\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

int data_scan_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_data_scan ptr = data->structure;
	p_itsp_str_scan_header	h_ptr = ptr->scan_header;
	p_itsp_str_data_pools	d_ptr = ptr->scan_data;

	char live[256] = {0}, scan[256] = {0},
		 sub[256] = {0}, fav[256] = {0},
		*buff_ptr = NULL;

	buff_ptr = live;
	for(int i = 0; h_ptr->live_runners[i]; i++)
		buff_ptr = write_numeric_range(buff_ptr, h_ptr->live_runners[i]);
	buff_ptr = scan;
	for(int i = 0; h_ptr->scan_runners[i]; i++)
		buff_ptr = write_numeric_range(buff_ptr, h_ptr->scan_runners[i]);
	buff_ptr = sub;
	for(int i = 0; h_ptr->sub_runners[i]; i++)
		buff_ptr = write_numeric_range(buff_ptr, h_ptr->sub_runners[i]);
	buff_ptr = fav;
	for(int i = 0; h_ptr->favorite_runners[i]; i++)
		buff_ptr = write_numeric_range(buff_ptr, h_ptr->favorite_runners[i]);

	fprintf(stream ,data_scan_write_template,
							h_ptr->leg,
							h_ptr->scan_mode,
							h_ptr->combos,
							scan, (strlen(scan)>8) ? "" : "\t",
							live,
							fav, (strlen(fav)>8) ? "" : "\t",
							sub,
							d_ptr->pool_header.pool_mode,
							d_ptr->pool_header.segment,
							d_ptr->pool_header.segments,
							d_ptr->pool_data.rows,
							d_ptr->pool_data.columns,
							d_ptr->pool_data.segment_total,
							d_ptr->pool_data.total,
							d_ptr->pool_data.net_total,
							ptr->scan_total,
							ptr->live_total);

	if(d_ptr->pool_data.rows && d_ptr->pool_data.matrix_data){
		int i = 0, j = 0;
		fprintf(stream ,"\tmatrix data :\n");
		for(i = 0 ; i < d_ptr->pool_data.rows; i++){
			fprintf(stream ,"\t\t");
			for(j = 0; j < d_ptr->pool_data.columns; j++)
				fprintf(stream ,"%10.2f	",d_ptr->pool_data.matrix_data[i][j]);
			fprintf(stream ,"\n");
		}
	}

	return OK;
}

char* data_scan_pen_req_read_template 	= 	// 3 - pools_header
										"([0-9]{2})"					// 1 - leg
										"([GN])"						// 2 - pool mode
										"([0-9]{2})"					// 3 - segments
										"([0-9]{2})"					// 4 - segment
										"[0-9]{5}";

char* data_scan_pen_req_write_template =
										"	leg %02d	:\n "
										"		pool mode	:	%c	|"
										"	segment	:	%02d/%02d\n";

int scan_pend_req_read_frame(char* log, p_itsp_data_struct* data){

	p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
	log = read_digits(log, &ptr->pool_header.leg, 2);
	ptr->pool_header.pool_mode = *log++;
	log = read_digits(log, &ptr->pool_header.segments, 2);
	log = read_digits(log, &ptr->pool_header.segment, 2);
	memset(&ptr->pool_data, 0, sizeof(itsp_str_pool_data));
	return OK;

}

char* scan_pend_req_read_from(char* buff, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(buff, data_scan_pen_req_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
		read_digits(segments[1].start, &ptr->pool_header.leg, 2);
		ptr->pool_header.pool_mode = *segments[2].start;
		read_digits(segments[3].start, &ptr->pool_header.segments, 2);
		read_digits(segments[4].start, &ptr->pool_header.segment, 2);
		memset(&ptr->pool_data, 0, sizeof(itsp_str_pool_data));
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data scan pending / request\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return buff;

}

char* scan_pend_req_write_frame(char* buff, p_itsp_str_data_pools ptr){

	buff = write_digits(buff, ptr->pool_header.leg, 2, 0);
	*buff++ = ptr->pool_header.pool_mode;
	buff = write_digits(buff, ptr->pool_header.segments, 2, 0);
	buff = write_digits(buff, ptr->pool_header.segment, 2, 0);
	return buff;
}

int scan_pend_req_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_data_pools ptr = data->structure;

	fprintf(stream ,data_scan_pen_req_write_template,
							ptr->pool_header.leg,
							ptr->pool_header.pool_mode,
							ptr->pool_header.segment,
							ptr->pool_header.segments);

	return OK;
}

int scan_ack_final_read_frame(char* log, p_itsp_data_struct* data){

	p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
	memset(ptr, 0, sizeof(itsp_str_data_pools));
	read_amount(log, &ptr->pool_data.segment_total);
	return OK;

}

char* scan_ack_final_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, "([0-9,P-Y]*[@-I,`-i])[0-9]{5}", 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
		memset(ptr, 0, sizeof(itsp_str_data_pools));
		read_amount(segments[1].start, &ptr->pool_data.segment_total);
		return segments[1].end;
	}
	else{
		fprintf(std_err, "Match error : pools header\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;

}

char* scan_ack_final_write_frame(char* buff, p_itsp_str_data_pools ptr){
	buff = write_amount(buff, ptr->pool_data.segment_total, 0, 0, 4);
	return buff;

}

int scan_ack_final_log_to(FILE* stream, p_itsp_data_struct data){

		p_itsp_str_data_pools ptr = data->structure;

		fprintf(stream ,"	segment total = %06.2f\n",
						ptr->pool_data.segment_total);

	return OK;
}

void data_scan_free_frame(p_itsp_frame frame){

	p_itsp_str_data_scan fscan = NULL;
	p_itsp_str_scan_header sh = NULL;

	void* ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;

	if(ptr) {
		switch(frame->header->frame_type->message){
		case msg_pending:
		case msg_acknowledge:
			break;
		case msg_request:
			if(frame->header->frame_type->reason == reason_begin){
				 sh = ptr;
				if(sh->scan_runners) free_combo(sh->scan_runners);
				if(sh->live_runners) free_combo(sh->live_runners);
				if(sh->favorite_runners) free_combo(sh->favorite_runners);
				if(sh->sub_runners) free_combo(sh->sub_runners);
			}
			break;
		case msg_data:
			fscan = ptr;
			if(fscan->scan_header->scan_runners) free_combo(fscan->scan_header->scan_runners);
			if(fscan->scan_header->live_runners) free_combo(fscan->scan_header->live_runners);
			if(fscan->scan_header->favorite_runners) free_combo(fscan->scan_header->favorite_runners);
			if(fscan->scan_header->sub_runners) free_combo(fscan->scan_header->sub_runners);
			free(fscan->scan_header);
			if(fscan->scan_data->pool_data.columns && fscan->scan_data->pool_data.matrix_data){
				int i = 0;
				while(i < fscan->scan_data->pool_data.rows)
					free(fscan->scan_data->pool_data.matrix_data[i++]);
				free(fscan->scan_data->pool_data.matrix_data);
			}
			free(fscan->scan_data);
			break;
		}

		free(ptr);
	}
}

