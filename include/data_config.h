/*
 * data_config.h
 *
 *  Created on: 9 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_CONFIG_H_
#define INCLUDE_DATA_CONFIG_H_

extern int data_config_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_config_read_from(char* log, p_itsp_data_struct* frame);

extern char* data_config_write_frame(char* buff, p_itsp_str_data_config ptr);

extern int data_config_log_to(FILE* stream, p_itsp_data_struct frame);

extern void data_config_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_CONFIG_H_ */
