/*
 * itsp_cnx.h
 *
 *  Created on: 30 mai 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_ITSP_CNX_H_
#define INCLUDE_ITSP_CNX_H_

 #include <sys/socket.h>
 #include <arpa/inet.h> //inet_addr
 #include <i_fifo.h>
 #include <i_thread.h>
 #include <s3k_structs.h>
#include <sys/time.h>

#define SOCK_BUFF_SIZE 4095

#define CNX_SELECT_TIMEOUT_S  2
#define CNX_SELECT_TIMEOUT_US 0

#define CNX_MAX_TIMEOUT_S 15

#define STX 2
#define ETX 3

extern socklen_t size_addr;

typedef struct s_net_frame{
	int		frame_id;
	time_t	date_time;
	size_t	size;
	char*	data;
}s_net_frame, *p_s_net_frame;

typedef enum phy_cnx_status{
	phy_cnx_error		= -1,
	phy_cnx_off			= 0,
	phy_cnx_attempt		= 1,
	phy_cnx_on 			= 2,
	phy_cnx_monitored	= 3,
	phy_cnx_server		= 4
}phy_cnx_status_type;


typedef enum itsp_mode_type{
	mode_host	= 'h',
	mode_remote	= 'r'
}itsp_mode_type;

typedef enum itsp_cnx_status_type{
	itsp_cnx_off		= 0,
	itsp_cnx_physic		= 1,
	itsp_cnx_config		= 2,
	itsp_cnx_sync		= 3,
	itsp_cnx_logic		= 4
}itsp_cnx_status_type;

typedef enum itsp_cnx_type{
	itsp_chrono			= 0,
	itsp_pool			= 1,
	itsp_chrono_pool	= 2,
}itsp_cnx_type;

typedef enum itsp_action_type{
	action_receve		= 0,
	action_send			= 1,
	action_connect		= 2,
	action_disconnect	= 3
}itsp_action_type;

typedef enum itsp_sequence_way{
	way_in		= 0,
	way_out		= 1
}itsp_sequence_way;

typedef struct itsp_action{
	itsp_msg_type		message;
	itsp_msg_reason		reason;
	itsp_action_type	type;
}itsp_action, *p_itsp_action;

typedef struct{
	itsp_data_type		data_type;
	int					number;
	int					race;
	char				pool[4];
	p_s_fifo			actions;			// p_itsp_action
	int					interruptible_flag;
	int					host_only_flag;
	void*				special_data;
}action_sequence, *p_action_sequence;

typedef enum event_type{
	event_no_event		= 0,
	event_error			= 1,
	event_disconnect	= 2,
	event_connect		= 3,
	event_receve		= 4,
	event_sent			= 5,
	event_timeout		= 6
}event_type;

#define CNX_ATTRIBUTES \
		int								nu_cnx;\
		pthread_mutex_t*				lock;\
		FILE*							log_file;\
		char							col_att[4];\
		char							grep[4];\
		itsp_cnx_type					cnx_type;\
		itsp_mode_type					itsp_mode;\
		int 							socket;\
		struct sockaddr_in 				address;\
		volatile phy_cnx_status_type	phy_status;\
		volatile itsp_cnx_status_type	logi_status;\
		int								current_race;\
		time_t							timeout_check;\
		/*s_net_frame*/\
		p_s_fifo						output_frames;\
		int								current_seq;\
		/*p_action_sequence*/\
		p_s_fifo						sequences_pipe;\
		/*p_action_sequence*/\
		p_s_fast_map 					working_sequences;

typedef struct s_itsp_cnx{
	CNX_ATTRIBUTES
}s_itsp_cnx, *p_s_itsp_cnx;

///////////////////////////////////////////////////////////////////////////////////

extern p_s_net_frame		new_net_frame();

extern void					cnx_set_phy_status(p_s_itsp_cnx cnx, phy_cnx_status_type status);

extern phy_cnx_status_type	cnx_get_phy_status(p_s_itsp_cnx cnx);

extern void					cnx_set_logi_status(p_s_itsp_cnx cnx, itsp_cnx_status_type status);

extern itsp_cnx_status_type	cnx_get_logi_status(p_s_itsp_cnx cnx);

extern void 				cnx_reset_timeout(p_s_itsp_cnx cnx);

extern p_s_itsp_cnx 		cnx_init();

extern void			 		cnx_init_(p_s_itsp_cnx cnx);

extern int 					cnx_phy_connect(p_s_itsp_cnx cnx);

extern p_s_itsp_cnx 		cnx_phy_accept(FILE* stream, int socket_);

extern int					process_cnx_event(void* cnx, event_type evt, p_s_net_frame frame);

extern int					process_received_data(p_s_itsp_cnx cnx, char* buff, int size);

extern						p_s_net_frame make_net_frame(char* buff, int size);

#define 					cnx_set_frame(cnx, fr) fifo_push(cnx->output_frames, fr);

#define cnx_reset_timeout(cnx)({\
		pthread_mutex_lock(cnx->lock);\
		cnx->timeout_check = time(NULL);\
		pthread_mutex_unlock(cnx->lock);\
	})

#define cnx_check_timeout(cnx) (cnx->timeout_check + CNX_MAX_TIMEOUT_S <= time(NULL))

#endif /* INCLUDE_ITSP_CNX_H_ */
