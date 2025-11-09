/*
 * itsp_pilot.h
 *
 *  Created on: 26 oct. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_PILOT_H_
#define INCLUDE_ITSP_PILOT_H_

extern int 		itsp_init(p_s_itsp_cnx cnx);

extern int		itsp_collect_pools(p_s_itsp_cnx cnx, int race, char* pool, int final_flag);

extern int		itsp_totalize_pool(char* grep, int race, char* pool);

extern int		itsp_send_payoffs(p_s_itsp_cnx cnx, int race, char* pool, int result_flag);

#endif /* INCLUDE_ITSP_PILOT_H_ */
