#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>


void signaler(pid_t* pid_ary, int size, int signal);
int count_token (char* buf, const char* delim);
char** str_filler (char* buf, const char* delim);
void free_command_line(char** command_list);

int main(int argc, char* argv[])
{
	
	// initialization of pid etc. like lab 4

	// initialize sigset
	char* filename;
	FILE *fp;
	if (argc != 2){
		fprintf(stderr, "Usage: ./MCP {filename}\n");
		return 0;
	}
	filename = argv[1];
	fp = fopen(filename, "r");
	if (!fp){
		fprintf(stderr, "File failed to open! Check permissions on file!\n");
		return 0;
	}
	char* b = NULL;
	size_t len;
	ssize_t line=0;
	int line_number = -1;
	while (line != -1){
		line = getline(&b, &len, fp);
		line_number+=1;
	}
	line = 0;
	rewind(fp);
	pid_t pid_array[line_number];
	pid_t pid = getpid();
	for (int i = 0; i<line_number; i++){
		line = getline(&b, &len, fp);
		if (pid>0) {
			pid = fork();
		}
		printf("Child process created with pid <%d>\n", pid);
		pid_array[i] = pid;
		if (pid == 0){
			char* saveptr;
			strtok_r(b, "\n", &saveptr);
			char ** command_list = str_filler(b, " ");
			if (execvp(command_list[0], command_list)==-1){
				fprintf(stderr, "Execution failed!");
				free_command_line(command_list);
				fclose(fp);
				free(b);
				exit(-1);
			}
		}
		if (pid < 0){
			fprintf(stderr, "Fork failed!\n");
			for (int j=0; j<i; j++){
				kill(pid_array[j], SIGINT);
				exit(-1);
			}
		}
	}
	free(b);
	fclose(fp);
	for (int i = 0; i<line_number; i++) {
		waitpid(pid_array[i], NULL, 0);
	}
	return 0;
}

void signaler(pid_t* pid_ary, int size, int signal)
{
	// sleep for three seconds
	sleep(3);
	pid_t pid = getpid();
	for(int i = 0; i < size; i++)
	{	
		pid_t child_pid = pid_ary[i];
		printf("Parent process: <%d> - Senging signal: <%s> to child process: <%d>\n", pid, strsignal(signal), child_pid);
		kill(child_pid, signal);
		// print: Parent process: <pid> - Sending signal: <signal> to child process: <pid>
		// send the signal
	}
}


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

char** str_filler (char* buf, const char* delim)
{
	int num_token = count_token(buf, delim);
	char ** command_list = (char**)malloc((num_token+1)*sizeof(char*));
	char* saveptr1, *saveptr2;
	char* copy_buf;
       	copy_buf = strdup(buf);
	strtok_r(copy_buf, "\n", &saveptr1);
	int count = 0;
	char* tok= strtok_r(copy_buf, delim, &saveptr2);
	while (tok != NULL){	
		command_list[count] = strdup(tok);
		count++;
		tok = strtok_r(NULL, delim, &saveptr2);
	}
	command_list[count] = NULL;
	free(copy_buf);
	return command_list;
}


void free_command_line(char** command_list)
{
	for (int i = 0; command_list[i]!=NULL; i++){
		free(command_list[i]);
	}
	free(command_list);
}
