/*
 * data_alert.c
 *
 *  Created on: 15 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>

char* data_alert_read_template =				// 1 - data_result
												"([a-z,A-Z, ]{1})"				// 1 - type
												"([ -~]+)"						// 2 - message
												"[0-9]{5}";

char* data_alert_write_template =				"	type	:	%c	|"
												"	message	:	%s\n";

int data_alert_read_frame(char* log, p_itsp_data_struct* data){

		(*data)->data_log = malloc(strlen(log) + 1);
		strcpy((*data)->data_log, log);
		p_itsp_str_alert ptr = (*data)->structure = malloc(sizeof(itsp_str_alert));
		ptr->type = *log++;
		ptr->message = sub2str(SUB_STRING(log, log + (strlen(log) - CHECKSUM_SIZE)));
		return OK;
}

char* data_alert_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, data_alert_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_alert ptr = (*data)->structure = malloc(sizeof(itsp_str_alert));
		ptr->type = *segments[1].start;
		ptr->message = sub2str(segments[2]);
		return segments[0].end;
	}
	else{
		DEBUG_WARN("Match error : data alert\n");
		(*data)->structure = NULL;
		return NULL;
	}
}

char* data_alert_write_frame(char*buff, p_itsp_str_alert ptr){
	buff += sprintf(buff, "%c", ptr->type);
	buff += sprintf(buff, "%s", ptr->message);
	return buff;
}

int data_alert_log_to(FILE* stream, p_itsp_data_struct data){

	p_itsp_str_alert ptr = data->structure;

	fprintf(stream ,data_alert_write_template,
			ptr->type,
			ptr->message);

	return OK;
}

void data_alert_free_frame(p_itsp_frame frame){

	p_itsp_str_alert ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;

	if(ptr) {
		if(ptr->message) free(ptr->message);
		free(ptr);
	}
}
