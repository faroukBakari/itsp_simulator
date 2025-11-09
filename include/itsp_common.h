/*
 * itsp_common.h
 *
 *  Created on: 9 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_COMMON_H_
#define INCLUDE_ITSP_COMMON_H_

#include <time.h>

extern long c_date(p_itsp_str_date d);
extern long c_time(p_itsp_str_time t);
extern long tm_2_long(struct tm *tm);
extern char* sprintf_date_time(char* buff, long ctime, char data_time_flag);
extern struct tm* long_2_tm(long *t);
extern long c_date_time_tm(p_itsp_str_date d, p_itsp_str_time t);
extern long itsp_date_time(long ctime, p_itsp_str_date d, p_itsp_str_time t);
extern void get_date_time(time_t *t, struct tm *tm);
extern char* read_amount(char* source, double* value);
extern char* write_amount(char* buff, double amount, int signed_flag, int can_be_dot, int precision);
extern char* read_time(char* source, p_itsp_str_time p_time);
extern char* write_time(char* source, p_itsp_str_time p_time);
extern char* read_date(char* source, p_itsp_str_date p_date);
extern char* write_date(char* source, p_itsp_str_date p_date);
extern char* read_digits(char* source, int* value, int nb_dgts);
extern char* write_digits(char* buff, int value, int nb_dgts, int can_be_dots);
extern char* read_fixed(char* source, char* out, long unsigned int nb_chars);
extern char* write_fixed(char* buff, char* in, long unsigned int nb_chars);
extern char* read_numeric_range(char* source, int** out);
extern char* write_numeric_range(char* buff, int* range);
extern long int mumeric_range_cmp(int* range1, int* range2);
extern int numeric_range_cardinal(int* range);
extern long int count_occurences(char* start, char* end, char c);
extern int itsp_header_check_sum(char* header);
extern int itsp_data_check_sum(char* data, long unsigned int data_size);
extern char* itsp_write_check_sum(char* start, char* end);
extern char* read_combo(char* buff, int*** combo_);
extern char* write_combo(char* buff, int** combo);
extern long	combo_cmp_func(const void* combo1, const void* combo2);
extern int** clone_combo(int** combo);
extern int** make_empty_combo(int i);
extern int combo_cardinal(int** combo);
extern void free_combo(int** combo);

#define print_combo(stream, combo)({\
	char buff_do_not_use_this[1024]={0};\
	write_combo(buff_do_not_use_this, combo);\
	fprintf(stream, "%s", buff_do_not_use_this);\
})

#endif /* INCLUDE_ITSP_COMMON_H_ */
