/*
 * data_pools.c
 *
 *  Created on: 10 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>
#include <math.h>

char* data_pools_h_read_template 	= 	// 3 - pools_header
										"([GN])"						// 1 - pool mode
										"([0-9]{2})"					// 2 - segments
										"([0-9]{2})"					// 3 - segment
										"[0-9]{5}";

char* data_pools_h_write_template =
										"	pool mode : %c	|"
										"	segment	:	%02d/%02d\n";

char* data_pools_read_template 	= 		// 3 - data_pools
										"([GN])"						// 1 - pool mode
										"([0-9]{2})"					// 2 - segments
										"([0-9]{2})"					// 3 - segment
										"([0-9]{2})"					// 4 - rows
										"([0-9]{2})"					// 5 - columns
										"(([0-9,P-Y]*[@-I,`-i])*)"		// 6 - data matrix (amounts)
										"([0-9,P-Y]*[@-I,`-i])"			// 7 - segment total
										"([0-9,P-Y]*[@-I,`-i])"			// 8 - total
										"([0-9,P-Y]*[@-I,`-i])"			// 9 - net total
										"[0-9]{5}";

char* data_pools_write_template 	=
										"	pool mode	:	%c		|"
										"	segment	: %02d/%02d		|"
										"	matrix dims	: %d x %d\n"
										"	seg. total	: %06.2f	|"
										"	total	: %06.2f	|"
										"	net total	: %06.2f\n";

int data_pools_read_frame(char* log, p_itsp_data_struct* data){

	int i = 0, j = 0;
	double row_total = 0.0, row_total_count = 0.0;
	p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
	ptr->pool_header.pool_mode = *log++;
	log = read_digits(log, &ptr->pool_header.segments, 2);
	log = read_digits(log, &ptr->pool_header.segment, 2);
	log = read_digits(log, &ptr->pool_data.rows, 2);
	log = read_digits(log, &ptr->pool_data.columns, 2);

	if(ptr->pool_data.rows){
		ptr->pool_data.matrix_data = malloc(ptr->pool_data.rows * sizeof(double*));
		for(i = 0 ; i < ptr->pool_data.rows; i++){
			ptr->pool_data.matrix_data[i] = malloc(ptr->pool_data.columns * sizeof(double));
			row_total_count = 0;
			for(j = 0; j < ptr->pool_data.columns; j++){
				log = read_amount(log, &ptr->pool_data.matrix_data[i][j]);
				row_total_count += ptr->pool_data.matrix_data[i][j];
			}
			log = read_amount(log, &row_total);
			if(isequal_double(row_total_count, row_total, TOLERANCE)){
				DEBUG_WARN("total row mismatch! (%f <> %f", row_total_count, row_total);
				return FAULT;
			}
		}
	}

	log = read_amount(log, &ptr->pool_data.segment_total);
	log = read_amount(log, &ptr->pool_data.total);
	log = read_amount(log, &ptr->pool_data.net_total);
	return OK;
}

int data_pools_ack_read_frame(char* log, p_itsp_data_struct* data){
	p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
	memset(ptr, 0, sizeof(itsp_str_data_pools));
	read_amount(log, &ptr->pool_data.segment_total);
	return OK;

}

char* data_pools_ack_read_from(char* log, p_itsp_data_struct* data){
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

char* data_pools_ack_write_frame(char* buff, p_itsp_str_data_pools ptr){
	buff = write_amount(buff, ptr->pool_data.segment_total,0,0, 4);
	return buff;
}

int data_pools_h_read_frame(char* log, p_itsp_data_struct* data){

	p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
	ptr->pool_header.pool_mode = *log++;
	log = read_digits(log, &ptr->pool_header.segments, 2);
	log = read_digits(log, &ptr->pool_header.segment, 2);
	memset(&ptr->pool_data, 0, sizeof(itsp_str_pool_data));
	return OK;

}

char* data_pools_h_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, data_pools_h_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
		ptr->pool_header.pool_mode = *segments[1].start;
		read_digits(segments[2].start, &ptr->pool_header.segments, 2);
		read_digits(segments[3].start, &ptr->pool_header.segment, 2);
		memset(&ptr->pool_data, 0, sizeof(itsp_str_pool_data));
		return segments[0].end;
	}
	else{
		DEBUG_WARN("Match error : pools header");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;

}

char* data_pools_h_write_frame(char* buff, p_itsp_str_data_pools ptr){

	*buff++ = ptr->pool_header.pool_mode;
	buff = write_digits(buff, ptr->pool_header.segments, 2, 0);
	buff = write_digits(buff, ptr->pool_header.segment, 2, 0);

	return buff;
}

char* data_pools_read_from(char* log, p_itsp_data_struct* data){
	char tmp[6];
	char *s = NULL;
	memset(tmp,0,6);
	int i = 0, j = 0;
	const sub_str* segments = NULL;
	double row_total = 0.0, row_total_count = 0.0;

	segments = match_expr(log, data_pools_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_pools ptr = (*data)->structure = malloc(sizeof(itsp_str_data_pools));
		ptr->pool_header.pool_mode = *segments[1].start;
		read_digits(segments[2].start, &ptr->pool_header.segments, 2);
		read_digits(segments[3].start, &ptr->pool_header.segment, 2);
		read_digits(segments[4].start, &ptr->pool_data.rows, 2);
		read_digits(segments[5].start, &ptr->pool_data.columns, 2);

		if(ptr->pool_data.rows){
			s = segments[6].start;
			ptr->pool_data.matrix_data = malloc(ptr->pool_data.rows * sizeof(double*));
			for(i = 0 ; i < ptr->pool_data.rows; i++){
				ptr->pool_data.matrix_data[i] = malloc(ptr->pool_data.columns * sizeof(double));
				row_total_count = 0;
				for(j = 0; j < ptr->pool_data.columns; j++){
					s = read_amount(s, &ptr->pool_data.matrix_data[i][j]);
					row_total_count += ptr->pool_data.matrix_data[i][j];
				}
				s = read_amount(s, &row_total);
				if(isequal_double(row_total_count, row_total, TOLERANCE))
					DEBUG_WARN("total row mismatch! (%f <> %f", row_total_count, row_total);
			}
		}

		s = read_amount(segments[8].start, &ptr->pool_data.segment_total);
		s = read_amount(s, &ptr->pool_data.total);
		s = read_amount(s, &ptr->pool_data.net_total);
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data pools\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

char* data_pools_write_frame(char* buff, p_itsp_str_data_pools ptr){

	*buff++ = ptr->pool_header.pool_mode;
	buff = write_digits(buff, ptr->pool_header.segments, 2, 0);
	buff = write_digits(buff, ptr->pool_header.segment, 2, 0);
	buff = write_digits(buff, ptr->pool_data.rows, 2, 0);
	buff = write_digits(buff, ptr->pool_data.columns, 2, 0);

	if(ptr->pool_data.rows)
		for(int i = 0 ; i < ptr->pool_data.rows; i++){
			double row_amout = 0.0;
			for(int j = 0; j < ptr->pool_data.columns; j++){
				buff = write_amount(buff, ptr->pool_data.matrix_data[i][j], 0, 0, 4);
				row_amout += ptr->pool_data.matrix_data[i][j];
			}
			buff = write_amount(buff, row_amout, 0, 0, 4);
		}

	buff = write_amount(buff, ptr->pool_data.segment_total, 0, 0, 4);
	buff = write_amount(buff, ptr->pool_data.total, 0, 0, 4);
	buff = write_amount(buff, ptr->pool_data.net_total, 0, 0, 4);

	return buff;
}

int data_pools_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_data_pools ptr = data->structure;

	fprintf(stream ,data_pools_write_template,
							ptr->pool_header.pool_mode,
							ptr->pool_header.segment,
							ptr->pool_header.segments,
							ptr->pool_data.rows,
							ptr->pool_data.columns,
							ptr->pool_data.segment_total,
							ptr->pool_data.total,
							ptr->pool_data.net_total);

	if(ptr->pool_data.rows && ptr->pool_data.matrix_data){
		int i = 0, j = 0;
		fprintf(stream ,"\tmatrix data :\n");
		for(i = 0 ; i < ptr->pool_data.rows; i++){
			fprintf(stream ,"\t\t");
			for(j = 0; j < ptr->pool_data.columns; j++)
				fprintf(stream ,"%10.2f	",ptr->pool_data.matrix_data[i][j]);
			fprintf(stream ,"\n");
		}
	}
	return OK;
}

int data_pools_h_log_to(FILE* stream, p_itsp_data_struct data){


	p_itsp_str_data_pools ptr = data->structure;

	fprintf(stream ,data_pools_h_write_template,
					ptr->pool_header.pool_mode,
					ptr->pool_header.segment,
					ptr->pool_header.segments);


	return OK;
}

int data_pools_ack_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_data_pools ptr = data->structure;

	fprintf(stream ,"	segment total = %06.2f\n",
					ptr->pool_data.segment_total);

	return OK;
}

void data_pools_free_frame(p_itsp_frame frame){
	p_itsp_str_data_pools ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;
	if(ptr) {
		if(ptr->pool_data.columns && ptr->pool_data.matrix_data){
			int i = 0;
			while(i < ptr->pool_data.rows)
				free(ptr->pool_data.matrix_data[i++]);
			free(ptr->pool_data.matrix_data);
		}
		free(ptr);
	}
}

























