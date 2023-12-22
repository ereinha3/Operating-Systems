#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<unistd.h>

#include"command.h"
#include"string_parser.h"
int main(int argc, char** argv) {
	// Boolean to ensure proper syntax
	int validity = 0;
	// Boolean to establish file mode or shell mode
	int file_output = 0;
	// Placeholder variable if file name is given
	char* filename;
	// Declaring name of output file to be written to
	char* output_filename = "output.txt";
	FILE *fp;
	// Ensuring proper usage or first argument
	if (strcmp(argv[0], "./pseudo-shell")){
		printf("Usage: ./pseudo-shell [-f {filename}]\n");
		return 0;
		}
	// Making sure file exists and preceded by -f flag, otherwise error messaging and exit program
	// This puts the program in "file mode" meaning commands will be read from a file and printed to an output file
	if (argc==3){
		if (!strcmp(argv[1], "-f")){
			validity = 1;
			file_output = 1;
			filename = argv[2];
			fp = fopen(filename, "r");
			if (!fp){
				printf("Input file, %s, failed to open! Check permissions/file contents!\n", filename);
				return 0;
			}
			// Open output file to write to and change standard output to be written to this destination
			freopen(output_filename, "w", stdout);
		}
	}
	// Proper usage of first argument has already been checked, the program has already been set to "shell mode"
	if (argc == 1){
		validity = 1;
	}
	// Buffer, len, and line variables declared or initialized for use in getline() function
	char* b = NULL;
	size_t len;
	ssize_t line;
	// Only executes if validity boolean was true
	while (validity){
		// If in shell mode, read standard input
		if (!file_output){
			printf(">>>");
			line = getline(&b, &len, stdin);
		}
		// Otherwise get a line from the file
		else {
			line = getline(&b, &len, fp);
		}
		// If we are in file mode and getline() has signaled we are at the end of the line, close, free and exit
		if ((line == -1)&&file_output){
			char* bye_msg = "End of file\nBye Bye!\n";
			write(1, bye_msg, strlen(bye_msg));
			fclose(fp);
			free(b);
			return 0;
		}
		// Otherwise we have an error so write a message
		if (line == -1){
			char* err_msg = "Error interpretting input when using getline().\n";
			write(1, err_msg, strlen(err_msg));
		}
		// Using created function to parse string into individual tokens separated by a delimiter
		command_line large_tokens = str_filler(b, ";");
		// If empty, print no command was inputted and free
		if (large_tokens.num_token == 0){
			free_command_line(&large_tokens);
			char * err_msg = "No command was inputted\n";
			write(1, err_msg, strlen(err_msg));
		}
		else{
			// Iterating over tokens up to the last one (last is NULL)
			for (int i = 0; large_tokens.command_list[i]!=NULL; i++) {
				// Parsing by spaces to get individual commands / arguments
				command_line small_tokens = str_filler(large_tokens.command_list[i], " ");
				// Separating the command from the rest of the tokens
				char * command = small_tokens.command_list[0];
				int count = small_tokens.num_token;
				// Execute ls command if matched
				if (!strcmp(command,"ls")){
						listDir();
					}
				// Execute pwd command if matched
				else if (!strcmp(command, "pwd")){
						showCurrentDir();
					}
				// Execute mkdir command if matched
				else if (!strcmp(command, "mkdir")){
					// Ensure there are two arguments otherwise error messaging
					if (!count==2){
						char* err_msg = "Mkdir command requires argument for directory name";
						write(1, err_msg, strlen(err_msg));
					}
					// Otherwise pass in second token as new directory name
					else{
						makeDir(small_tokens.command_list[1]);
					}
				}
				// Execute cd command if matched
				else if (!strcmp(command, "cd")){
					// Ensure there are two arguments otherwise error messaging
					if (!count==2){
						char* err_msg = "Cd command requires argument for directory name";
						write(1, err_msg, strlen(err_msg));
					}
					// Otherwise pass in second token as directory name
					else{
						changeDir(small_tokens.command_list[1]);
					}
				}
				// Execute cp command if matched
				else if (!strcmp(command, "cp")){
					// Ensure there are three arguments otherwise error messaging
					if (!count==3){
						char* err_msg = "Cp command requires argument for source and destinatino file";
						write(1, err_msg, strlen(err_msg));
					}
					// Pass in source and destination file names into function
					else{
						copyFile(small_tokens.command_list[1], small_tokens.command_list[2]);
					}
				}
				// Execute mv command if matched
				else if (!strcmp(command, "mv")){
					// Ensure there are three arguments otherwise error messaging
					if (!count==3){
						char* err_msg = "Mv command requires argument for source and destinatino file";
						write(1, err_msg, strlen(err_msg));
					}
					// Pass in source and destination file names into function
					else{
						moveFile(small_tokens.command_list[1], small_tokens.command_list[2]);
					}
				}
				// Execute rm command if matched
				else if (!strcmp(command, "rm")){
					// Ensure there are two arguments otherwise error messaging
					if (!count==2){
						char* err_msg = "Rm command requires argument for directory name";
						write(1, err_msg, strlen(err_msg));
					}
					// Pass in filename to be deleted
					else{
						deleteFile(small_tokens.command_list[1]);
					}
				}
				// Execute rm command if matched
				else if (!strcmp(command, "cat")){
					// Ensure there are two arguments otherwise error messaging
					if (!count==2){
						char* err_msg = "Cat command requires argument for directory name";
						write(1, err_msg, strlen(err_msg));
					}
					// Pass in filename to have contents printed
					else{
						displayFile(small_tokens.command_list[1]);
					}
				}
				// If commmand is exit and this is the only token
				else if (!strcmp(command, "exit")&&count==1){
					// Free both command_line variables
					free_command_line(&small_tokens);
					free_command_line(&large_tokens);
					// Close output file is in file mode
					if (file_output){
						fclose(fp);
					}
					// Free buffer
					free(b);
					// Exit
					return 0;
				}
				// This means nothing matched and the command was unrecognized, error messaging
				else {
					char* err_msg = "Error! Unrecognized command: ";
					write(1, err_msg, strlen(err_msg));
					write(1, command, strlen(command));
					write(1, "\n", strlen("\n"));
				}
				// Free the command_line variable to avoid memory leaks
				free_command_line(&small_tokens);
			}
		// Free the larger command_line variable to avoid memory leaks
		free_command_line(&large_tokens);
		}
	}
	// If the program is in file mode, close the output file
	if (file_output){
		fclose(fp);
	}
	// Free the buffer
	free(b);
}
