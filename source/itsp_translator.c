/*
 * itsp_translator.c
 *
 *  Created on: 30 mai 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <i_file.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>
#include <itsp_header.h>
#include <itsp_data.h>
#include <itsp_frame.h>
#include <itsp_translator.h>

static long int nb_frames = 0;

char* load_log_file(char* file_name, FILE* res_load){

	FILE* file = NULL;
	if(!(file = open_file(file_name, "r+")))
		abort();

	char* str = load_file(file);

	if(!str){
		if(res_load)
			fprintf(res_load,"load_log_file : no data found.\n exit...\n");
		return NULL;
	}

	if(!str){
		if(res_load)
			fprintf(res_load,"load_log_file : no data found after cleaning.\n exit...\n");
		return NULL;
	}

	fclose(file);
	return str;
}

p_itsp_frame* load_frames(FILE* log, FILE* res_read){
	int frame_ctr = 0, max_nb_frames = DEF_LENTH;
	p_itsp_frame *frames = malloc(max_nb_frames * sizeof(p_itsp_frame));
	void* tmp = NULL;
	memset(frames, 0, max_nb_frames * sizeof(p_itsp_frame));

	if(res_read){
		fprintf(res_read, "loading frames : ");
		fflush(res_read);
	}

	while((read_log_frame(&frames[frame_ctr], log, res_read))){
		frames[frame_ctr]->frame_num = frame_ctr + 1;
		frame_ctr++;
		if(max_nb_frames < (frame_ctr + 1)){
			max_nb_frames *= 2;
			tmp = malloc(max_nb_frames * sizeof(p_itsp_frame));
			memset(tmp, 0, max_nb_frames * sizeof(p_itsp_frame));
			memcpy(tmp, frames, frame_ctr * sizeof(p_itsp_frame));
			free(frames);
			frames = tmp;
		}
	}

	tmp = malloc((frame_ctr + 1) * sizeof(p_itsp_frame));
	memcpy(tmp, frames, (frame_ctr + 1) * sizeof(p_itsp_frame));
	free(frames);
	frames = tmp;

	if(res_read){
		fprintf(res_read, "%d frames.\n", frame_ctr);
		fflush(res_read);
	}
	nb_frames = frame_ctr;

	return frames;
}

void print_log_data(FILE* stream, p_itsp_frame* itsp_data){
	p_itsp_frame frame = NULL;
	int ctr = 0;
	while((frame = itsp_data[ctr])){
		ctr++;
		log_frame(stream, frame);
	}
}

#define MAX_WORKING_SEQ 40
#define MAX_SEQ_FRAMES_GAP MAX_WORKING_SEQ - 1

int sort_seqz_func(const void* seq1, const void* seq2){
	return  (*(p_itsp_sequence*)seq1)->frames[0]->frame_num -
			(*(p_itsp_sequence*)seq2)->frames[0]->frame_num;
}

p_itsp_sequence* cluster_sequences(FILE* analyse_log, p_itsp_frame* itsp_data){
	int i = 0, j = 0, k = 0, f_ctr = 0, working_seq_ctr = 0, nb_seq = 0;
	p_itsp_frame fr = NULL, last_fr = NULL, tmp_fr = NULL;
	char host_name[4];


	while((fr = itsp_data[f_ctr++]))
		if((fr->header->frame_type->data == data_link) &&
			(fr->header->frame_type->reason == reason_host)){
			strcpy(host_name, fr->header->source);
			break;
		}

	// counting frames...
	while((fr = itsp_data[f_ctr++]))
		continue;

	if(analyse_log) fprintf(analyse_log,"host : %s\n",host_name);
	p_itsp_sequence* sqz = malloc(f_ctr * sizeof(p_itsp_sequence));
	p_itsp_sequence working_seq[MAX_WORKING_SEQ] = {0};


	f_ctr = 0;
	while((fr = itsp_data[f_ctr++])){

		DEBUG_TOFILE(analyse_log,"working frame nb %d", fr->frame_num);

		if(!strcmp(host_name, fr->header->source))
			fr->host_flag = 1;
		else
			fr->host_flag = 0;

		for(i = 0; i < MAX_WORKING_SEQ; i++){

			if(!working_seq[i])
				continue;

			if((working_seq[i]->seq_num == fr->header->sequence)&&
				!strcmp(working_seq[i]->event, fr->header->event_code)&&
			   (working_seq[i]->data_type == fr->header->frame_type->data)){

				if(analyse_log){
					DEBUG_TOFILE(analyse_log,"confirmed match for frames bellow :");
					log_frame(analyse_log, last_fr);
					log_frame(analyse_log, fr);
					fflush(std_out);
				}

				working_seq[i]->frames[working_seq[i]->nb_frames++] = fr;

				if(!working_seq[i]->to[0])
					strcpy(working_seq[i]->to, working_seq[i]->frames[1]->header->source);

				if(fr->header->frame_type->message == msg_acknowledge){
					DEBUG_TOFILE(analyse_log,"closing complete working seq nb %d", working_seq[i]->seq_num);

					working_seq[i]->frames[working_seq[i]->nb_frames] = NULL;
					switch ( working_seq[i]->frames[0]->header->frame_type->message){
					case msg_pending:
						k = 0;
						while((tmp_fr = working_seq[i]->frames[k++])&&
								(tmp_fr->header->frame_type->message != msg_data))
							continue;
						working_seq[i]->seq_type = (tmp_fr) ? pending_sequence : short_pending_sequence;
						break;
					case msg_request:
						k = 0;
						while((tmp_fr = working_seq[i]->frames[k++])&&
								(tmp_fr->header->frame_type->message != msg_data))
							continue;
						working_seq[i]->seq_type = (tmp_fr) ? request_sequence : short_request_sequence;
						break;
					case msg_data:
						working_seq[i]->seq_type = data_sequence;
						break;
					default:
						working_seq[i]->seq_type = error_sequence;
					}
					sqz[nb_seq++] = working_seq[i];
					working_seq[i] = NULL;
					working_seq_ctr--;
				}
				else
					if(working_seq[i]->nb_frames >= MAX_SEQ_FRAMES - 1){
						DEBUG_TOFILE(analyse_log,"MAX_SEQ_FRAMES reached : forcing close seq nb %d\n", working_seq[i]->seq_num);
						working_seq[i]->incomplete_flag = 1;
						working_seq[i]->seq_type = error_sequence;
						working_seq[i]->frames[working_seq[i]->nb_frames] = NULL;
						sqz[nb_seq++] = working_seq[i];
						working_seq[i] = NULL;
						working_seq_ctr--;
					}
				break;
			}
			else{
				if(fr->frame_num > (working_seq[i]->frames[working_seq[i]->nb_frames - 1]->frame_num + MAX_SEQ_FRAMES_GAP)){
					DEBUG_TOFILE(analyse_log,"closing incomplete working seq nb %d", working_seq[i]->seq_num);
					working_seq[i]->incomplete_flag = 1;
					working_seq[i]->seq_type = error_sequence;
					working_seq[i]->frames[working_seq[i]->nb_frames] = NULL;
					sqz[nb_seq++] = working_seq[i];
					working_seq[i] = NULL;
					working_seq_ctr--;
				}
			}
		}

		if(i == MAX_WORKING_SEQ){
			for(j = 0; j < MAX_WORKING_SEQ; j++){
				if(!working_seq[j])
					break;
			}
			if(j == MAX_WORKING_SEQ){
				fprintf(std_err, "max working sequences reached\n");
				abort();
			}

			DEBUG_TOFILE(analyse_log,"adding new working seq nb %d\n", fr->header->sequence);

			working_seq[j] = malloc(sizeof(itsp_sequence));
			memset(working_seq[j], 0, sizeof(itsp_sequence));
			working_seq[j]->seq_num = fr->header->sequence;
			working_seq[j]->nb_frames = 0;
			working_seq[j]->incomplete_flag = 0;
			working_seq[j]->frames[working_seq[j]->nb_frames++] = fr;
			working_seq[j]->data_type = fr->header->frame_type->data;
			strcpy(working_seq[j]->event, fr->header->event_code);
			strcpy(working_seq[j]->from, fr->header->source);
			working_seq[j]->to[0] = 0;
			working_seq_ctr++;
		}
	}
	for(i = 0; i < MAX_WORKING_SEQ; i++){
		if(working_seq[i]){
			DEBUG_TOFILE(analyse_log,"closing incomplete working seq nb %d\n", working_seq[i]->seq_num);
			working_seq[i]->incomplete_flag = 1;
			working_seq[i]->frames[working_seq[i]->nb_frames] = NULL;
			working_seq[i]->seq_type = error_sequence;
			sqz[nb_seq++] = working_seq[i];
		}
	}
	sqz[nb_seq] = NULL;

	p_itsp_sequence* tmp = malloc((nb_seq+1) * sizeof(p_itsp_sequence));
	memcpy(tmp, sqz, (nb_seq+1) * sizeof(p_itsp_sequence));

	free(sqz);
	sqz = tmp;
	qsort(sqz, nb_seq, sizeof(p_itsp_sequence), sort_seqz_func);

	return sqz;
}

void log_sequence(FILE* analyse_log, p_itsp_sequence seq){
	int i = 0;
	if(!seq) return;
	fprintf(analyse_log,"\n---------------------------------------------------------------\n");
	if((seq->seq_type == error_sequence)){
		fprintf(analyse_log,"!!WARNINIG : INCOMPLETE SEQUENCE!!\n");
	}
	fprintf(analyse_log,"sequence number %d : %s -> %s\n", 	seq->seq_num,
															seq->from,
															seq->to);
	fprintf(analyse_log,"data type		: %c\n"
						"sequence type	: %d\n"
						"event			: %s\n"
						"race  			: %02d\n"
						"pool  			: %s\n"
						"init reason		: %c\n",
						seq->data_type,
						seq->seq_type,
						seq->event,
						seq->frames[0]->header->race_number,
						seq->frames[0]->header->pool_code,
						seq->frames[0]->header->frame_type->reason);
	for (i=0; i<seq->nb_frames;i++){
		if(seq->frames[i]->header->frame_type->data != seq->data_type)
			fprintf(analyse_log,"\n!!WARNING!! : <sequence data type mismatch>");
		fprintf(analyse_log,"\n%s(%c%c-%c) at %02d:%02d:%02d (fr %d)\n	",
							seq->frames[i]->header->source,
							seq->frames[i]->header->frame_type->message,
							seq->frames[i]->header->frame_type->data,
							seq->frames[i]->header->frame_type->reason,
							seq->frames[i]->header->time.hour,
							seq->frames[i]->header->time.minutes,
							seq->frames[i]->header->time.seconds,
							seq->frames[i]->frame_num);
		if(seq->frames[i]->data_str)
			data_log_to(analyse_log, seq->frames[i]->data_str);
		if((seq->frames[i]->header->frame_type->message == msg_acknowledge)&&
				((seq->frames[i]->header->frame_type->reason) < 97 &&
				(seq->frames[i]->header->frame_type->reason != 32))){
			fprintf(analyse_log,"!!WARNING!! : <negative acknowledge : %c>\n",
					seq->frames[i]->header->frame_type->reason);
		}
	}

	if(seq->data_type == data_link){
		if(seq->frames[0]->header->frame_type->reason == reason_end)
			fprintf(analyse_log,"\n--->ITSP INITIATED\n");
		else
			if(seq->from && seq->to)
				fprintf(analyse_log,"\n--->LINK INIT\n");
	}
	if(seq->data_type == data_race_status){
		if(seq->frames[0]->header->frame_type->reason == reason_end)
			fprintf(analyse_log,"\n--->PARTI course %02d\n",seq->frames[0]->header->race_number);
		if(seq->frames[0]->header->frame_type->reason == reason_begin)
			fprintf(analyse_log,"\n--->NAME RACE sur la course %02d\n",
					seq->frames[0]->header->race_number);
	}
	if(seq->data_type == data_pools){
		if((seq->frames[0]->header->frame_type->reason == reason_final)&&
			(seq->frames[0]->header->frame_type->message == msg_pending)&&
			!strcmp(seq->frames[0]->header->pool_code,"***")){
			fprintf(analyse_log,"\n--->ENVOI PPF sur la course %02d\n",
					seq->frames[0]->header->race_number);
		}
	}
	if(seq->data_type == data_results){
		i = 0;
		while(seq->frames[i] && (seq->frames[i]->header->frame_type->message != msg_data))
			i++;
		if(seq->frames[i]){
			if(seq->frames[i]->header->frame_type->reason == reason_final)
				fprintf(analyse_log,"\n--->Arriv�e officielle course %02d\n",seq->frames[0]->header->race_number);
			else
				fprintf(analyse_log,"\n--->Arriv�e provisoire course %02d\n",seq->frames[0]->header->race_number);
		}
	}

}

void log_sequences(FILE* analyse_log, p_itsp_sequence* sqz){
	p_itsp_sequence seq = NULL;
	while((seq = *sqz++)){
		log_sequence(analyse_log, seq);
	}
}
