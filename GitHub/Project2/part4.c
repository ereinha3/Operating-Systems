#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>


void signaler(pid_t* pid_ary, int size, int signal);
int count_token (char* buf, const char* delim);
char** str_filler (char* buf, const char* delim);
void free_command_line(char** command_list);
static volatile sig_atomic_t reset_alarm = 1;
int CLK_TCK = 100;

// Function to reset alarm once timer has been reached
void reset(){
	reset_alarm = 1;
}

// This command is somewhat analogous to the top command in terminal
// Instead, this command prints only a variety of information that I determined relevant
void handle_top(int num, pid_t pid, long time){
	// Syntax to open proc/pid/status or the proc file for the given process
	FILE *fp;
	char buf[100];
	char* b = NULL;
	size_t len;
	sprintf(buf, "/proc/%d/status", pid);
	fp = fopen(buf, "r");
	int print = 0;
	printf("----------------------------\n");
	printf("Printing information for process %d with pid <%d>...\n", num, pid);
	// Iterating over the proc file and printing only relevant information for the process
	// This was done by using strcmp for relevant information and using a boolean dictating whether the line would be printed
	while (getline(&b, &len, fp)>0){
		if (!strncmp("Name", b, 4)){
			print = 1;
		}
		if (!strncmp("State", b, 5)){
			print = 1;
		}
		if (!strncmp("VmSize", b, 6)){
			print = 1;
		}
		if (!strncmp("VmPeak", b, 6)){
			print = 1;
		}
		if (!strncmp("Threads", b, 7)){
			print = 1;
		}
		if (!strncmp("PPid", b, 4)){
			print = 1;
		}
		if (print){
			printf("%s", b);
		}
		print = 0;
	}
	// Printing the execution time which was roughly calculated by sys/time methods and subtracting the time at the start of the program from the time at the end
	float exec_time = (float)time/1000000;
	printf("Approximate execution time: %f seconds.\n", exec_time);
	fclose(fp);
	// The proc/pid/stat file is significantly more difficult to read as there is no human-readable text
	// However, lines 14 and 15 correspond to the number of clock ticks that the process has been running in user mode and kernel mode, respectively
	sprintf(buf, "/proc/%d/stat", pid);
	fp = fopen(buf, "r");
	getline(&b, &len, fp);
	char* saveptr;
	strtok_r(b, " ", &saveptr);
	for (int i = 0; i<12; i++){
		strtok_r(NULL, " ", &saveptr);
	}
	// The combination of user mode and kernel mode time divided by the number of clock ticks in a second corresponds to the total number of time each process was allocated to the CPU for
	int utime = atoi(strtok_r(NULL, " ", &saveptr));
	int stime = atoi(strtok_r(NULL, " ", &saveptr));
	float cputime = (stime+utime)/CLK_TCK;
	printf("The total CPU time for this process's allocated time is %f seconds.\n", cputime);
	printf("----------------------------\n");
	free(b);
	fclose(fp);
}

// Function to add the signals below to a given sigset
void add_signals(sigset_t* sigset){
	if (sigaddset(sigset, SIGUSR1) == -1){
		fprintf(stderr, "Add signal SIGUSR1 failed!\n");
	}
	if (sigaddset(sigset, SIGALRM) == -1){
		fprintf(stderr, "Add signal SIGALRM failed!\n");
	}
	if (sigprocmask(SIG_BLOCK, sigset, NULL) == -1){
		fprintf(stderr, "Add signal SIG_BLOCK failed!\n");
	}
}




