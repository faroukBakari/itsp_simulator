/*
 * data_file.c
 *
 *  Created on: 20 mars 2017
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



char* data_file_read_template			=	// 11 - data_file_transfert
										"(.{3})"						// 1 - destination
										"([0-9,a-z,A-Z, ,\\.,_]{32})"	// 2 - file name
										"([0-9]{8})"					// 3 - modification date
										"([0-9]{6})"					// 4 - modification time
										"([0-9,P-Y]*[@-I,`-i])"			// 5 - file size
										"([0-9,P-Y]*[@-I,`-i])"			// 6 - file segments
										"([0-9,P-Y]*[@-I,`-i])"			// 7 - current segment
										"([0-9,P-Y]*[@-I,`-i])";		// 8 - current segment size

int data_file_read_frame(char* log, p_itsp_data_struct* data){

	p_itsp_str_file p = (*data)->structure = malloc(sizeof(itsp_str_file));
	double tmp;
	memset(p,0,sizeof(itsp_str_file));

	log = read_fixed(log, p->file_header.destination, 3);
	log = read_fixed(log, p->file_header.name, 32);

	log = read_date(log, &p->file_header.date);
	log = read_time(log, &p->file_header.time);
	log = read_amount(log, &tmp); p->file_header.size = tmp;
	log = read_amount(log, &tmp); p->file_header.segments = tmp;
	log = read_amount(log, &tmp); p->file_header.current_segment = tmp;
	log = read_amount(log, &tmp); p->file_header.segment_size = tmp;
	long int nbchars = p->file_header.segment_size;
	if(nbchars && ((*data)->frame_type->message == msg_data)){
		p->file_data = malloc(nbchars + 1);
		log = read_fixed(log, p->file_data, nbchars);
	}
	else
		p->file_data = NULL;
	return OK;
}

char* data_file_read_from(char* buff, p_itsp_data_struct* data){
	const sub_str* segments = NULL;
	char *start = NULL, *s_ptr = NULL;
	double tmp;

	segments = match_expr(buff, data_file_read_template, 0);
	if(segments){
		start = segments->start;
		p_itsp_str_file p = (*data)->structure = malloc(sizeof(itsp_str_file));
		memset(p,0,sizeof(itsp_str_file));

		memcpy(p->file_header.destination, segments[1].start, 3);
		memcpy(p->file_header.name, segments[2].start, 32);
		read_date(segments[3].start, &p->file_header.date);
		read_time(segments[4].start, &p->file_header.time);
		read_amount(segments[5].start, &tmp); p->file_header.size = tmp;
		read_amount(segments[6].start, &tmp); p->file_header.segments = tmp;
		read_amount(segments[7].start, &tmp); p->file_header.current_segment = tmp;
		read_amount(segments[8].start, &tmp); p->file_header.segment_size = tmp;
		long int nbchars = p->file_header.segment_size;
		s_ptr = segments[8].end;
		if(nbchars && ((*data)->frame_type->message == msg_data)){
			replace_expr(s_ptr, "(\\\\010)", "\r");
			replace_expr(s_ptr, "(\\\\013)", "\n");
			p->file_data = malloc(nbchars + 1);
			memcpy(p->file_data, s_ptr, nbchars);
			p->file_data[nbchars] = 0;
			s_ptr += nbchars + CHECKSUM_SIZE;
		}
		else
			p->file_data = NULL;

		(*data)->data_log = sub2str(SUB_STRING(start, s_ptr));
		return s_ptr + nbchars;

	}
	else{
		fprintf(std_err, "Match error : data file\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return buff;
}

char* data_file_write_frame(char* buff, p_itsp_str_file p){

	buff = write_fixed(buff, p->file_header.destination, 3);
	buff += sprintf(buff, "%-32s", p->file_header.name);

	buff = write_date(buff, &p->file_header.date);
	buff = write_time(buff, &p->file_header.time);
	buff = write_amount(buff, p->file_header.size, 0, 0, 4);
	buff = write_amount(buff, p->file_header.segments, 0, 0, 4);
	buff = write_amount(buff, p->file_header.current_segment, 0, 0, 4);
	buff = write_amount(buff, p->file_header.segment_size, 0, 0, 4);
	if(p->file_header.segment_size && p->file_data)
		buff = write_fixed(buff, p->file_data, p->file_header.segment_size);
	return buff;
}

char* data_file_write_template			=
									"	destination			:	%02s\n"
									"	file name			:	%s\n"
									"	modification date	:	%02d/%02d/%04d\n"
									"	modification date	:	%02d:%02d:%02d\n"
									"	file size		:	%6.0d	|"
									"	file segments	:	%6.0d\n"
									"	segment size 	:	%6.0d	|"
									"	current segment	:	%6.0d\n"
									"File data : \n	%s\n";

int data_file_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_file ptr = data ? data->structure : NULL;

	if(ptr)
		fprintf(stream ,data_file_write_template,
					ptr->file_header.destination,
					ptr->file_header.name,
					ptr->file_header.date.day,
					ptr->file_header.date.month,
					ptr->file_header.date.year,
					ptr->file_header.time.hour,
					ptr->file_header.time.minutes,
					ptr->file_header.time.seconds,
					ptr->file_header.size,
					ptr->file_header.segments,
					ptr->file_header.current_segment,
					ptr->file_header.segment_size,
					(ptr->file_data && ptr->file_data)? ptr->file_data : " ");

	return OK;
}

void data_file_free_frame(p_itsp_frame frame){

	p_itsp_str_file ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;

	if(ptr) {
		if(ptr->file_data) free(ptr->file_data);
		free(ptr);
	}
}
