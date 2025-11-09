/*
 * itsp_data.h
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_DATA_H_
#define INCLUDE_ITSP_DATA_H_

#include <itsp_structs.h>

extern int data_read_frame(char* ptr, p_itsp_frame data);

extern char* data_read_from(char* ptr, p_itsp_data_struct data);

extern char* data_write_frame(char* buff, p_itsp_data_struct dt);

extern int data_log_to(FILE* stream, p_itsp_data_struct data);

extern void free_itsp_frame_data(p_itsp_frame frame);

#endif /* INCLUDE_ITSP_DATA_H_ */
