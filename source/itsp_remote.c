/*
 * itsp_remote.c
 *
 *  Created on: 30 ao�t 2017
 *      Author: f.baccari
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <errno.h>

 #include <i_string.h>
 #include <itsp_cnx.h>
 #include <s3k_session.h>
 #include <itsp_remote.h>

void set_remote_status(p_s_itsp_remote remote, remote_status status){
	pthread_mutex_lock(remote->lock);
	remote->status = status;
	pthread_mutex_unlock(remote->lock);
}

volatile remote_status get_remote_status(p_s_itsp_remote remote){
	pthread_mutex_lock(remote->lock);
	remote_status value = remote->status;
	pthread_mutex_unlock(remote->lock);
	return value;
}


int cnx_io_frames(p_s_itsp_remote remote){

	char buffer[SOCK_BUFF_SIZE] = {0}, *write_buff = NULL;
	int read_size = 0, write_size = 0;
	int ret = 0, max_sd = 0, out = FAULT;;
	struct timeval t_out = {0};
	p_s_net_frame frame = NULL;
	int written = 0;
	fd_set readfds, writefds;
	p_s_itsp_cnx remote_cnx = (p_s_itsp_cnx)remote;


	if(cnx_get_phy_status(remote_cnx) < phy_cnx_on){
		DEBUG_TOFILE(remote->log_file, "cnx phy status error : %d", remote->phy_status);
		goto return_line;
	}

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(remote->socket, &readfds);
	if(fifo_count(remote->output_frames))
		FD_SET(remote->socket, &writefds);
	max_sd = remote->socket;

	t_out.tv_sec	= CNX_SELECT_TIMEOUT_S;
	t_out.tv_usec	= CNX_SELECT_TIMEOUT_US;

	ret = select( max_sd + 1, &readfds , &writefds , NULL , &t_out);

	if(ret  == -1){
		DEBUG_TOFILE(remote->log_file, "error %d on select : %s", errno, strerror(errno));
		cnx_set_phy_status(remote_cnx, phy_cnx_error);
		process_cnx_event(remote, event_error, frame);
		goto return_line;
	}


	out = OK;

	if(!ret) {
		if(cnx_check_timeout(remote_cnx))
			process_cnx_event(remote, event_timeout, NULL);
		goto return_line;
	}

	if (FD_ISSET(remote->socket, &readfds)){

		cnx_reset_timeout(remote_cnx);

		if(((read_size = read(remote->socket , buffer , SOCK_BUFF_SIZE)) < 0) || (errno && errno != 138)){
			DEBUG_TOFILE(remote->log_file, "Error %d on read : %s", errno, strerror(errno));
			cnx_set_phy_status(remote_cnx, phy_cnx_error);
			out = FAULT;
			process_cnx_event(remote, event_error, NULL);
			goto return_line;
		}

		if(!read_size){
			TRACE(remote->log_file, "connection on socket %d went off\n", remote->socket);
			cnx_set_phy_status(remote_cnx, phy_cnx_off);
			out = FAULT;
			process_cnx_event(remote, event_disconnect, NULL);
			goto return_line;
		}

		process_received_data(remote_cnx, buffer, read_size);

		//TRACE(remote->log_file, "%ld bites of data received on socket %d : %s\n", (long int)read_size, remote->socket, buffer);

	}

	if (FD_ISSET(remote->socket, &writefds) && (frame = fifo_pull(remote->output_frames))){

		cnx_reset_timeout(remote_cnx);

		write_buff = frame->data;
		write_size = frame->size;

		while(((written = write(remote->socket , write_buff , write_size)) > 0) && (write_size > written)){
			write_buff += written;
			write_size -= written;
		}

		if(written == -1){
			DEBUG_TOFILE(remote->log_file, "Error %d (ret = %d) when writing on socket %d. : %s\n", errno, written, remote->socket, strerror(errno));
			cnx_set_phy_status(remote_cnx, phy_cnx_error);
			out = FAULT;
			process_cnx_event(remote, event_error, frame);
			goto return_line;
		}

		if(!written){
			TRACE(remote->log_file, "connection on socket %d went off\n", remote->socket);
			cnx_set_phy_status(remote_cnx, phy_cnx_off);
			out = FAULT;
			process_cnx_event(remote, event_error, frame);
			goto return_line;
		}

		process_cnx_event(remote, event_sent, frame);

		//TRACE(remote->log_file, "%ld bites of data sent on socket %d\n", (long int)write_size, remote->socket);
	}

	return_line : return out;
}

p_s_itsp_remote itsp_remote_init(
		char*				col_att,
		char*				grep,
		itsp_cnx_type		itsp_type,
		char*				log_file
		)
{

	p_s_itsp_remote remote = malloc(sizeof(s_itsp_remote));
	memset(remote, 0, sizeof(s_itsp_remote));
	cnx_init_((void*)remote);

	remote->log_file = stdout;
	/*if(!(remote->log_file = fopen(log_file, "w+"))){
		free(remote);
		DEBUG_TOFILE(remote->log_file,"cannot open file \"%s\"", log_file);
		return NULL;
	}*/

	if(!fast_map_at(liste_attributaires, col_att)){
		TRACE(remote->log_file,"att \"%s\" not found. making a new one..\n", col_att);
		s_s3k_attrib att = {0};
		memcpy(att.code_att,col_att,3);
		att.id_att = att_ctr++;
		strcpy(att.itsp_version.document, "ITSP");
		att.itsp_version.version.version_number = 5;
		att.itsp_version.version.revision_number = 18;
		att.att_description = NULL;
		att.jet_lag = 0;
		fast_map_insert(liste_attributaires, col_att, &att);
	}

	strcpy(remote->col_att, col_att);

	p_s_s3k_grep p_grep = NULL;
	p_s_s3k_att_conf p_col_conf = NULL;
	if(!(p_grep = fast_map_at(liste_reunions, grep)) || !(p_col_conf = fast_map_at(p_grep->col_configs, col_att)))
		TRACE(remote->log_file, "no grep \"%s\" found with collector \"%s\"\n", grep, col_att);

	strcpy(remote->grep, grep);


	/*while(!p_col_conf || !p_col_conf->host_adr_ip){
		char buff[256] = {0};
		TRACE(stdout, "itsp_remote_init : no host ip address found in \"%s\" col_conf on grep \"%s\".\nplease type in a valid ip address :", remote->col_att, remote->grep);
		scanf("%s",buff);
		if(inet_pton(AF_INET, buff, &remote->address.sin_addr)){
			p_col_conf->host_adr_ip = malloc(strlen(buff) + 1);
			strcpy(p_col_conf->host_adr_ip, buff);
		}
		p_col_conf->host_adr_ip = malloc(strlen("127.0.0.1") + 1);
		strcpy(p_col_conf->host_adr_ip, "127.0.0.1");
	}*/

	int port = 4000;
	/*while(!p_grep || !(port = p_grep->port)){
		TRACE(stdout, "itsp_remote_init : no port number found for grep \"%s\".\nplease type in an valid port number :", remote->grep);
		scanf("%hu", &p_grep->port);
		//p_grep->port = 4000;
	}*/

	remote->address.sin_family = AF_INET;
	remote->address.sin_port = htons(port);


	remote->cnx_type = itsp_type;

	remote->itsp_mode = mode_remote;

	remote->status = remote_st_init;

	TRACE(remote->log_file, "Remote init for att \"%s\" on grep \"%s\"\n", col_att, grep);

	return remote;
}


