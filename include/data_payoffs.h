/*
 * data_payoffs.h
 *
 *  Created on: 14 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_PAYOFFS_H_
#define INCLUDE_DATA_PAYOFFS_H_

extern int data_payoffs_read_frame(char* log, p_itsp_data_struct* data, char* pool_code);

extern char* data_payoffs_read_from(char* log, p_itsp_data_struct* data);

extern char* data_payoffs_write_frame(char* buff, p_itsp_str_data_payoffs ptr);

extern int data_payoffs_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_payoffs_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_PAYOFFS_H_ */
