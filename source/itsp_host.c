/*
 * itsp_host.c
 *
 *  Created on: 23 ao�t 2017
 *      Author: f.baccari
 */


 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>

 #include <i_string.h>
 #include <s3k_session.h>
 #include <itsp_host.h>

long s_itsp_cnx_cmp_func(void* cnx1, void*cnx2){
	return memcmp(cnx1, cnx2, sizeof(s_itsp_cnx));
}

void set_host_status(p_s_itsp_host srv, host_status status){
	pthread_mutex_lock(srv->lock);
	srv->status = status;
	pthread_mutex_unlock(srv->lock);
}

host_status get_host_status(p_s_itsp_host srv){
	pthread_mutex_lock(srv->lock);
	host_status value = srv->status;
	pthread_mutex_unlock(srv->lock);
	return value;
}

p_s_itsp_host itsp_host_init(
		char*				grep,
		itsp_cnx_type		itsp_type,
		char*				log_file
		)
{
	p_s_itsp_host server = malloc(sizeof(s_itsp_host));

	memset(server, 0, sizeof(s_itsp_host));

	server->log_file = stdout;
	/*if(!(server->log_file = fopen(log_file, "w+"))){
		free(server);
		DEBUG_TOFILE(server->log_file,"cannot open file \"%s\"", log_file);
		return NULL;
	}*/

	p_s_s3k_grep p_grep = NULL;
	if(!(p_grep = fast_map_at(liste_reunions, grep))){
		free(server);
		DEBUG_TOFILE(server->log_file,"grep \"%s\" not found\n", grep);
		return NULL;
	}

	strcpy(server->grep, grep);
	strcpy(server->org_att, p_grep->org_att);

	while(!p_grep->port){
		/*TRACE(stdout, "itsp_host_init : no port number found for grep \"%s\".\nplease type in an valid port number :", server->grep);
		scanf("%hu", &p_grep->port);*/
		p_grep->port = 4000;
	}

	server->address.sin_family = AF_INET;
	server->address.sin_port = htons(p_grep->port);
	server->address.sin_addr.s_addr = INADDR_ANY;


	server->itsp_type = itsp_type;

	server->monitoring_thread = NULL;

	server->itsp_cnxs = fast_map_init(4, sizeof(s_itsp_cnx), string_key_com);

	server->status =  host_st_init;

	server->lock = mutex_init();

	TRACE(server->log_file, "Host init with org \"%s\" on grep \"%s\"\n", p_grep->org_att, p_grep->name);

	return server;

}

