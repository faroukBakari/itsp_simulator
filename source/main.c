/*
 * main.c
 *
 *  Created on: 29 mai 2017
 *      Author: f.baccari
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <i_string.h>
 #include <i_file.h>
 #include <s3k_session.h>
 #include <itsp_host.h>
 #include <itsp_remote.h>
 #include <itsp_scheduler.h>
 #include <sys/stat.h>

 #include <math.h>
 #include <itsp_common.h>
 #include <itsp_frame.h>
 #include <itsp_cnx.h>
 #include <itsp_frame_maker.h>
 #include <itsp_scheduler.h>
 #include <itsp_frame_analyser.h>
 #include <itsp_pilot.h>


void test_process_frame(char* fr){
	p_s_itsp_cnx cnx = cnx_init();
	strcpy(cnx->col_att, "OJ1");
	strcpy(cnx->grep, "NDT");
	cnx->log_file = stdout;
	cnx->cnx_type = itsp_chrono_pool;
	cnx->itsp_mode = mode_host;
	cnx->phy_status = phy_cnx_on;
	cnx->logi_status = itsp_cnx_sync;
	p_itsp_frame itsp_fr = malloc(sizeof(itsp_frame));
	size_t size = strlen(fr);
	p_s_net_frame frame = new_net_frame();
	frame->size = size;
	frame->data = malloc(size + 1);
	memcpy(frame->data, fr, size);
	frame->data[size] = 0;
	read_net_frame(frame, itsp_fr, cnx->log_file);
	p_action_sequence seq = make_sequence((p_s_itsp_cnx)cnx, itsp_fr->header->frame_type, way_in,
			itsp_fr->header->race_number, itsp_fr->header->pool_code, itsp_fr->header->sequence, NULL);
	p_itsp_action action = fifo_pull(seq->actions);
	action->message = itsp_fr->header->frame_type->message;
	action->type = action_receve;
	action->reason = itsp_fr->header->frame_type->reason;
	itsp_frame_analyze(cnx, itsp_fr, seq, action);
}

int main(){

	std_err = stderr;

	std_out = fopen("log.txt", "w+");
	load_session_from_log("../itsp_translator/log/ITSP_resitsp6201_hes_R1");
	exit(0);
	//load_session_from_log("SGR");

	/*test_process_frame(	"DSOJ152NDT01...b190451006902776"
						"C195900M....06WINCB..PLCCB..SHWCB..EX CA..DD C02A..P03C0203A....07203");
	exit(0);*/

	p_s_itsp_host host = itsp_host_init("NDT", itsp_chrono_pool, "log_moa");
	start_host_server(host);
	sleep(1);
	p_s_itsp_remote remote = itsp_remote_init("OJ1", "LOL", itsp_chrono_pool, "log_oj1");
	start_remote_client(remote);

	sleep(10);

	stop_remote_client(remote);
	stop_host_server(host);


	return 0;
}
