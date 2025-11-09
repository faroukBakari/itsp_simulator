/*
 * itsp_data.c
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>
#include <data_link.h>
#include <data_config.h>
#include <data_race_status.h>
#include <data_pools.h>
#include <data_total.h>
#include <data_scan.h>
#include <data_payoffs.h>
#include <data_results.h>
#include <data_alert.h>
#include <data_will_pay.h>
#include <data_file.h>

char* negative_ack_msg_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, "([ -~]+)[0-9]{5}", 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		(*data)->structure = sub2str(segments[1]);
		return segments->end;
	}
	else{
		DEBUG_WARN("Match error : negative ack msg\n");
		(*data)->structure = NULL;
		return NULL;
	}
}

int negative_ack_msg_read_frame(char* log, p_itsp_data_struct* data){

	long unsigned int str_ln = strlen(log);
	(*data)->data_log = malloc(str_ln + 1);
	strcpy((*data)->data_log, log);

	(*data)->structure = malloc(str_ln - CHECKSUM_SIZE + 1);
	memcpy((*data)->structure, log, str_ln - CHECKSUM_SIZE);
	((char*)((*data)->structure))[str_ln - CHECKSUM_SIZE] = 0;

	return OK;

}

char* negative_ack_msg_write_frame(char* buff, char* data){
	buff += sprintf(buff, "%s", data);
	return buff;
}

int negative_ack_msg_log_to(FILE* stream, p_itsp_data_struct data){
	TRACE(stream, "message	:	%s\n", (char*)data->structure);
	return OK;
}

void negative_ack_msg_free_frame(p_itsp_frame frame){
	if(frame->data_str->structure)
		free(frame->data_str->structure);
}

/*****************************************************************************/

char* data_write_frame(char* buff, p_itsp_data_struct dt){
	p_itsp_frame_type frame_type = dt->frame_type;
	void* data = dt->structure;

	if((frame_type->message == msg_acknowledge)
		&& (65 <= frame_type->reason) && (frame_type->reason <= 90))
		return negative_ack_msg_write_frame(buff, data);

	if(frame_type->data == data_link){
		return data_link_write_frame(buff, data);
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_config)){
		return data_config_write_frame(buff, data);
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_race_status))
		return data_rs_write_frame(buff, data);

	if((frame_type->data == data_pools)){
		if(frame_type->message == msg_data)
			return data_pools_write_frame(buff, data);
		else{
			if(frame_type->message != msg_acknowledge)
				return data_pools_h_write_frame(buff, data);
			else
				return data_pools_ack_write_frame(buff, data);
		}
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_pool_total)){
		return data_totals_write_frame(buff, data);
	}

	if(frame_type->data == data_scan){
		if((frame_type->message == msg_request) &&
				(frame_type->reason == reason_begin))
		{
			return scan_request_begin_write_frame(buff, data);
		}

		if((frame_type->message == msg_data) &&
				(frame_type->reason == reason_final))
		{
			return data_scan_write_frame(buff, data);
		}

		if(((frame_type->message == msg_pending) ||
				(frame_type->message == msg_request)) &&
				(frame_type->reason == reason_final))
		{
			return scan_pend_req_write_frame(buff, data);
		}

		if((frame_type->message == msg_acknowledge) &&
				(frame_type->reason == reason_final))
		{
			return scan_ack_final_write_frame(buff, data);
		}
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_payoffs)){
		return data_payoffs_write_frame(buff, data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_results)){
		return data_results_write_frame(buff, data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_alert)){
		return data_alert_write_frame(buff, data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_will_pay)){
		return data_will_pay_write_frame(buff, data);
	}

	if(frame_type->data == data_file_transfert){
		return data_file_write_frame(buff, data);
	}

	DEBUG_WARN("\ndata_write_frame : unhandled data type : %c%c-%c",
			frame_type->message,
			frame_type->data,
			frame_type->reason);

	return NULL;
}

