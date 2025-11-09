/*
 * i_file.h
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_I_FILE_H_
#define INCLUDE_I_FILE_H_

#include <stdio.h>
#undef __USE_MISC
#include <sys/stat.h>

typedef struct i_file{
	char* name;
	char* path;
	FILE* header;
	struct stat attributes;
}i_file, *pi_file;

extern pi_file init_i_file(char* name);

extern void free_i_file(pi_file fp);

extern long unsigned int remaining_file_data(FILE* fp);

extern FILE* open_file(char* filename, char* open_flag);

extern char*  get_chars(char* buffer, long int size, FILE* this);

extern long unsigned int  get_line(char* buffer, FILE* this);

extern char* load_file(FILE* fp);

extern char** load_file_lines(FILE* fp);

extern long unsigned int file_size(FILE* fp);

#endif /* INCLUDE_I_FILE_H_ */
