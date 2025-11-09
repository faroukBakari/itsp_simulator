/*
 * itsp_frame.c
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <i_tools.h>
#include <i_string.h>
#include <i_file.h>
#include <itsp_structs.h>
#include <itsp_header.h>
#include <itsp_data.h>
#include <itsp_common.h>
#include <itsp_frame.h>

#define BUFFER_SZ 2048

int MODE_FRANCAIS = 0;

char* hdr  =				"[PRDA]"						// 1 - message type
							"[CPXWT$SRALF]"					// 2 - data type
							".{3}"							// 3 - source
							"[0-9]{2}"						// 4 - sequence
							".{3}"							// 5 - event code
							"[0-9,\\.]{2}"					// 6 - race number
							".{3}"							// 7 - pool code
							"[ befhrHDRPCSIATX\\?]"			// 8 - reason
							"[0-9]{2}[0-9]{2}[0-9]{2}"		// 9 - time
							"[0-9]{4}"						// 10 - lenth
							"[0-9]{5}";						// 11 - check sum



int read_net_frame(p_s_net_frame net_fr, p_itsp_frame frame, FILE* res_read){

	if(!itsp_header_check_sum(net_fr->data)){
		DEBUG_TOFILE(res_read, "HEADER CHECKSUM ERROR on frame : \"%s\"",net_fr->data);
		return reason_data_checksum;
	}

	int dt_sz = strlen(net_fr->data) - HEADER_SIZE;
	if((dt_sz > 0) && !itsp_data_check_sum(net_fr->data + HEADER_SIZE, dt_sz)){
		DEBUG_TOFILE(res_read, "DATA CHECKSUM ERROR on frame : \"%s\"",net_fr->data + HEADER_SIZE);
		return reason_data_checksum;
	}

	char *source = net_fr->data;
	//replace_expr(source, "(2/4)[^/\\-\\,]", "2#4");

	memset(frame, 0, sizeof(itsp_frame));
	frame->date_time = (long)net_fr->date_time;
	frame->header = malloc(sizeof(itsp_header));

	source = header_read_from(source, frame->header);

	frame->frame_num = net_fr->frame_id;

	if(frame->header->ldata){
		frame->data_str = malloc(sizeof(itsp_data_struct));
		frame->data_str->frame_type = frame->header->frame_type;
		frame->data_str->data_log = malloc(frame->header->ldata + 1);
		memcpy(frame->data_str->data_log, source, frame->header->ldata);
		frame->data_str->data_log[frame->header->ldata] = 0;
		int out = reason_invalid_data;
		if(!(out = data_read_frame(source, frame))){
			free(frame->data_str);
			frame->data_str = NULL;
			DEBUG_TOFILE(res_read, "data_read_from error : \"%s\"", source);
			return out;
		}
	}

	return OK;
}

p_s_net_frame write_net_frame(p_itsp_frame frame, FILE* res_read){

	char h_start[4096] = {0}, *h_end = NULL, *d_start = NULL, *d_end = NULL;

	h_end = h_start + HEADER_SIZE - CHECKSUM_SIZE;

	if(frame->data_str){
		d_start = h_end + CHECKSUM_SIZE;
		d_end = data_write_frame(d_start, frame->data_str);
		frame->header->ldata = d_end - d_start + CHECKSUM_SIZE;
		header_write_frame(h_start, frame->header);
		//replace_expr(h_start, "(2#4)", "2/4");
		itsp_write_check_sum(h_start, h_end);
		itsp_write_check_sum(d_start, d_end);
	}else{
		frame->header->ldata = 0;
		header_write_frame(h_start, frame->header);
		//replace_expr(h_start, "(2#4)", "2/4");
		itsp_write_check_sum(h_start, h_end);
	}

	return make_net_frame(h_start, HEADER_SIZE + frame->header->ldata);
}

int read_log_frame(p_itsp_frame* fr, FILE* log, FILE* res_read){

	static char *buff = NULL;
	static long unsigned int buff_size = 0;
	long int start = 0, end = 0, sz = 0;;
	int out = FAULT;
	p_itsp_frame frame = malloc(sizeof(itsp_frame));
	const sub_str *sb = NULL;

	*fr = frame;
	frame->data_str = NULL;
	frame->header = NULL;
	get_date_time(&frame->date_time, NULL);




	while((sz = get_line(NULL, log))){
		if(!buff  || ((sz + 1) > buff_size)) buff = realloc(buff, (buff_size = sz + 1));
		get_line(buff, log);
		replace_expr(buff, "(2/4)[^/\\-\\,]", "2#4");
		if((sb = match_expr(buff, hdr,0)))
			break;
	}

	if(sb){
		out = OK;

		frame->header = malloc(sizeof(itsp_header));
		header_read_from(sb->start, frame->header);
		start = ftell(log) - strlen(sb->end);

		if(frame->header->ldata){

			if(!(sb = match_expr(sb->end, hdr,0)))
				while((sz = get_line(NULL, log))){

					if((sz + 1) > buff_size) buff = realloc(buff, (buff_size = sz + 1));

					get_line(buff, log);

					if((sb = match_expr(buff, hdr,0))) break;

				}

			end = ftell(log) - ((sb) ? strlen(sb->start) : 0);

			if(end - start > 0){
				if((end - start + 1) > buff_size)
					buff = realloc(buff, (buff_size = end - start + 1));

				fseek(log, start, SEEK_SET);
				get_chars(buff, end - start, log);
				buff[end - start] = 0;

				if((frame->header->frame_type->data != data_link) ||
				   (frame->header->frame_type->data != data_alert))
					replace_expr(buff, "(\\| *[\r\n]+ +\\|)", "");


				replace_expr(buff, "(2/4)[^/\\-\\,]", "2#4");


				frame->data_str = malloc(sizeof(itsp_data_struct));
				frame->data_str->frame_type = frame->header->frame_type;
				if(buff && !data_read_from(buff, frame->data_str)){
					free(frame->data_str);
					frame->data_str = NULL;
				}
				if(buff) {
					free(buff);
					buff = NULL;
				}
			}
			else{
				DEBUG_TOFILE(stderr, "Empty data scope for non_empty data frame");
				abort();
			}
		}
	}
	else{
		free(frame);
		*fr = NULL;
	}
	return out;
}

int log_frame(FILE* stream, p_itsp_frame frame){
	if(!frame) return FAULT;
	fprintf(stream, "--------------------------------------------------------\n");
	fprintf(stream, "frame number : %d\n", frame->frame_num);
	header_log_to(stream, frame->header);
	if(frame->header->ldata){
		if(!frame->data_str) fprintf(stream, "!!WARNING!! : unhandled data frame\n");
		data_log_to(stream, frame->data_str);
	}
	fprintf(stream, "--------------------------------------------------------\n");
	return OK;
}

void free_itsp_frame(p_itsp_frame frame){
	if(frame->data_str)
		free_itsp_frame_data(frame);
	if(frame->header->header_log)
		free(frame->header->header_log);
	free(frame->header);
	free(frame);
}