int data_read_frame(char* ptr, p_itsp_frame frame){
	p_itsp_data_struct data = frame->data_str;
	p_itsp_frame_type frame_type = data->frame_type;

	if((frame_type->message == msg_acknowledge)
		&& (65 <= frame_type->reason) && (frame_type->reason <= 90))
		return negative_ack_msg_read_frame(ptr, &data);

	if(frame_type->data == data_link){
		return data_link_read_frame(ptr, &data);
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_config)){
		return data_config_read_frame(ptr, &data);
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_race_status))
		return data_rs_read_frame(ptr, &data);

	if((frame_type->data == data_pools)){
		if(frame_type->message == msg_data)
			return data_pools_read_frame(ptr, &data);
		else{
			if(frame_type->message != msg_acknowledge)
				return data_pools_h_read_frame(ptr, &data);
			else
				return data_pools_ack_read_frame(ptr, &data);
		}
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_pool_total)){
		return data_totals_read_frame(ptr, &data);
	}

	if(frame_type->data == data_scan){
		if((frame_type->message == msg_request) &&
				(frame_type->reason == reason_begin))
		{
			return scan_request_begin_read_frame(ptr, &data, frame->header->pool_code);
		}

		if((frame_type->message == msg_data) &&
				(frame_type->reason == reason_final))
		{
			return data_scan_read_frame(ptr, &data, frame->header->pool_code);
		}

		if(((frame_type->message == msg_pending) ||
				(frame_type->message == msg_request)) &&
				(frame_type->reason == reason_final))
		{
			return scan_pend_req_read_frame(ptr, &data);
		}

		if((frame_type->message == msg_acknowledge) &&
				(frame_type->reason == reason_final))
		{
			return scan_ack_final_read_frame(ptr, &data);
		}
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_payoffs)){
		return data_payoffs_read_frame(ptr, &data, frame->header->pool_code);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_results)){
		return data_results_read_frame(ptr, &data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_alert)){
		return data_alert_read_frame(ptr, &data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_will_pay)){
		return data_will_pay_read_frame(ptr, &data);
	}

	if(frame_type->data == data_file_transfert){
		return data_file_read_frame(ptr, &data);
	}

	DEBUG_WARN("\ndata_read_frame : unhandled data type : %c%c-%c\ndata_scope:\n%s\n",
			frame_type->message,
			frame_type->data,
			frame_type->reason,
			ptr);

	return reason_unhandled_data;
}

char* data_read_from(char* ptr, p_itsp_data_struct data){
	p_itsp_frame_type frame_type = data->frame_type;

	if((frame_type->message == msg_acknowledge)
		&& (65 <= frame_type->reason) && (frame_type->reason <= 90))
		return negative_ack_msg_read_from(ptr, &data);

	if(frame_type->data == data_link){
		return data_link_read_from(ptr, &data);
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_config)){
		return data_config_read_from(ptr, &data);
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_race_status))
		return data_rs_read_from(ptr, &data);

	if((frame_type->data == data_pools)){
		if(frame_type->message == msg_data)
			return data_pools_read_from(ptr, &data);
		else{
			if(frame_type->message != msg_acknowledge)
				return data_pools_h_read_from(ptr, &data);
			else
				return data_pools_ack_read_from(ptr, &data);
		}
	}

	if((frame_type->message == msg_data) && (frame_type->data == data_pool_total)){
		return data_totals_read_from(ptr, &data);
	}

	if(frame_type->data == data_scan){
		if((frame_type->message == msg_request) &&
				(frame_type->reason == reason_begin))
		{
			return scan_request_begin_read_from(ptr, &data);
		}

		if((frame_type->message == msg_data) &&
				(frame_type->reason == reason_final))
		{
			return data_scan_read_from(ptr, &data);
		}

		if(((frame_type->message == msg_pending) ||
				(frame_type->message == msg_request)) &&
				(frame_type->reason == reason_final))
		{
			return scan_pend_req_read_from(ptr, &data);
		}

		if((frame_type->message == msg_acknowledge) &&
				(frame_type->reason == reason_final))
		{
			return scan_ack_final_read_from(ptr, &data);
		}
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_payoffs)){
		return data_payoffs_read_from(ptr, &data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_results)){
		return data_results_read_from(ptr, &data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_alert)){
		return data_alert_read_from(ptr, &data);
	}

	if((frame_type->message == msg_data) &&
			(frame_type->data == data_will_pay)){
		return data_will_pay_read_from(ptr, &data);
	}

	if(frame_type->data == data_file_transfert){
		return data_file_read_from(ptr, &data);
	}

	DEBUG_WARN("\ndata_read_from : unhandled data type : %c%c-%c\ndata_scope:\n%s\n",
			frame_type->message,
			frame_type->data,
			frame_type->reason,
			ptr);

	return NULL;
}

