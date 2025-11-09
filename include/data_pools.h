/*
 * data_pools.h
 *
 *  Created on: 10 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_POOLS_H_
#define INCLUDE_DATA_POOLS_H_

extern int data_pools_ack_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_pools_ack_read_from(char* log, p_itsp_data_struct* data);

extern char* data_pools_ack_write_frame(char* buff, p_itsp_str_data_pools ptr);

extern int data_pools_h_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_pools_h_read_from(char* log, p_itsp_data_struct* data);

extern char* data_pools_h_write_frame(char* buff, p_itsp_str_data_pools ptr);

extern int data_pools_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_pools_read_from(char* log, p_itsp_data_struct* data);

extern char* data_pools_write_frame(char* buff, p_itsp_str_data_pools ptr);

extern int data_pools_log_to(FILE* stream, p_itsp_data_struct data);

extern int data_pools_h_log_to(FILE* stream, p_itsp_data_struct data);

extern int data_pools_ack_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_pools_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_POOLS_H_ */
