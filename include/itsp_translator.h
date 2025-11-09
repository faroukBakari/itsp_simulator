/*
 * itsp_translator.h
 *
 *  Created on: 30 mai 2017
 *      Author: f.baccari
 */

#ifndef ITSP_TRANSLATOR_H_
#define ITSP_TRANSLATOR_H_

#include <itsp_structs.h>

#define DEF_LENTH (long int)1000

extern char* load_log_file(char* file_name, FILE* res_load);

extern p_itsp_frame* load_frames(FILE* log, FILE* res_read);

extern void print_log_data(FILE* stream, p_itsp_frame* itsp_data);

extern p_itsp_sequence* cluster_sequences(FILE* analyse_log, p_itsp_frame* itsp_data);

extern void log_sequence(FILE* analyse_log, p_itsp_sequence seq);

extern void log_sequences(FILE* analyse_log, p_itsp_sequence* sqz);

#endif /* ITSP_TRANSLATOR_H_ */
