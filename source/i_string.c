/*
 * i_string.c
 *
 *  Created on: 7 mars 2017
 *      Author: f.baccari
 */

#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <i_tools.h>
#include <i_string.h>

#define MAX_TAB_SZ 100

char* sub2str(const sub_str sb){
	long unsigned int z = SUB_STR_LEN(sb);
	char* out = malloc(z + 1);
	if(z) memcpy(out, sb.start, z);
	out[z] = 0;
	return out;
}

sub_str* match_expr(char* source, char* pattern, long int exec_flags){
	static regex_t regexCompiled = {0};
	static char* last_pattern = NULL;
	static long int last_pattern_lenth = 0;
	static sub_str *output = NULL;

	if(!last_pattern || strcmp(pattern, last_pattern)){
		regfree(&regexCompiled);
		if (regcomp(&regexCompiled, pattern, REG_NEWLINE | REG_EXTENDED)){
		  DEBUG_WARN("Regular expression error in '%s'\n", pattern);
		  last_pattern = NULL;
		  last_pattern_lenth = 0;
		  regfree(&regexCompiled);
		  return NULL;
		}
		long int tmp_len = strlen(pattern);
		if(last_pattern_lenth < tmp_len){
			last_pattern = realloc(last_pattern, tmp_len + 1);
			last_pattern_lenth = tmp_len;
		}
		strcpy(last_pattern, pattern);
		output = realloc(output, (regexCompiled.re_nsub+2) * sizeof(sub_str));
		output[regexCompiled.re_nsub+1] = (sub_str){0};
	}

	{
		regmatch_t substr[regexCompiled.re_nsub + 1];
		if (regexec(&regexCompiled, source, regexCompiled.re_nsub + 1, substr, exec_flags))
			return NULL;

		int i = regexCompiled.re_nsub+1;
		while(i--)
			output[i] = SUB_STRING(source + substr[i].rm_so, source + substr[i].rm_eo);
	}
	return output;
}

int replace_expr(char* c_str, char* expression, char* substitute){

	long int len = strlen(c_str),
			new_len = strlen(substitute),
			delta = 0, subtab_indice = 0;
	char 	*start = c_str,
			*end = c_str + len;

	const sub_str *tmps = NULL;
	sub_str subs_ = SUB_STRING(substitute, substitute + new_len);

	static sub_str *subtab = NULL;
	static long int subtab_sz = 0;
	if(!subtab){
		subtab = malloc(MAX_TAB_SZ * sizeof(sub_str));
		subtab_sz = MAX_TAB_SZ;
	}

	while((tmps = match_expr(start, expression, 0))){
		if(start < tmps[1].start)
			subtab[subtab_indice++] = SUB_STRING(start, tmps[1].start);
		if(subtab_sz - 1 <= subtab_indice){
			void* tmp = malloc(2 * subtab_sz * sizeof(sub_str));
			memcpy(tmp, subtab, subtab_sz * sizeof(sub_str));
			free(subtab);
			subtab = tmp;
			subtab_sz *= 2;
		}
		if(new_len)
			subtab[subtab_indice++] = subs_;
		delta += new_len - SUB_STR_LEN(tmps[1]);
		start = tmps[1].end;
	}
	if(start < end)
		subtab[subtab_indice++] = SUB_STRING(start, end);

	if(subtab_indice){
		int i = 0;
		while(i<subtab_indice){
			if(c_str != subtab[i].start)
				memmove(c_str,subtab[i].start,SUB_STR_LEN(subtab[i]));
			c_str += SUB_STR_LEN(subtab[i]);
			i++;
		}
		*c_str = 0;
		return OK;
	}
	return FAULT;
}

void concat_lines(char* text){
	char str[strlen(text) + 1], *ptr = str;
	int i = 0;

	while(text[i]){
		if((text[i] != '\r') && (text[i] != '\n'))
			*ptr++ = text[i];
		i++;
	}
	*ptr = 0;
	strcpy(text, str);
}
