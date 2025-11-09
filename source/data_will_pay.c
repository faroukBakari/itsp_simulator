/*
 * data_will_pay.c
 *
 *  Created on: 15 mars 2017
 *      Author: f.baccari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>


char* data_will_pay_read_template 	=	// 1 - data_will_pay
										"([0-9]{2})"						// 1 - rows
										"([0-9]{2})"						// 2 - columns
										"(([0-9\\,\\-]*/)+"					// 3 - runner list
										"([0-9]{2}"							// 4 - winner
										"[\\+\\-][0-9,P-Y]*[@-I,`-i]"		// 5 - price
										"[0-9,P-Y]*[@-I,`-i])*)*"			// 6 - winning
										"[0-9]{5}";

char* data_will_pay_read_row_template 	=	// 1 - data_will_pay
										"(([0-9\\,\\-]*/)+)"				// 3 - runner list
										"(([0-9]{2}"						// 4 - winner
										"[\\+\\-][0-9,P-Y]*[@-I,`-i]"		// 5 - price
										"[0-9,P-Y]*[@-I,`-i])*)";			// 6 - winning

char* data_will_pay_write_frame(char* buff, p_itsp_str_will_pay ptr){

	buff = write_digits(buff, ptr->rows, 2, 0);
	buff = write_digits(buff, ptr->columns, 2, 0);
	if(ptr->rows){
		int i = 0;
		while(i < ptr->rows){

			int j = 0;
			while(ptr->will_pay_row[i].runner_list[j])
				buff = write_numeric_range(buff, ptr->will_pay_row[i].runner_list[j++]);

			j = 0;
			while(j < ptr->columns){
				buff = write_digits(buff, ptr->will_pay_row[i].items[j].winner, 2, 0);
				buff = write_amount(buff, ptr->will_pay_row[i].items[j].price, 1, 0, 4);
				buff = write_amount(buff, ptr->will_pay_row[i].items[j].winning, 0, 0, 4);
				j++;
			}
			i++;
		}
	}
	return buff;

}

int data_will_pay_read_frame(char* log, p_itsp_data_struct* data){

	p_itsp_str_will_pay ptr = (*data)->structure = malloc(sizeof(itsp_str_will_pay));
	log = read_digits(log,&ptr->rows, 2);
	log = read_digits(log,&ptr->columns, 2);
	if(ptr->rows){
		ptr->will_pay_row = malloc(ptr->rows * sizeof(itsp_will_pay_row));
		int i = 0;
		while(i < ptr->rows){
			int* buff[256] = {NULL}, j = 0;
			char* ptr_tmp = log;
			while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[j++])))
				log = ptr_tmp;

			ptr->will_pay_row[i].runner_list = malloc((j + 1) * sizeof(int*));
			ptr->will_pay_row[i].runner_list[j] = NULL;
			memcpy(ptr->will_pay_row[i].runner_list, buff, j * sizeof(int*));

			ptr->will_pay_row[i].items = malloc(ptr->columns * sizeof(itsp_will_pay_item));

			j = 0;
			while(j < ptr->columns){
				log = read_digits(log,&ptr->will_pay_row[i].items[j].winner, 2);
				log = read_amount(log,&ptr->will_pay_row[i].items[j].price);
				log = read_amount(log,&ptr->will_pay_row[i].items[j].winning);
				j++;
			}
			i++;
		}
	}
	return OK;
}

char* data_will_pay_read_from(char* log, p_itsp_data_struct* data){
	const sub_str* segments = NULL;

	segments = match_expr(log, data_will_pay_read_template, 0);
	if(segments){
		(*data)->data_log = sub2str(segments[0]);
		p_itsp_str_will_pay ptr = (*data)->structure = malloc(sizeof(itsp_str_will_pay));
		read_digits(segments[1].start,&ptr->rows, 2);
		read_digits(segments[2].start,&ptr->columns, 2);
		if(ptr->rows){
			ptr->will_pay_row = malloc(ptr->rows * sizeof(itsp_will_pay_row));
			int i = 0;
			char* s = segments[3].start;
			while(i < ptr->rows){
				segments = match_expr(s, data_will_pay_read_row_template, 0);
				if(segments){
					int* buff[256] = {NULL}, j = 0;
					char* tmp = sub2str(segments[1]), *ptr_tmp = tmp;
					while((ptr_tmp = read_numeric_range(ptr_tmp, &buff[j++])))
						continue;
					free(tmp);
					ptr->will_pay_row[i].runner_list = malloc((j + 1) * sizeof(int*));
					ptr->will_pay_row[i].runner_list[j] = NULL;
					memcpy(ptr->will_pay_row[i].runner_list, buff, j * sizeof(int*));

					ptr->will_pay_row[i].items = malloc(ptr->columns * sizeof(itsp_will_pay_item));

					j = 0;
					char* p = segments[3].start;
					while(j < ptr->columns){
						p = read_digits(p,&ptr->will_pay_row[i].items[j].winner, 2);
						p = read_amount(p,&ptr->will_pay_row[i].items[j].price);
						p = read_amount(p,&ptr->will_pay_row[i].items[j].winning);
						j++;
					}
				}
				else{
					fprintf(std_err, "Match error : will pay row nb : %d\n", ptr->rows - 1);
					free(ptr->will_pay_row);
					free(*data);
					*data = NULL;
					return log;
				}
				s = segments[0].end;
				i++;
			}
		}
		return segments[0].end;
	}
	else{
		fprintf(std_err, "Match error : will pay\n");
		(*data)->structure = NULL;
		return NULL;
	}
	return log;
}

int data_will_pay_log_to(FILE* stream, p_itsp_data_struct data){
	/**************************PREPARE****************************/
	p_itsp_str_will_pay ptr = data->structure;

	fprintf(stream ,"matrix dims : %d x %d\n",
							ptr->rows,
							ptr->columns);
	int i = 0;
	while(i < ptr->rows){
		char buff[256] = {0}, *c_ptr = buff;
		int j = 0;
		while(ptr->will_pay_row[i].runner_list[j])
			c_ptr = write_numeric_range(c_ptr, ptr->will_pay_row[i].runner_list[j++]);
		fprintf(stream ,"	runner list	:	%s\n",buff);
		fprintf(stream ,"		winner		price		winning\n");
		j = 0;
		while(j < ptr->columns){
			fprintf(stream ,"		%3d		%10.2f	%10.2f\n",
					ptr->will_pay_row[i].items[j].winner,
					ptr->will_pay_row[i].items[j].price,
					ptr->will_pay_row[i].items[j].winning);
			j++;
		}
		fprintf(stream ,"\n");
		i++;
	}
	return OK;
}

void data_will_pay_free_frame(p_itsp_frame frame){

	p_itsp_str_will_pay ptr = ( frame->data_str && frame->data_str->structure) ?
					frame->data_str->structure : NULL;

	if(ptr) {
		if(ptr->will_pay_row){
			int i = 0;
			while(i < ptr->rows){
				if(ptr->will_pay_row[i].runner_list) free_combo(ptr->will_pay_row[i].runner_list);
				if(ptr->will_pay_row[i].items) free(ptr->will_pay_row[i].items);
				i++;
			}
			free(ptr->will_pay_row);
		}
		free(ptr);
	}
}


























