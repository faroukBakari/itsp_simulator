/*
 * create_session.h
 *
 *  Created on: 31 mai 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_S3K_SESSION_H_
#define INCLUDE_S3K_SESSION_H_

#include <i_fast_map.h>
#include <i_sorted_vect.h>
#include <itsp_structs.h>

extern p_s_fast_map liste_attributaires;
extern p_s_fast_map liste_connexions;
extern p_s_fast_map liste_reunions;
extern p_s_fast_map liste_paris;

extern int att_ctr;
extern int pool_typ_ctr;
extern int grep_ctr;

extern p_itsp_sequence* load_session_from_log(char* log_file);

#endif /* INCLUDE_S3K_SESSION_H_ */
