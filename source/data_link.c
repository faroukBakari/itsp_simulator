/*
 * data_link.c
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

//										// 0 - data_link
char* data_link_read_template = 		"(ITSP)"						// 1 - document
										"([0-9]{2})\\."					// 2 - version
										"([0-9]{2})"					// 3 - revision
										"([ -~]*)"			// 4 - link message (optional)
										"[0-9]{5}";
char* data_link_write_template =	"	protocol : %s %d.%d\n"
										"	message  : %s\n";

int data_link_read_frame(char* log, p_itsp_data_struct* data){

	size_t str_ln = strlen(log);
	char* last = log + str_ln - CHECKSUM_SIZE;

	p_itsp_str_data_link ptr = (*data)->structure = malloc(sizeof(itsp_str_data_link));
	log = read_fixed(log, ptr->identifier.document, ITSP_MAX_LENGTH_DOCUMENT_NAME); ptr->identifier.document[ITSP_MAX_LENGTH_DOCUMENT_NAME] = 0;
	log = read_digits(log, &ptr->identifier.version.version_number, 2);
	if(*log++ != '.'){
		TRACE(std_err,"dot separator");
		return FAULT;
	}
	log = read_digits(log, &ptr->identifier.version.revision_number, 2);
	ptr->text = sub2str(SUB_STRING(log, last));

	return OK;
}

char* data_link_read_from(char* log, p_itsp_data_struct* data){
	char tmp[6];
	memset(tmp,0,6);
	const sub_str* segments = NULL;
	segments = match_expr(log, data_link_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_data_link ptr = (*data)->structure = malloc(sizeof(itsp_str_data_link));
		memcpy(ptr->identifier.document,segments[1].start, segments[1].end - segments[1].start);
		ptr->identifier.document[segments[1].end - segments[1].start] = 0;
		memcpy(tmp,segments[2].start,2); ptr->identifier.version.version_number = atoi(tmp);
		memcpy(tmp,segments[3].start,2); ptr->identifier.version.revision_number = atoi(tmp);
		ptr->text = sub2str(segments[4]);
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : data link\n");
		(*data)->structure = NULL;
		return NULL;
	}
}

char* data_link_write_frame(char* buff, p_itsp_str_data_link ptr){
		buff += sprintf(buff, "%4s", ptr->identifier.document);
		buff = write_digits(buff, ptr->identifier.version.version_number, 2, 0);
		*buff++ = '.';
		buff = write_digits(buff, ptr->identifier.version.revision_number, 2, 0);
		if(ptr->text) buff += sprintf(buff, "%s", ptr->text);
		return buff;
}

int data_link_log_to(FILE* stream, p_itsp_data_struct data){
	p_itsp_str_data_link ptr = data->structure;

	fprintf(stream ,data_link_write_template,
			ptr->identifier.document,
			ptr->identifier.version.version_number,
			ptr->identifier.version.revision_number,
			ptr->text);

	return OK;
}

void data_link_free_frame(p_itsp_frame frame){

	p_itsp_str_data_link ptr = ( frame->data_str && frame->data_str->structure) ?
			frame->data_str->structure : NULL;
	if(ptr){
		free(ptr->text);
		free(ptr);
	}
}
