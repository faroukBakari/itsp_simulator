/*
 * itsp_host.h
 *
 *  Created on: 23 ao�t 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_HOST_H_
#define INCLUDE_ITSP_HOST_H_

#include <itsp_cnx.h>

#define MAX_SRV_CXN	1023
#define SERVER_SELECT_TIMEOUT_S 1
#define SERVER_SELECT_TIMEOUT_US 0

typedef enum host_status{
	host_st_error	= -1,
	host_st_init 	= 0,
	host_st_running = 1,
	host_st_paused 	= 2,
	host_st_stoped 	= 3
}host_status;

typedef struct s_itsp_host{
	pthread_mutex_t* 		lock;
	FILE*					log_file;
	char					org_att[4];
	char					grep[4];
	int 					socket;
	struct sockaddr_in		address;
	itsp_cnx_type			itsp_type;
	p_s_thread				monitoring_thread;
	p_s_fast_map			itsp_cnxs;			//char[4](att), s_itsp_cnx
	volatile host_status	status;
}s_itsp_host, *p_s_itsp_host;

extern void set_host_status(p_s_itsp_host srv, host_status status);

extern host_status get_host_status(p_s_itsp_host srv);

extern p_s_itsp_host itsp_host_init(
		char*				grep,
		itsp_cnx_type		itsp_type,
		char*				log_file
		);

extern int start_host_server(p_s_itsp_host server);

extern int stop_host_server(p_s_itsp_host server);

extern int process_host_cnxs(p_s_itsp_host server);


#endif /* INCLUDE_ITSP_HOST_H_ */
