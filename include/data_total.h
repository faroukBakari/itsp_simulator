/*
 * data_total.h
 *
 *  Created on: 13 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_TOTAL_H_
#define INCLUDE_DATA_TOTAL_H_

extern int data_totals_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_totals_read_from(char* log, p_itsp_data_struct* data);

extern char* data_totals_write_frame(char* buff, p_itsp_str_data_totals ptr);

extern int data_totals_write_to(FILE* stream, p_itsp_data_struct data);

extern void data_totals_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_TOTAL_H_ */
