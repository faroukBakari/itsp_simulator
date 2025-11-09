/*
 * i_file.c
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <regex.h>
#include <fcntl.h>
#include <limits.h>
#define PATH_MAX        4096	/* # chars in a path name including nul */
extern char *realpath (__const char *__restrict __name,
		       char *__restrict __resolved) __THROW;

#include <i_tools.h>
#include <i_string.h>
#include <i_file.h>
#include <i_exit.h>

#define BUFFER_SIZE 256

pi_file init_i_file(char* name){
	char path[PATH_MAX] = {0};
	pi_file fp = malloc(sizeof(i_file));
	memset(fp, 0, sizeof(i_file));

	fp->name = malloc(strlen(name) + 1);
	strcpy(fp->name, name);

	realpath(name, path);
	fp->path = malloc(strlen(path) + 1);
	strcpy(fp->path, path);

	if(stat(name, &fp->attributes) == -1){
		DEBUG_WARN("stat call for file \"%s\" fails with error %d : %s.", name, errno, strerror(errno));
		abort();
	}

	if (!(fp->header = fopen(name, "r+"))){
		DEBUG_WARN("fopen error %d for \"%s\" : %s.", errno, name, strerror(errno));
		abort();
	}

	return fp;
}

void free_i_file(pi_file fp){
	fclose(fp->header);
	free(fp->name);
	free(fp->path);
}

long unsigned int remaining_file_data(FILE* fp){
	long unsigned int pos = ftell(fp), out = 0;
	fseek(fp, 0, SEEK_END);
	out = ftell(fp) - pos;
	fseek(fp, pos, SEEK_SET);
	return out;
}

FILE* open_file(char* filename, char* open_flag){
	FILE* file = NULL;
	if (!(file = fopen(filename, open_flag))){
		DEBUG_WARN("fopen error %d for ""%s"" : %s.", errno, filename, strerror(errno));
		abort();
	}
	return file;
}

char*  get_chars(char* buffer, long int size, FILE* this){

	char value = 0;

	while(size-- && (value = fgetc(this)))
		*buffer++ = value;

	return buffer;
}

long unsigned int  get_line(char* buff, FILE* this){

	char value = 0, *ptr = buff;
	long int start = 0;
	int ctr = 0;

	do
		start = ftell(this);
	while(((value = fgetc(this)) == '\r') || (value == '\n'));

	if(value == EOF)
		return 0;

	do {
		ctr++;
		if(ptr) *ptr++ = value;
	} while(((value = fgetc(this))  != EOF) && (value != '\n') && (value != '\r'));

	if(ptr){
		*ptr = 0;
		if(value != EOF) fseek(this, -1, SEEK_CUR);
	}
	else fseek(this, start, SEEK_SET);

	return ctr;
}

char* load_file(FILE* fp){
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	char* file_data = malloc(sz+1);
	rewind(fp);
	if(fread(file_data, 1, sz, fp) != sz)
		return NULL;
	file_data[sz] = 0;
	return file_data;
}

long unsigned int file_size(FILE* fp){
	long unsigned int current = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	long unsigned int size = ftell(fp);
	fseek(fp, current, SEEK_SET);
	return size;
}

char** load_file_lines(FILE* fp){
	char *lines[10240] = {NULL}, **out = NULL;
	int line_ctr = 0, lenght = 0;

	while((lenght = get_line(NULL, fp))){
		lines[line_ctr] = malloc(line_ctr + 1);
		get_chars(lines[line_ctr], lenght, fp);
		lines[line_ctr][lenght] = 0;
		if(line_ctr++ > 10240){
			DEBUG_WARN("load_file_lines error max lines reached");
			abort();
		}
	}

	if(line_ctr){
		out = malloc((line_ctr + 1) * sizeof(char*));
		memcpy(out, lines, (line_ctr + 1) * sizeof(char*));
	}

	return out;
}










