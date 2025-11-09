/*
 * itsp_common.c
 *
 *  Created on: 9 mars 2017
 *      Author: f.baccari
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <i_tools.h>
#include <i_string.h>
#include <itsp_structs.h>
#include <itsp_common.h>


long c_time(p_itsp_str_time time){
	struct tm t = {0};
	t.tm_hour = time->hour;
	t.tm_min = time->minutes;
	t.tm_sec = time->seconds;
	return mktime(&t);
}

long c_date(p_itsp_str_date date){
	struct tm t = {0};
	t.tm_year = date->year - 1900;
	t.tm_mon = date->month - 1;
	t.tm_mday = date->day;
	return mktime(&t);
}


long c_date_time_tm(p_itsp_str_date d, p_itsp_str_time t){
	struct tm date_time_tm = {0};
	if(d){
		date_time_tm.tm_year = d->year - 1900;
		date_time_tm.tm_mon = d->month - 1;
		date_time_tm.tm_mday = d->day;
	}

	if(t){
		date_time_tm.tm_hour = t->hour;
		date_time_tm.tm_min = t->minutes;
		date_time_tm.tm_sec = t->seconds;
	}

	date_time_tm.tm_isdst = -1;

	return mktime(&date_time_tm);
}

long itsp_date_time(long ctime, p_itsp_str_date d, p_itsp_str_time t){

	ctime = ctime ? ctime : time(NULL);
	struct tm *date_time_tm = localtime((const time_t*)&ctime);

	if(d){
		d->year = date_time_tm->tm_year + 1900;
		d->month = date_time_tm->tm_mon + 1;
		d->day = date_time_tm->tm_mday;
	}

	if(t){
		t->hour = date_time_tm->tm_hour;
		t->minutes = date_time_tm->tm_min;
		t->seconds = date_time_tm->tm_sec;
	}

	return ctime;
}

char* sprintf_date_time(char* buff, long ctime, char data_time_flag){
	struct tm *date_time_tm = localtime((const time_t*)&ctime);
	switch(data_time_flag){
		case 'd' :
			return buff + sprintf(buff, "%02d/%02d/%04d",
							date_time_tm->tm_year +1900,
							date_time_tm->tm_mon + 1,
							date_time_tm->tm_mday);
		break;
		case 't' :
			return buff + sprintf(buff, "%02d:%02d:%02d",
							date_time_tm->tm_hour,
							date_time_tm->tm_min,
							date_time_tm->tm_sec);
		break;
		default :
			return buff + sprintf(buff, "%02d/%02d/%04d %02d:%02d:%02d",
							date_time_tm->tm_year +1900,
							date_time_tm->tm_mon + 1,
							date_time_tm->tm_mday,
							date_time_tm->tm_hour,
							date_time_tm->tm_min,
							date_time_tm->tm_sec);
		break;
	}
	return buff;
}



struct tm* long_2_tm(long *t){
	return localtime(t);
}

long tm_2_long(struct tm *tm){
	return mktime(tm);
}


void get_date_time(time_t *t, struct tm *tm){
	time_t tt = time(NULL);
	struct tm *tmt = localtime(&tt);
	if(t) 	*t = tt;
	if(tm) *tm = *tmt;
}

char* read_amount(char* source, double* value){

	char buff[128] = {0}, *ptr = buff;

	if(*source == '.'){
		*value = 0.0;
		source++;
		return source;
	}

	if((*source == '+') || (*source == '-')){
		*ptr++ = *source++;
	}

	while('0' <= *source && *source <= '9')
		*ptr++ = *source++;

	if((('P' <= *source) && (*source <= 'Y')) ||
	   (('`' <= *source) && (*source <= 'i')))
		*ptr++ = '.';

	while(('P' <= *source) && (*source <= 'Y'))
		*ptr++ = *source++ - 32;

	if(('@' <= *source) && (*source <= 'I'))
		*ptr++ = *source++ -16;
	else{
		if(('`' <= *source) && (*source <= 'i'))
			*ptr++ = *source++ - 48;
		else{
			fprintf(std_out, "read_amount error\n");
			abort();
		}
	}

	*value = strtod(buff, NULL);
	return source;
}

extern double exp10(double);

char* write_amount(char* buff, double amount, int signed_flag, int can_be_dot, int precision){

	char format[16] = {0}, amount_[64] = {0}, *ptr = NULL, delta = 0;
	int amount_round = 0;
	int ln = 0;

	amount *= exp10(precision);
	amount_round = lrint(amount);

	if(!amount_round){
		if(can_be_dot)
			*buff++ = '.';
		else{
			if(signed_flag) *buff++ = '+';
			*buff++ = '@';
		}
		return buff;
	}

	sprintf(format, "%%+.%dd", precision);
	sprintf(amount_, format, amount_round);

	if(signed_flag) *buff++ = amount_[0];

	ln = strlen(amount_);
	memmove(amount_, amount_ + 1, ln);
	ln--;

	memmove(amount_ + ln - precision + 1, amount_ + ln - precision, precision);
	if(precision) {
		amount_[ln - precision] = '.';
		ptr = amount_ + ln;
		while(*ptr == '0') *ptr-- = 0;
		if(*ptr == '.') *ptr-- = 0;
	}

	ptr = amount_;

	while(*(ptr + 1) && ((('0' <= *ptr) && (*ptr <= '9')) || (*ptr == '.'))){
		if(*ptr == '.') delta += 32;
		else *buff++= *ptr + delta;
		ptr++;
	}

	delta += 16;

	*buff++ = *ptr + delta;

	return buff;
}

char* read_time(char* source, p_itsp_str_time p_time){
	char tmp[3] = {0};
	memset(p_time, 0, sizeof(itsp_str_time));
	memcpy(tmp,source,2); 		p_time->hour 			= atoi(tmp);
	memcpy(tmp,source + 2,2); 	p_time->minutes 		= atoi(tmp);
	memcpy(tmp,source + 4,2); 	p_time->seconds 		= atoi(tmp);
	return source + 6;
}

char* write_time(char* buff, p_itsp_str_time p_time){
	buff += sprintf(buff, "%02d%02d%02d", p_time->hour, p_time->minutes, p_time->seconds);
	return buff;
}

char* read_date(char* source, p_itsp_str_date p_date){
	char tmp[5] = {0};
	memcpy(tmp,source,2); 		p_date->month 			= atoi(tmp);
	memcpy(tmp,source + 2,2); 	p_date->day 			= atoi(tmp);
	memcpy(tmp,source + 4,4); 	p_date->year 			= atoi(tmp);
	return source + 8;
}

char* write_date(char* buff, p_itsp_str_date p_date){
	buff += sprintf(buff, "%02d%02d%04d", p_date->month, p_date->day, p_date->year);
	return buff;
}

char* read_digits(char* source, int* value, int nb_dgts){
	char tmp[nb_dgts+1];
	tmp[nb_dgts] = 0;
	memcpy(tmp, source, nb_dgts);
	*value = atoi(tmp);
	return source + nb_dgts;
}

char* write_digits(char* buff, int value, int nb_dgts, int can_be_dots){
	if(value || !can_be_dots)
		buff += sprintf(buff, "%.*d", nb_dgts, value);
	else
		while(nb_dgts--) *buff++ = '.';
	return buff;
}

char* read_fixed(char* source, char* out, long unsigned int nb_chars){
	out[nb_chars] = 0;
	memcpy(out, source, nb_chars);
	return source + nb_chars;
}

char* write_fixed(char* buff, char* in, long unsigned int nb_chars){
	memcpy(buff, in, nb_chars);
	return buff + nb_chars;
}

char* read_numeric_range(char* source, int** out){
	long unsigned int sz = 0;
	char* ptr = source;
	int *buff = NULL, i = 0, a = 0;

	char nb[4] = {0};
	while(OK){
		switch(*ptr){
		case '-' :
			a = atoi(nb);
			memset(nb,0,4);
			i = 0;
			break;
		case ',' :
			if(a)	sz+= atoi(nb) - a + 1;
			else	sz++;
			memset(nb,0,4);
			i = a = 0;
			break;
		case '/' :
			if(a)	sz+= atoi(nb) - a + 1;
			else	sz++;
			goto allocation;
		case 0 :
			return NULL;
		default:
			if(('0' <= *ptr) && (*ptr <= '9'))
				nb[i++] = *ptr;
			else
				return NULL;
			break;
		}
		ptr++;
	}

	allocation : *out = malloc((sz + 1)* sizeof(int));
	buff = *out;

	ptr = source;
	i = a = 0;
	memset(nb,0,4);
	while(*ptr){
		switch(*ptr){
		case '-' :
			a = atoi(nb);
			memset(nb,0,4);
			i = 0;
			break;
		case ',' :
			if(a){
				int b = atoi(nb);
				for(; a <= b; a++)
					*buff++ = a;
			}else
				*buff++ = atoi(nb);
			memset(nb,0,4);
			i = a = 0;
			break;
		case '/' :
			if(a){
				int b = atoi(nb);
				for(; a <= b; a++)
					*buff++ = a;
			}else
				*buff++ = atoi(nb);
			*buff = 0;
			return ptr + 1;
		default:
			nb[i++] = *ptr;
			break;
		}
		ptr++;
	}

	free(*out);
	return NULL;
}

char* write_numeric_range(char* buff, int* range){

	int a = 0, b = 0;

	if(!range || !*range){
		*buff++ = '/';
		return buff;
	}

	while((a = *range)){

		while((*range + 1) == *(range + 1)){
			range++;
			b = *range;
		}

		range++;

		if(b)		buff += sprintf(buff, "%d-%d%c", a, b, *range ? ',' : '/');
		else if(a)	buff += sprintf(buff, "%d%c", a, *range ? ',' : '/');
		else 		break;

		a = b = 0;

	}

	return buff;
}

long int mumeric_range_cmp(int* range1, int* range2){
	char buff1[256] = {0}, buff2[256] = {0};

	write_numeric_range(buff1, range1);
	write_numeric_range(buff2, range2);

	return (long int)strcmp(buff1, buff2);
}

int numeric_range_cardinal(int* range){
	int card = 0;
	while(range[card]) card++;
	return card;
}

long int count_occurences(char* start, char* end, char c){
	long int out = 0;
	do{
		if(*start == c) out++;
	}while(++start < end);
	return out;
}

int itsp_header_check_sum(char* header){
	int sum = 0, i = HEADER_SIZE - CHECKSUM_SIZE;
	char sum_str[CHECKSUM_SIZE + 1];
	memset(sum_str, 0, CHECKSUM_SIZE + 1);

	while(i--)
		sum += *header++;

	sprintf(sum_str, "%0*o", CHECKSUM_SIZE, sum);

	return (memcmp(sum_str, header, 5) == 0);
}

int itsp_data_check_sum(char* data, long unsigned int data_size){
	int sum = 0, i = data_size - CHECKSUM_SIZE;
	char sum_str[CHECKSUM_SIZE + 1];
	memset(sum_str, 0, CHECKSUM_SIZE + 1);
	while(i--)
		sum += *data++;

	sum %= 0100000;

	sprintf(sum_str, "%0*o", CHECKSUM_SIZE, sum);

	return (memcmp(sum_str, data, 5) == 0);
}

char* itsp_write_check_sum(char* start, char* end){
	int out = 0;
	char buff[CHECKSUM_SIZE + 1];
	memset(buff, 0, CHECKSUM_SIZE + 1);
	while(start < end)
		out += *start++;
	out %= 0100000;
	sprintf(buff, "%0*o", CHECKSUM_SIZE, out);
	memcpy(end, buff, CHECKSUM_SIZE);
	return end + CHECKSUM_SIZE;
}

char* write_combo(char* buff, int** combo){
	int i = 0;
	while(combo[i])
		buff = write_numeric_range(buff, combo[i++]);
	return buff;
}

char* read_combo(char* buff, int*** combo_){
	int i = 0, **combo = NULL, sz = 0;
	char* ptr = buff;

	while(((*ptr <= '9') && ('0' <= *ptr)) || (*ptr == '-') || (*ptr == ',') || (*ptr == '/'))
		if(*ptr++ == '/') i++;

	if(!*combo_)
		*combo_ = malloc((i+1) * sizeof(int*));

	combo = *combo_;
	combo[i] = NULL;

	sz = i; i = 0;
	while(i < sz)
		buff = read_numeric_range(buff, &combo[i++]);

	return buff;
}

long combo_cmp_func(const void* combo1, const void* combo2){
	char buff1[1024] = {0}, buff2[1024] = {0};

	write_combo(buff1, (int**)combo1);
	write_combo(buff2, (int**)combo2);

	return (long)strcmp(buff1, buff2);
}

int** clone_combo(int** combo){
	int ** out = NULL, i = 0;

	while(combo[i++])
		continue;

	out = malloc(i-- * sizeof(int*));

	out[i] = NULL;

	while(i--){
		int j = 0;

		while(combo[i][j++])
			continue;

		out[i] = malloc(j-- * sizeof(int));

		do out[i][j] = combo[i][j];
		while(j--);

	}

	return out;
}

int** make_empty_combo(int i){
	int** combo = malloc((i + 1) * sizeof(int*));
	combo[i] = NULL;
	while(i--){
		combo[i] = malloc(sizeof(int));
		combo[i][0] = 0;
	}
	return combo;
}

int combo_cardinal(int** combo){
	int dimz = 0;
	while(*combo++) dimz++;
	return dimz;
}

void free_combo(int** combo){
	int i = 0;
	while(combo[i]) free(combo[i++]);
	free(combo);
}







