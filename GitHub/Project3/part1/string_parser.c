#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim)
{
	if (buf == NULL){
		return 0;
	}
	else{
		int tok_count = 0;
		char* saveptr, *copy_buf;
		copy_buf = strdup(buf);
		char* token = strtok_r(copy_buf, delim, &saveptr);
		while (token!=NULL){
			tok_count++;
			token = strtok_r(NULL, delim, &saveptr);
		}
		free(copy_buf);
		return tok_count;
	}
}

command_line str_filler (char* buf, const char* delim)
{
	command_line result;
	result.num_token = count_token(buf, delim);
	result.command_list = (char**)malloc((result.num_token+1)*sizeof(char*));
	char* saveptr1, *saveptr2;
	char* copy_buf;
       	copy_buf = strdup(buf);
	strtok_r(copy_buf, "\n", &saveptr1);
	int count = 0;
	char* tok= strtok_r(copy_buf, delim, &saveptr2);
	while (tok != NULL){	
		result.command_list[count] = strdup(tok);
		count++;
		tok = strtok_r(NULL, delim, &saveptr2);
	}
	result.command_list[count] = NULL;
	free(copy_buf);
	return result;
}


void free_command_line(command_line* command)
{
	for (int i = 0; command->command_list[i]!=NULL; i++){
		free(command->command_list[i]);
	}
	free(command->command_list);
}
