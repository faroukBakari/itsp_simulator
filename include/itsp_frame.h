/*
 * itsp_frame.h
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_FRAME_H_
#define INCLUDE_ITSP_FRAME_H_

#include <itsp_cnx.h>

extern int read_net_frame(p_s_net_frame net_fr, p_itsp_frame frame, FILE* res_read);

extern p_s_net_frame write_net_frame(p_itsp_frame frame, FILE* res_read);

extern int read_log_frame(p_itsp_frame* frame, FILE* log, FILE* res_read);

extern int log_frame(FILE* stream, p_itsp_frame frame);

extern void free_itsp_frame(p_itsp_frame frame);

#endif /* INCLUDE_ITSP_FRAME_H_ */
