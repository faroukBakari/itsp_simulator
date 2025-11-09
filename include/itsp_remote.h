/*
 * itsp_remote.h
 *
 *  Created on: 30 ao�t 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_REMOTE_H_
#define INCLUDE_ITSP_REMOTE_H_

#include <itsp_cnx.h>

typedef enum remote_status{
	remote_st_error		= -1,
	remote_st_init 		= 0,
	remote_st_running 	= 1,
	remote_st_stoped 	= 2
}remote_status;

typedef struct s_itsp_remote{
	//------------------------------
	CNX_ATTRIBUTES
	//------------------------------
	p_s_thread						monitoring_thread;
	volatile remote_status			status;
}s_itsp_remote, *p_s_itsp_remote;

extern void 					set_remote_status(p_s_itsp_remote remote, remote_status status);

extern volatile remote_status 	get_remote_status(p_s_itsp_remote remote);

extern p_s_itsp_remote 			itsp_remote_init(
												char*				col_att,
												char*				grep,
												itsp_cnx_type		itsp_type,
												char*				log_file
												);
extern int						start_remote_client(p_s_itsp_remote remote);

extern int						stop_remote_client(p_s_itsp_remote remote);

#endif /* INCLUDE_ITSP_REMOTE_H_ */
