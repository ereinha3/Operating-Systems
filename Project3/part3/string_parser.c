/*
 * string_parser.c
 *
 *  Created on: Nov 25, 2020
 *      Author: gguan, Monil
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	Check for NULL string
	*	#2.	iterate through string counting tokens
	*		Cases to watchout for
	*			a.	string start with delimeter
	*			b. 	string end with delimeter
	*			c.	account NULL for the last token
	*	#3. return the number of token (note not number of delimeter)
	*/
	char *tkn, *saveptr1, *str1, *placeholder;
	int counter = 1;

	strtok_r(buf, "\n", &saveptr1);

	for(placeholder = str1 = strdup(buf); ; str1 = NULL)
	{
		tkn = strtok_r(str1, delim, &saveptr1);
		if(tkn == NULL)
			break;
		counter++;
	}
	
	free(placeholder);
	return counter;
}

command_line str_filler (char* buf, const char* delim)
{
	int i;
	char *tkn, *saveptr1, *str1, *placeholder;
	command_line cmd; // number 1
	cmd.num_token = count_token(buf, delim); // number 2

	cmd.command_list = (char **) malloc(sizeof(char *) * cmd.num_token);

	for(i = 0, placeholder = str1 = strdup(buf); ; i++, str1 = NULL)
	{
		tkn = strtok_r(str1, delim, &saveptr1);
		if(tkn == NULL)
			break;
		cmd.command_list[i] = strdup(tkn);
	}

	free(placeholder);
	cmd.command_list[cmd.num_token - 1] = NULL;
	return cmd;
	


	//TODO：
	/*
	*	#1.	create command_line variable to be filled and returned
	*	#2.	count the number of tokens with count_token function, set num_token. 
    *           one can use strtok_r to remove the \n at the end of the line.
	*	#3. malloc memory for token array inside command_line variable
	*			based on the number of tokens.
	*	#4.	use function strtok_r to find out the tokens 
    *   #5. malloc each index of the array with the length of tokens,
	*			fill command_list array with tokens, and fill last spot with NULL.
	*	#6. return the variable.
	*/
}


void free_command_line(command_line* command)
{
	//TODO：
	/*
	*	#1.	free the array base num_token
	*/
	for(int i = 0; i < command->num_token; i++)
	{
		free(command->command_list[i]);
	}
	free(command->command_list);
}