int main_task(p_s_itsp_host host_srv){

	p_s_itsp_cnx cnx = NULL;
	p_s_net_frame fr = NULL;
	int odd, ret;
	int max_sd;
	time_t t = 0;
	struct tm tm = {0};
	struct timeval t_out = {0};
	char* write_buff = NULL, buffer[SOCK_BUFF_SIZE] = {0};
	int write_size = 0, read_size = 0, written = 0;
	struct{
		char ctr;
		char null_1;
		char null_2;
		char null_3;
	}new_cnx = {0};

	fd_set readfds, writefds;

	if(get_host_status(host_srv) != host_st_init){
		DEBUG_TOFILE(host_srv->log_file, "host status error\n");
		return FAULT;
	}

	if(!(host_srv->socket = socket(AF_INET , SOCK_STREAM , 0))) {
		DEBUG_TOFILE(host_srv->log_file,"Socket error %d : %s\n", errno, strerror(errno));
		return FAULT;
	}

	int yes = 1;
	if (setsockopt(host_srv->socket, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0) {
		DEBUG_TOFILE(host_srv->log_file,"Setsockopt error %d : %s\n", errno, strerror(errno));
		return FAULT;
	}

	if (bind(host_srv->socket, (struct sockaddr *)&host_srv->address, sizeof(host_srv->address))<0){
		DEBUG_TOFILE(host_srv->log_file, "bind error %d : %s\n", errno, strerror(errno));
		set_host_status(host_srv, host_st_error);
		return FAULT;
	}

	if (listen(host_srv->socket, 10) < 0){
		DEBUG_TOFILE(host_srv->log_file, "Listen error %d : %s\n", errno, strerror(errno));
		set_host_status(host_srv, host_st_error);
		return FAULT;
	}

	set_host_status(host_srv, host_st_running);

	t = time(NULL);
	tm = *localtime(&t);
	TRACE(host_srv->log_file, "Host server started at %02d:%02d:%02d (%02d/%02d/%02d) -> socket nb %d\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
			host_srv->socket);

	host_status st = 0;
	while(((st = get_host_status(host_srv)) != host_st_error) && (st != host_st_stoped)){

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(host_srv->socket, &readfds);
		FD_SET(host_srv->socket, &writefds);
		max_sd = host_srv->socket;

		fast_map_data_iterate(cnx, host_srv->itsp_cnxs){
			if(cnx_get_phy_status(cnx) == phy_cnx_server){
				FD_SET(cnx->socket, &readfds);
				if(fifo_count(cnx->output_frames))
					FD_SET(cnx->socket, &writefds);
				if(max_sd < cnx->socket)
					max_sd = cnx->socket;
			}

		}

		t_out.tv_sec	= SERVER_SELECT_TIMEOUT_S;
		t_out.tv_usec	= SERVER_SELECT_TIMEOUT_US;

		ret = select( max_sd +1 , &readfds , &writefds , NULL , &t_out);

		//TRACE(server->log_file, "host select attempt (ret = %d)\n", ret);

		if(!ret) {
			fast_map_data_iterate(cnx, host_srv->itsp_cnxs)
				if(cnx_check_timeout(cnx))
					process_cnx_event(cnx, event_timeout, NULL);
			continue;
		}


		if(ret  == -1){
			DEBUG_TOFILE(host_srv->log_file, "error %d on select : %s\n=> Server stopped", errno, strerror(errno));
			set_host_status(host_srv, host_st_error);
			return FAULT;
		}

		if (ret && FD_ISSET(host_srv->socket, &readfds)){
			//TRACE(host_srv->log_file, "Incoming connection on host server...\n");
			if(get_host_status(host_srv) == host_st_paused){
				odd = accept(host_srv->socket, (struct sockaddr *)&host_srv->address, &size_addr);
				close(odd);
				TRACE(host_srv->log_file, "host server -> connection attempt rejected\n");
			}
			else{
				if((cnx = cnx_phy_accept(host_srv->log_file, host_srv->socket))){
					getpeername(cnx->socket , (struct sockaddr*)&cnx->address , &size_addr);
					/*TRACE(host_srv->log_file,"New physical connection from %s (socket %d) NAT %d -> %d\n",
									inet_ntoa(cnx->address.sin_addr),
									cnx->socket,
									ntohs(host_srv->address.sin_port),
									ntohs(cnx->address.sin_port));*/

					if(new_cnx.ctr == 127)
						new_cnx.ctr = 0;

					if(fast_map_at(host_srv->itsp_cnxs, &new_cnx)){
						DEBUG_TOFILE(host_srv->log_file, "Max new cnx reached!");
						abort();
					}
					cnx->itsp_mode = mode_host;
					cnx->logi_status = itsp_cnx_physic;
					cnx->cnx_type = itsp_chrono_pool;
					strcpy(cnx->grep, host_srv->grep);
					void* tmp = cnx;
					cnx = fast_map_insert(host_srv->itsp_cnxs, &new_cnx, cnx);
					free(tmp);
					process_cnx_event(cnx, event_connect, NULL);
					new_cnx.ctr++;
				}
				cnx = NULL;
			}
		}


		fast_map_data_iterate(cnx, host_srv->itsp_cnxs){


			if (FD_ISSET(cnx->socket, &readfds)){

				cnx_reset_timeout(cnx);

				if(((read_size = read(cnx->socket , buffer , SOCK_BUFF_SIZE)) < 0) || errno){
					DEBUG_TOFILE(host_srv->log_file, "Error %d on read : %s", errno, strerror(errno));
					cnx_set_phy_status(cnx, phy_cnx_error);
					process_cnx_event(cnx, event_error, NULL);
					continue;
				}

				if(!read_size){
					TRACE(host_srv->log_file, "connection on socket %d whent off\n", cnx->socket);
					cnx_set_phy_status(cnx, phy_cnx_off);
					process_cnx_event(cnx, event_disconnect, NULL);
					continue;
				}

				process_received_data(cnx, buffer, read_size);

				//TRACE(host_srv->log_file, "%ld bites of data receved on socket %d : %s\n", (long int)read_size, cnx->socket, buffer);


			}

			if (FD_ISSET(cnx->socket, &writefds) && (fr = fifo_pull(cnx->output_frames))){

				cnx_reset_timeout(cnx);

				write_buff = fr->data;
				write_size = fr->size;

				while(((written = write(cnx->socket , write_buff , write_size)) > 0) && (write_size > written)){
					write_buff += written;
					write_size -= written;
				}

				if(written == -1){
					DEBUG_TOFILE(host_srv->log_file, "Error %d (ret = %d) when writing on socket %d. : %s\n", errno, written, cnx->socket, strerror(errno));
					cnx_set_phy_status(cnx, phy_cnx_error);
					process_cnx_event(cnx, event_error, NULL);
					continue;
				}

				if(!written){
					TRACE(host_srv->log_file, "connection on socket %d whent off\n", cnx->socket);
					cnx_set_phy_status(cnx, phy_cnx_off);
					process_cnx_event(cnx, event_disconnect, NULL);
					continue;
				}

				process_cnx_event(cnx, event_sent, fr);

				//TRACE(host_srv->log_file, "%ld bites of data sent on socket %d : %s\n", (long int)write_size, cnx->socket, write_buff);

			}

			if(cnx_check_timeout(cnx))
				process_cnx_event(cnx, event_timeout, NULL);
		}

	}

	t = time(NULL);
	tm = *localtime(&t);

	fast_map_data_iterate(cnx, host_srv->itsp_cnxs)
		if(cnx_get_phy_status(cnx) > phy_cnx_off){
			close(cnx->socket);
			cnx_set_phy_status(cnx, phy_cnx_off);
			process_cnx_event(cnx, event_disconnect, NULL);
		}

	close(host_srv->socket);

	TRACE(host_srv->log_file, "Host server stopped at %02d:%02d:%02d (%02d/%02d/%02d)\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

	return OK;
}

int start_host_server(p_s_itsp_host server){

	if(get_host_status(server) != host_st_init){
		DEBUG_TOFILE(server->log_file,"status error\n");
		return FAULT;
	}

	server->monitoring_thread = create_thread(main_task, server);

	run_thread(server->monitoring_thread);

	return OK;
}

int stop_host_server(p_s_itsp_host server){
	if(get_host_status(server) != host_st_running)
		return FAULT;

	set_host_status(server, host_st_stoped);
	if(!join_thread(server->monitoring_thread))
		DEBUG_TOFILE(server->log_file,"main_thread join error\n");


	return OK;
}

