#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim)
{
	// If the string is NULL, return 0
	if (buf == NULL){
		return 0;
	}
	else{
		// Initiliaze count to 0 and declare saveptr and buffer copy to use in strtok_r() function
		// Buffer copy needed as strtok_r() will mess with actual string
		int tok_count = 0;
		char* saveptr, *copy_buf;
		copy_buf = strdup(buf);
		// Initially pass in buffer and saveptr will point to next token location
		char* token = strtok_r(copy_buf, delim, &saveptr);
		// Now keep iterating until end of string is reached which is when the token returned is NULL
		// Increment count on each iteration
		while (token!=NULL){
			tok_count++;
			token = strtok_r(NULL, delim, &saveptr);
		}
		// Free buffer copy
		free(copy_buf);
		// Return number of tokens counted
		return tok_count;
	}
}

command_line str_filler (char* buf, const char* delim)
{
	// Declaring command_line variable to be used, description given in header file
	command_line result;
	// Calling count_token function to determine number of tokens
	result.num_token = count_token(buf, delim);
	// Allocating memory to created an array of num_token character pointers
	result.command_list = (char**)malloc((result.num_token+1)*sizeof(char*));
	// Declaring variables to be used in strtok_r
	char* saveptr1, *saveptr2;
	char* copy_buf;
       	copy_buf = strdup(buf);
	// Getting rid of the newline character
	strtok_r(copy_buf, "\n", &saveptr1);
	int count = 0;
	// Using strtok_r to iterate over tokens separated by the delimiter
	char* tok= strtok_r(copy_buf, delim, &saveptr2);
	while (tok != NULL){	
		result.command_list[count] = strdup(tok);
		count++;
		tok = strtok_r(NULL, delim, &saveptr2);
	}
	// Setting last token to NULL
	result.command_list[count] = NULL;
	// Freeing buffer copy
	free(copy_buf);
	// Returning the command_line variable
	return result;
}


void free_command_line(command_line* command)
{
	// Freeing each command in the list
	for (int i = 0; command->command_list[i]!=NULL; i++){
		free(command->command_list[i]);
	}
	// Freeing entire command_line variable
	free(command->command_list);
}
