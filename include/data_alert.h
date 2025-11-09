/*
 * data_alert.h
 *
 *  Created on: 15 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_ALERT_H_
#define INCLUDE_DATA_ALERT_H_

extern int data_alert_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_alert_read_from(char* log, p_itsp_data_struct* data);

extern char* data_alert_write_frame(char*buff, p_itsp_str_alert ptr);

extern int data_alert_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_alert_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_ALERT_H_ */
