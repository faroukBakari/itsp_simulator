/*
 * data_link.h
 *
 *  Created on: 9 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_LINK_H_
#define INCLUDE_DATA_LINK_H_

extern int data_link_read_frame(char* log, p_itsp_data_struct* frame);

extern char* data_link_read_from(char* log, p_itsp_data_struct* frame);

extern char* data_link_write_frame(char* buff, p_itsp_str_data_link ptr);

extern int data_link_log_to(FILE* stream, p_itsp_data_struct frame);

extern void data_link_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_LINK_H_ */
