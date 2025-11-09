/*
 * data_scan.h
 *
 *  Created on: 13 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_SCAN_H_
#define INCLUDE_DATA_SCAN_H_

extern int scan_request_begin_read_frame(char* log, p_itsp_data_struct* data, char* pool_code);

extern char* scan_request_begin_read_from(char* log, p_itsp_data_struct* data);

extern char* scan_request_begin_write_frame(char* buff, p_itsp_str_scan_header ptr);

extern int scan_request_begin_log_to(FILE* stream, p_itsp_data_struct data);

extern int data_scan_read_frame(char* log, p_itsp_data_struct* data, char* pool_code);

extern char* data_scan_read_from(char* log, p_itsp_data_struct* data);

extern char* data_scan_write_frame(char* buff, p_itsp_str_data_scan ptr);

extern int data_scan_log_to(FILE* stream, p_itsp_data_struct data);

extern int scan_pend_req_read_frame(char* log, p_itsp_data_struct* data);

extern char* scan_pend_req_read_from(char* log, p_itsp_data_struct* data);

extern char* scan_pend_req_write_frame(char* buff, p_itsp_str_data_pools ptr);

extern int scan_pend_req_log_to(FILE* stream, p_itsp_data_struct data);

extern int scan_ack_final_read_frame(char* log, p_itsp_data_struct* data);

extern char* scan_ack_final_read_from(char* log, p_itsp_data_struct* data);

extern char* scan_ack_final_write_frame(char* buff, p_itsp_str_data_pools ptr);

extern int scan_ack_final_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_scan_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_SCAN_H_ */
