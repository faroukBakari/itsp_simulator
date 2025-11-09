/*
 * itsp_header.h
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_HEADER_H_
#define INCLUDE_ITSP_HEADER_H_

#include <itsp_frame.h>

extern char* header_read_from(char* log, p_itsp_header header);

extern char* header_write_frame(char* buff, p_itsp_header hr);

extern int header_log_to(FILE* stream, p_itsp_header header);

#endif /* INCLUDE_ITSP_HEADER_H_ */
