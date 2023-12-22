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
static volatile sig_atomic_t reset_alarm = 1;

void reset(){
	reset_alarm = 1;
}

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
	sigset_t sigset;
	sigemptyset(&sigset);
	signal(SIGALRM, reset);
	if (sigaddset(&sigset, SIGUSR1) == -1){
		fprintf(stderr, "Add signal SIGUSR1 failed!\n");
	}
	if (sigaddset(&sigset, SIGALRM) == -1){
		fprintf(stderr, "Add signal SIGALRM failed!\n");
	}
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) == -1){
		fprintf(stderr, "Add signal SIG_BLOCK failed!\n");
	}
	pid_t pid_array[line_number];
	pid_t ppid = getpid();
	pid_t pid = 1;
	for (int i = 0; i<line_number; i++){
		line = getline(&b, &len, fp);
		if (pid>0) {
			pid = fork();
		}
		pid_array[i] = pid;
		if (pid == 0){
			char* saveptr;
			strtok_r(b, "\n", &saveptr);
			int sig = -1;
			while (sig != SIGUSR1){
				sigwait(&sigset, &sig);
			}
			char ** command_list = str_filler(b, " ");
			if (execvp(command_list[0], command_list)==-1){
				free_command_line(command_list);
				fclose(fp);
				free(b);
				kill(ppid, SIGUSR1);
				exit(-1);
			}
		}
		if (pid < 0){
			fprintf(stderr, "Fork failed!\n");
			for (int j=0; j<i; j++){
				kill(pid_array[j], SIGINT);
				free(b);
				fclose(fp);
				kill(ppid, SIGUSR1);
				exit(-1);
			}
		}
	}
	free(b);
	fclose(fp);
	printf("Processes created:\n");
	signaler(pid_array, line_number, SIGUSR1);
	signaler(pid_array, line_number, SIGSTOP);
	int status[line_number];
	for (int i = 0; i<line_number; i++){
		printf("process %d with pid <%d>\n", i, pid_array[i]);
		status[i] = 0;
	}
	int finished = 0;
	int stat = 0;
	int curr_process = 0;
	int next_process;
	int sig;
	kill(pid_array[0], SIGCONT);
	printf("Started process %d with pid <%d>\n", 0, pid_array[0]);
	while (1){
		if (status[curr_process]==0){
			alarm(1);
			if (sigwait(&sigset, &sig)<0){
				fprintf(stderr, "Error waiting for signal from <%d> in parent process.\n", pid_array[curr_process]);
			}
			if (sig == SIGUSR1){
				status[curr_process] = 1;
				printf("Process %d with pid <%d> executed unsuccesfully!\n", curr_process, pid_array[curr_process]);
			}
			else {
				if (kill(pid_array[curr_process], SIGSTOP)<0){
					fprintf(stderr, "Error trying to stop child process <%d>.\n", pid_array[curr_process]);
				}
				else{
					printf("Stopped process %d with pid <%d>\n", curr_process, pid_array[curr_process]);
				}
				if (waitpid(pid_array[curr_process], &stat, WNOHANG | WUNTRACED | WCONTINUED)==-1){
					fprintf(stderr, "Error calling waitpid() on child process <%d>!\n", pid_array[curr_process]);
				}
				else {
					unsigned int eval = WIFEXITED(stat);
					if (eval>0){
						printf("Child process %d with pid <%d> executed successfully!\n", curr_process, pid_array[curr_process]);
						status[curr_process] = eval;
					}
				}
			}
		}
		reset_alarm = 0;
		if ((curr_process+1)==line_number){
			next_process = 0;
		}
		else {
			next_process = curr_process+1;
		}
		if (status[next_process] == 0){
			if (kill(pid_array[next_process], SIGCONT)<0){
				fprintf(stderr, "Error trying to start child process <%d>.\n", pid_array[next_process]);
			}
			else{
				printf("Started process %d with pid <%d>\n", next_process, pid_array[next_process]);
			}
		}
		/*
		for (int i = 0; i<line_number; i++){
			int tempstat = stat;
			if (status[i]!=1) {
				waitpid(pid_array[i], &stat, WNOHANG | WUNTRACED | WCONTINUED);
			}
			if (tempstat == stat){
				status[i] = 1;
			}
			unsigned int eval = WIFEXITED(stat);
			if (eval>0){
				status[i] = eval;
			}
		}
		*/
		curr_process = next_process;
		for (int i = 0; i<line_number; i++){
			if (status[i] == 1){
				finished++;
			}
		}
		if (finished == line_number){
			break;
		}
		finished = 0;
	}
	signaler(pid_array, line_number, SIGINT);
	for (int i=0; i<line_number; i++){
		printf("Process <%d> finished with status <%d>\n", pid_array[i],status[i]);
	}
	printf("All processes finished successfully!\n");
	return 0;
}

void signaler(pid_t* pid_ary, int size, int signal)
{
	for(int i = 0; i < size; i++)
	{	
		pid_t child_pid = pid_ary[i];
		kill(child_pid, signal);
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
