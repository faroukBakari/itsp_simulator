/*
 * data_file.h
 *
 *  Created on: 20 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_FILE_H_
#define INCLUDE_DATA_FILE_H_

extern int data_file_read_frame(char* buff, p_itsp_data_struct* data);

extern char* data_file_read_from(char* log, p_itsp_data_struct* data);

extern char* data_file_write_frame(char* buff, p_itsp_str_file p);

extern int data_file_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_file_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_FILE_H_ */
