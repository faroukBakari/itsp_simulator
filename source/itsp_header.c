/*
 * itsp_header.c
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>


char* header_write_template = 	"Header	: %s\n"
								"	frame	: %c%c	|"
									" src.  : %s		|"
									" seq.  : %d \n"
								"	event	: %s	|"
									" race  : %02d		|"
									" pool  : %s\n"
								"	reason	: %c		|"
									" time	: %02d:%02d:%02d	|"
									" lenth : %d\n";

char* header_read_template  = 	"([PRDA]"						// 1 - message type
								"[CPXWT$SRALF]"					// 2 - data type
								".{3}"							// 3 - source
								"[0-9]{2}"						// 4 - sequence
								".{3}"							// 5 - event code
								"[0-9,\\.]{2}"					// 6 - race number
								".{3}"							// 7 - pool code
								"[ befhrHDRPCSIATX\\?]"			// 8 - reason
								"[0-9]{2}[0-9]{2}[0-9]{2}"		// 9 - time
								"[0-9]{4})"						// 10 - lenth
								"[0-9]{5}";						// 11 - check sum

char* header_write_frame(char* buff, p_itsp_header hr){

	*buff++ = hr->frame_type->message;
	*buff++ = hr->frame_type->data;
	buff = write_fixed(buff, hr->source, 3);
	buff += sprintf(buff, "%02d", hr->sequence);
	buff = write_fixed(buff, hr->event_code, 3);
	if(hr->race_number)
		buff += sprintf(buff, "%02d", hr->race_number);
	else
		buff += sprintf(buff, "..");
	buff = write_fixed(buff, hr->pool_code, 3);
	*buff++ = hr->frame_type->reason;
	buff = write_time(buff, &hr->time);
	buff += sprintf(buff, "%04d", hr->ldata);
	return buff;

}

char* header_read_from(char* log, p_itsp_header hr){

	static struct {
		char				message_type ;
		char				data_type ;
		char				source[3] ;
		char				sequence[2] ;
		char				event_code[3] ;
		char				race_number[2] ;
		char				pool_code [3] ;
		char            	reason ;
		struct {
			char hour	[2] ;
			char minutes[2] ;
			char seconds[2] ;
		}					time ;
		char            	ldata[4] ;
	}*header_segmenter;


	/*const sub_str *sb = NULL;
	  if(!(sb = match_expr(log, header_read_template,0))){
		*hr = NULL;
		return NULL;
	}
	log = sb[0].start;*/

	char tmp[6] = "";

	hr->header_log = sub2str(SUB_STRING(log, log + HEADER_SIZE));

	header_segmenter = (void*)log;

	hr->frame_type->message	= header_segmenter->message_type;
	hr->frame_type->data	= header_segmenter->data_type;
	hr->frame_type->reason 	= header_segmenter->reason;

	memcpy(tmp,header_segmenter->sequence,2); 					hr->sequence 		= atoi(tmp);
	memcpy(tmp,header_segmenter->race_number,2);				hr->race_number 	= atoi(tmp);
	memcpy(tmp,header_segmenter->time.hour,2); 					hr->time.hour 		= atoi(tmp);
	memcpy(tmp,header_segmenter->time.minutes,2); 				hr->time.minutes 	= atoi(tmp);
	memcpy(tmp,header_segmenter->time.seconds,2); 				hr->time.seconds 	= atoi(tmp);

	memcpy(hr->source,header_segmenter->source,3); 				hr->source[3] 		= 0;
	memcpy(hr->event_code,header_segmenter->event_code,3); 		hr->event_code[3] 	= 0;
	memcpy(hr->pool_code,header_segmenter->pool_code,3); 		hr->pool_code[3] 	= 0;

	memcpy(tmp,header_segmenter->ldata,4);						hr->ldata 			= atoi(tmp);

	return log + HEADER_SIZE;
}

int header_log_to(FILE* stream, p_itsp_header header){
	fprintf(stream,header_write_template,
			header->header_log ? header->header_log : "",
			header->frame_type->message,
			header->frame_type->data,
			header->source,
			header->sequence,
			header->event_code,
			header->race_number,
			header->pool_code,
			header->frame_type->reason,
			header->time.hour, header->time.minutes, header->time.seconds,
			header->ldata);
	return OK;
}