int main(int argc, char* argv[])
{
	// initialize sigset
	char* filename;
	FILE *fp;
	// Ensuring proper usage
	if (argc != 2){
		fprintf(stderr, "Usage: ./part{i} {filename}\n");
		return 0;
	}
	filename = argv[1];
	fp = fopen(filename, "r");
	// Error messaging
	if (!fp){
		fprintf(stderr, "File failed to open! Check permissions on file!\n");
		return 0;
	}
	char* b = NULL;
	size_t len;
	ssize_t line=0;
	int line_number = -1;
	// Determining line count of file
	while (line != -1){
		line = getline(&b, &len, fp);
		line_number+=1;
	}
	line = 0;
	rewind(fp);
	// Creating a sigset and setting signals
	sigset_t sigset;
	sigemptyset(&sigset);
	signal(SIGALRM, reset);
	add_signals(&sigset);
	// Creating a pid array of all processes to be created
	pid_t pid_array[line_number];
	pid_t ppid = getpid();
	pid_t pid = 1;
	// Iterating over the number of lines to create that many processes
	for (int i = 0; i<line_number; i++){
		line = getline(&b, &len, fp);
		// Forking from parent
		if (pid>0) {
			pid = fork();
		}
		// Storing pid in pid array
		pid_array[i] = pid;
		// If this is the child
		if (pid == 0){
			char* saveptr;
			strtok_r(b, "\n", &saveptr);
			int sig = -1;
			// Waiting for user-defined signal which will be signaled once all processes are created
			while (sig != SIGUSR1){
				sigwait(&sigset, &sig);
			}
			// Created a command_list from user created data type
			char ** command_list = str_filler(b, " ");
			// Executing the command using execvp
			// This will free all memory and exit on success
			if (execvp(command_list[0], command_list)==-1){
				// Otherwise manually free and exit if faulty command
				free_command_line(command_list);
				fclose(fp);
				free(b);
				kill(ppid, SIGUSR1);
				exit(-1);
			}
			free_command_line(command_list);
		}
		// This would mean the fork failed and handle with free and error messaging
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
	// Freeing buffer and closing file
	free(b);
	fclose(fp);
	printf("Processes created:\n");
	// Signal all processes to begin and then stop them immediately
	signaler(pid_array, line_number, SIGUSR1);
	signaler(pid_array, line_number, SIGSTOP);
	int status[line_number];
	for (int i = 0; i<line_number; i++){
		printf("process %d with pid <%d>\n", i, pid_array[i]);
		status[i] = 0;
	}
	// Declare and initialize variables to be used later
	int finished = 0;
	int stat = 0;
	int curr_process = 0;
	int next_process;
	int sig;
	struct timeval t0;
	struct timeval t1;
	long time;
	// Start the first process in the array again
	kill(pid_array[0], SIGCONT);
	gettimeofday(&t0, 0);
	printf("Started process %d with pid <%d>\n", 0, pid_array[0]);
	while (1){
		// If current process has not finished
		if (status[curr_process]==0){
			// Send SIGALRM to parent process after 3 seconds
			alarm(3);
			// Wait for a signal to be recieved by the sigset
			if (sigwait(&sigset, &sig)<0){
				fprintf(stderr, "Error waiting for signal from <%d> in parent process.\n", pid_array[curr_process]);
			}
			// Getting the current time to be passed into top function
			gettimeofday(&t1, 0);
			time = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
			// Calling top on the current process
			handle_top(curr_process, pid_array[curr_process], time);
			// This signal is sent if the process fails to execute execvp(), the user-defined signal is sent to the parent process to indicate that this process has failed
			if (sig == SIGUSR1){
				status[curr_process] = 1;
				printf("Process %d with pid <%d> executed unsuccesfully!\n", curr_process, pid_array[curr_process]);
			}
			else {
				// Stop the child process
				if (kill(pid_array[curr_process], SIGSTOP)<0){
					fprintf(stderr, "Error trying to stop child process <%d>.\n", pid_array[curr_process]);
				}
				else{
					printf("Stopped process %d with pid <%d>\n", curr_process, pid_array[curr_process]);
				}
				// Get pid status of child process, flags indicate there is no waiting for termination
				if (waitpid(pid_array[curr_process], &stat, WNOHANG | WUNTRACED | WCONTINUED)==-1){
					fprintf(stderr, "Error calling waitpid() on child process <%d>!\n", pid_array[curr_process]);
				}
				else {
					// Determine exit status of child process
					unsigned int eval = WIFEXITED(stat);
					// This status indicates if the process executed successfully
					if (eval>0){
						printf("Child process %d with pid <%d> executed successfully!\n", curr_process, pid_array[curr_process]);
						// Change pid status to 1 if correct execution, else 0
						status[curr_process] = eval;
					}
				}
			}
		}
		// Reset the alarm
		reset_alarm = 0;
		// Change current process to be new pid, circular wait
		if ((curr_process+1)==line_number){
			next_process = 0;
		}
		else {
			next_process = curr_process+1;
		}
		// Check if next process hasn't finished and start it if it hasn't
		if (status[next_process] == 0){
			if (kill(pid_array[next_process], SIGCONT)<0){
				fprintf(stderr, "Error trying to start child process <%d>.\n", pid_array[next_process]);
			}
			else{
				gettimeofday(&t0, 0);
				printf("Started process %d with pid <%d>\n", next_process, pid_array[next_process]);
			}
		}
		// Update current process to next process
		curr_process = next_process;
		// Change finished count for number of processes terminated
		for (int i = 0; i<line_number; i++){
			if (status[i] == 1){
				finished++;
			}
		}
		// If all finished, break from while loop
		if (finished == line_number){
			break;
		}
		finished = 0;
	}
	// Kill all child processes, they should have already exited but this handles edge cases
	signaler(pid_array, line_number, SIGINT);
	// Print all processes finished with their status
	for (int i=0; i<line_number; i++){
		printf("Process <%d> finished with status <%d>\n", pid_array[i],status[i]);
	}
	printf("All processes finished successfully!\n");
	return 0;
}

// Signaler function to send given signal to all processes in the array
void signaler(pid_t* pid_ary, int size, int signal)
{
	for(int i = 0; i < size; i++)
	{	
		pid_t child_pid = pid_ary[i];
		kill(child_pid, signal);
	}
}

// Used in previous project, counts the number of tokens separated by a delimiter
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

// Parses the string by tokens and returns created command_line variable data type
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

// Frees command line variable
void free_command_line(char** command_list)
{
	for (int i = 0; command_list[i]!=NULL; i++){
		free(command_list[i]);
	}
	free(command_list);
}
