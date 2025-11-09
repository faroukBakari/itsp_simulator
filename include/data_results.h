/*
 * data_results.h
 *
 *  Created on: 14 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_RESULTS_H_
#define INCLUDE_DATA_RESULTS_H_

extern int data_results_read_frame(char* buff, p_itsp_data_struct* data);

extern char* data_results_read_from(char* log, p_itsp_data_struct* data);

extern char* data_results_write_frame(char* buff, p_itsp_str_data_results ptr);

extern int data_results_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_results_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_RESULTS_H_ */
