/*
 * s3k_commun.h
 *
 *  Created on: 9 oct. 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_S3K_COMMUN_H_
#define INCLUDE_S3K_COMMUN_H_

extern int** itsp_make_live_runner(p_s_s3k_pool_type p_pool_typ, p_s_s3k_pool p_pool);
extern int** itsp_make_favorite_runners(p_s_s3k_grep p_grep, p_s_s3k_pool_type p_pool_typ, p_s_s3k_pool p_pool, int leg);
extern int** itsp_make_scan_runners(p_s_s3k_grep p_grep, p_s_s3k_pool_type p_pool_typ, p_s_s3k_pool p_pool, int leg);

#endif /* INCLUDE_S3K_COMMUN_H_ */