int data_log_to(FILE* stream, p_itsp_data_struct data){

	if(data){
		fprintf(stream, "Data	: %s\n", data->data_log ? data->data_log : "");

		if((data->frame_type->message == msg_acknowledge)
			&& (65 <= data->frame_type->reason) && (data->frame_type->reason <= 90))
			return negative_ack_msg_log_to(stream, data);

		if((data->frame_type->data == data_link)){
			return data_link_log_to(stream, data);
		}

		if((data->frame_type->data == data_config) &&
		   (data->frame_type->message == msg_data)){
			return data_config_log_to(stream, data);
		}

		if((data->frame_type->data == data_race_status)){
			return data_rs_log_to(stream, data);
		}

		if((data->frame_type->data == data_pools)){
			if(data->frame_type->message == msg_data)
				return data_pools_log_to(stream, data);
			else{
				if(data->frame_type->message != msg_acknowledge)
					return data_pools_h_log_to(stream, data);
				else
					return data_pools_ack_log_to(stream, data);
			}
		}

		if((data->frame_type->data == data_pool_total)){
			return data_totals_write_to(stream, data);
		}

		if(data->frame_type->data == data_scan){
			if((data->frame_type->message == msg_request) &&
						(data->frame_type->reason == reason_begin))
			{
				return scan_request_begin_log_to(stream, data);
			}

			if((data->frame_type->message == msg_data) &&
						(data->frame_type->reason == reason_final))
			{
				return data_scan_log_to(stream, data);
			}

			if(((data->frame_type->message == msg_pending) ||
					(data->frame_type->message == msg_request)) &&
					(data->frame_type->reason == reason_final))
			{
				return scan_pend_req_log_to(stream, data);
			}

			if((data->frame_type->message == msg_acknowledge) &&
					(data->frame_type->reason == reason_final))
			{
				return scan_ack_final_log_to(stream, data);
			}


		}

		if((data->frame_type->data == data_payoffs) &&
			(data->frame_type->message == msg_data))
		{
			return data_payoffs_log_to(stream, data);
		}

		if((data->frame_type->data == data_results) &&
			(data->frame_type->message == msg_data))
		{
			return data_results_log_to(stream, data);
		}

		if((data->frame_type->data == data_alert) &&
				(data->frame_type->message == msg_data))
		{
			return data_alert_log_to(stream, data);
		}

		if((data->frame_type->data == data_will_pay) &&
				(data->frame_type->message == msg_data))
		{
			return data_will_pay_log_to(stream, data);
		}

		if(data->frame_type->data == data_file_transfert)
		{
			return data_file_log_to(stream, data);
		}

	}
	return FAULT;
}

void free_itsp_frame_data(p_itsp_frame frame){
	p_itsp_frame_type frame_type = frame->header->frame_type;

	if(frame->data_str->data_log)
		free(frame->data_str->data_log);

	if(
		(frame_type->message == msg_acknowledge) &&
		(65 <= frame_type->reason) &&
		(frame_type->reason <= 90)
	  )
		negative_ack_msg_free_frame(frame);
	else
		switch(frame_type->data){
		case data_link :
			data_link_free_frame(frame);
			break;
		case data_config :
			data_config_free_frame(frame);
			break;
		case data_race_status :
			data_rs_free_frame(frame);
			break;
		case data_pools :
			data_pools_free_frame(frame);
			break;
		case data_pool_total :
			data_totals_free_frame(frame);
			break;
		case data_scan :
			data_scan_free_frame(frame);
			break;
		case data_payoffs :
			data_payoffs_free_frame(frame);
			break;
		case data_results :
			data_results_free_frame(frame);
			break;
		case data_alert :
			data_alert_free_frame(frame);
			break;
		case data_will_pay :
			data_will_pay_free_frame(frame);
			break;
		case data_file_transfert :
			data_file_free_frame(frame);
			break;
		default :
			DEBUG_WARN("data_free_frame : unhandled data type : %c%c-%c",
					frame_type->message,
					frame_type->data,
					frame_type->reason);
		}

	if(frame->data_str) free(frame->data_str);

}





























