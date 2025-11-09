/*
 * data_will_pay.h
 *
 *  Created on: 15 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_DATA_WILL_PAY_H_
#define INCLUDE_DATA_WILL_PAY_H_

extern int data_will_pay_read_frame(char* log, p_itsp_data_struct* data);

extern char* data_will_pay_read_from(char* log, p_itsp_data_struct* data);

extern char* data_will_pay_write_frame(char* buff, p_itsp_str_will_pay ptr);

extern int data_will_pay_log_to(FILE* stream, p_itsp_data_struct data);

extern void data_will_pay_free_frame(p_itsp_frame frame);

#endif /* INCLUDE_DATA_WILL_PAY_H_ */