void* remote_monitoring_task(p_s_itsp_remote remote){

	time_t t = 0;
	struct tm tm = {0};

	set_remote_status(remote, remote_st_init);

	cnx_phy_connect((void*)remote);

	process_cnx_event(remote, event_connect, NULL);

	cnx_set_phy_status((void*)remote, phy_cnx_monitored);
	cnx_set_logi_status((void*)remote, itsp_cnx_physic);

	t = time(NULL);
	tm = *localtime(&t);
	TRACE(remote->log_file, "Remote monitor started at %02d:%02d:%02d (%02d/%02d/%02d) -> socket nb %d\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
			remote->socket);


	set_remote_status(remote, remote_st_running);


	while(cnx_io_frames(remote) && (get_remote_status(remote) == remote_st_running))
		continue;

	process_cnx_event(remote, event_disconnect, NULL);
	close(remote->socket);
	set_remote_status(remote, remote_st_stoped);

	t = time(NULL);
	tm = *localtime(&t);
	TRACE(remote->log_file, "Remote monitor on socket %d stopped at %02d:%02d:%02d (%02d/%02d/%02d)\n",
			remote->socket,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

	return (void*)OK;
}

int start_remote_client(p_s_itsp_remote remote){
	remote_status st = 0;
	if(((st = get_remote_status(remote)) == remote_st_error) || (st == remote_st_running)){
		DEBUG_TOFILE(remote->log_file, "status error\n");
		return FAULT;
	}

	remote->monitoring_thread = create_thread(remote_monitoring_task, remote);

	run_thread(remote->monitoring_thread);

	return OK;
}

int stop_remote_client(p_s_itsp_remote remote){
	if(get_remote_status(remote) != remote_st_running)
		return FAULT;

	set_remote_status(remote, remote_st_stoped);

	if(!join_thread(remote->monitoring_thread))
		DEBUG_TOFILE(remote->log_file, "main_thread join error\n");

	return OK;
}

