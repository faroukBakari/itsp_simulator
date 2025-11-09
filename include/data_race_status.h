/*
 * data_race_status.h
 *
 *  Created on: 9 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_RACE_STATUS_H_
#define INCLUDE_DATA_RACE_STATUS_H_

extern int data_rs_read_frame(char* log, p_itsp_data_struct* frame);

extern char* data_rs_read_from(char* log, p_itsp_data_struct* frame);

extern char* data_rs_write_frame(char* buff, p_itsp_str_data_race_status ptr);

extern int data_rs_log_to(FILE* stream, p_itsp_data_struct frame);

extern void data_rs_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_RACE_STATUS_H_ */
