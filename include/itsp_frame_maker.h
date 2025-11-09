/*
 * itsp_frame_maker.h
 *
 *  Created on: Sep 6, 2017
 *      Author: farouk
 */

#ifndef INCLUDE_ITSP_FRAME_MAKER_H_
#define INCLUDE_ITSP_FRAME_MAKER_H_

#define FILE_SEG_SIZE 1024

extern int make_itsp_frame(p_s_itsp_cnx cnx, p_action_sequence seq, p_itsp_action action, p_itsp_frame fr);

#endif /* INCLUDE_ITSP_FRAME_MAKER_H_ */
