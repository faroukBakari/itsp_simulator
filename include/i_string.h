/*
 * i_string.h
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#ifndef INCLUDE_I_STRING_H_
#define INCLUDE_I_STRING_H_

#include <string.h>

typedef struct sub_str{
	char* start;
	char* end;
}sub_str;

#define SUB_STRING(start,end) ({\
	sub_str syb_str_0 = {start,end};\
	syb_str_0;\
})

#define SUB_STR_LEN(sub) (long int)((sub).end - (sub).start)

extern char* sub2str(sub_str sb);

extern sub_str* match_expr(char* source, char* pattern, long int exec_flags);

extern int replace_expr(char* c_str, char* expression, char* substitute);

extern void concat_lines(char* text);

#endif /* INCLUDE_I_STRING_H_ */
