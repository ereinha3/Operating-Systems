#include"account.h"
#include"string_parser.h"
#include"string_parser.c"
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<limits.h>
#include<sys/mman.h>
#include<sys/wait.h>
#define NUM_ACCTS 10

// Declaring account and account number array
account *acts;
char *act_nums[NUM_ACCTS];
// Threads for processing bank transactions and array to keep track of them
pthread_t tids[NUM_ACCTS];
pthread_t bank_thread;
int ids[NUM_ACCTS];
// Command_line array to be used later
command_line* commands;
int line_count;
int line_interval;
// Only first barrier was used but declaration here
pthread_barrier_t barrier;
pthread_barrier_t barrier2;
int transaction_count = 0;
int threads_alive = NUM_ACCTS;
int waiting = 0;
int positions[NUM_ACCTS];
// Mutex lock initilizations, 6 were used
pthread_mutex_t trans_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t alive_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t time_to_update = PTHREAD_COND_INITIALIZER;
pthread_cond_t updated = PTHREAD_COND_INITIALIZER;
int num_updates = 0;
int bank_thread_waiting = 0;
int bank_thread_locked = 0;
// Child and parent pid for saving bank process and regular bank process
pid_t ppid;
pid_t cpid;

// Function to return a copy of the next line from a file pointer
char* line_return(FILE* fp){
	char* b = NULL;
	size_t size;
	getline(&b, &size, fp);
	char* saveptr;
	strtok_r(b, "\n", &saveptr);
	char* cpy = strdup(b);
	free(b);
	return cpy;
}

// Function to return a boolean if password matches
int validate(int index, char* pword){
	return (!strcmp(acts[index].password, pword));
}

// Function to get the index of a specific account based on the inputted account number
// Index corrsponds to the position in the account number array
int get_index(char* num){
	int index = -1;
	for (int i = 0; i<NUM_ACCTS; i++){
		if (!strcmp(num, act_nums[i])){
			index = i;
			break;
		}
	}
	if (index == -1){
		printf("Invalid Account Number: %s at line number\n", num);
	}
	return index;
}

// Function to return the total number of lines in a file
int get_line_count(char* filename){
	FILE *fp;
	fp = fopen(filename, "r");
	char* buf = NULL;
	size_t size;
	int count = 0;
	for (int i = 0; i<51; i++){
		getline(&buf, &size, fp);
	}
	while (getline(&buf, &size, fp)!=-1){
		count += 1;
	}
	fclose(fp);
	free(buf);
	return count;
}
		
// Function to process a set of transactions
// This is the function passed into each of the threads to handle the workload
void* process_transaction(void* arg){
	// Signal to the barrier that another thread is waiting
	pthread_barrier_wait(&barrier);
	// Detemine position to start reading transactions from
	int position = positions[*(int*)arg];
	int curr_line = position*line_interval;
	//printf("Currently in thread %d, starting at line number %d\n", position, curr_line);
	// Looping over the number of transactions to be processed
	// This number is the total number of lines divided by the number of threads created
	for (int i = 0; i<line_interval; i++){
		// Read from command_line array for corresponding command
		command_line buffer = commands[i+curr_line];
		//printf("On line number %d\n", i+curr_line+51);
		//fflush(stdout);
		// Number of tokens must be at least 3 (2 real ending with NULL token)
		if (buffer.num_token>=3){
			// Initializing variables for elements of command
			char * com = strdup(buffer.command_list[0]);
			char * acc_num = strdup(buffer.command_list[1]);
			char * p_word = strdup(buffer.command_list[2]);
			int truth = 0;
			// If any of these are NULL, reading the command failed for some reason
			if (com==NULL || acc_num == NULL || p_word == NULL){
				printf("Failed! with start line %d at %d in thread %d\n", curr_line, i+curr_line, position);
				truth = 0;
			}
			//printf("%s\n", acc_num);
			// Get the index of the account
			int index = -1;
			for (int i = 0; i<NUM_ACCTS; i++){
				if (!strcmp(acc_num, act_nums[i])){
					index = i;
					break;
				}
			}
			// Error handling if wrong index
			if (index == -1){
				printf("Invalid Account Number: %s at line number: %d\n", acc_num, i+curr_line);
			}
			// Otherwise, make sure the account number and password match
			else {
				truth = validate(index, p_word);
			}
			// Getting the lock for the given account
			pthread_mutex_t* lock = &acts[index].ac_lock;
			if (truth==1){
				int valid_transaction = 0;
				// If the command is check balance
				if (!strcmp(com, "C")){
					//printf("Current Balance for account %s:\t%f\n", acc_num,acts[index].balance);
					//printf("%s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2]);
				}
				// If the command is withdrawal
				else if (!strcmp(com, "W")){
					//printf("%s %s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2], buffer.command_list[3]);
					// Parse the amount to be withdrawn
					double amount = (double)atof(buffer.command_list[3]);
					// Lock, modify account information, unlock
					pthread_mutex_lock(lock);
					acts[index].balance -= amount;
					acts[index].transaction_tracter += amount;
					pthread_mutex_unlock(lock);
					// Setting such that transaction is valid
					valid_transaction = 1;
					//printf("Withdrew %f from account %s\n",amount, acc_num);
				}

				else if (!strcmp(com, "D")){
					//printf("%s %s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2], buffer.command_list[3]);
					// Parse the amount to be deposited
					double amount = (double)atof(buffer.command_list[3]);
					// Lock, modify account information, unlock
					pthread_mutex_lock(lock);
					acts[index].balance += amount;
					acts[index].transaction_tracter += amount;
					pthread_mutex_unlock(lock);
					// Setting such that transaction is valid
					valid_transaction = 1;
					//printf("Deposited %f in account %s\n",amount, acc_num);
				}
				else if (!strcmp(com, "T")){
					//printf("%s %s %s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2], buffer.command_list[3], buffer.command_list[4]);
					// Get the index of the command to recieve the money
					int new_index = get_index(buffer.command_list[3]);
					// Parse the amount to be deposited
					double money = (double)atof(buffer.command_list[4]); 
					// Lock, modify sender account information, unlock
					pthread_mutex_lock(lock);
					acts[index].balance -= money;
					acts[index].transaction_tracter += money;
					// Get reciever account lock
					pthread_mutex_unlock(lock);
					pthread_mutex_t* new_lock = &acts[new_index].ac_lock;
					// Lock, modify reciever account information, unlock
					pthread_mutex_lock(new_lock);
					acts[new_index].balance += money;
					pthread_mutex_unlock(new_lock);
					// Setting such that transaction is valid
					valid_transaction = 1;
					//printf("Transfered %f from account %s to account %s\n",money, acts[index].account_number, acts[new_index].account_number);
				}
				// If the transaction was valid
				if (valid_transaction == 1){
					// Increase count for total number of transactions processed
					pthread_mutex_lock(&trans_lock);
					transaction_count ++;
					//printf("Transaction count increased for thread %d.\n", position);
					pthread_mutex_unlock(&trans_lock);
				}
				// If transaction count is greater than or equal to 5000
				if (transaction_count >= 5000){
					// Increment the number of threads that are waiting
					pthread_mutex_lock(&wait_lock);
					//printf("Waiting thread count increased.\n");
					waiting++;
					//printf("waiting %d, alive %d in thread %d\n", waiting, threads_alive, position);
					sched_yield();
					// If all alive threads are waiting
					if (threads_alive == waiting){
						//printf("Locking update lock in thread %d...\n", position);
						// Wait until the bank thread is actively waiting
						while (bank_thread_waiting == 0){
							continue;
						}
						// Signal to update now 
						pthread_mutex_lock(&update_lock);
						//printf("Signaling to update...\n");
						//printf("Condition validated, update signaled...\n");
						while (bank_thread_waiting == 0){
							continue;
						}
						pthread_cond_broadcast(&time_to_update);
						//printf("Unlocking update lock in thread %d...\n", position);
						pthread_mutex_unlock(&update_lock);
					}
					//printf("Waiting for updated signal in thread %d\n", position);
					// Receive signal that account have been update and continue
					pthread_cond_wait(&updated, &wait_lock);
					//printf("Resumed thread %d\n", position);
					pthread_mutex_unlock(&wait_lock);
				}

			}
			// Free variables used
			free(com);
			free(acc_num);
			free(p_word);
		}
		// If none of that worked, password did not match or it was a faulty transaction
		else{
			//printf("Password did not match or faulty transaction!\n");
		}
	}
	// Decrement thread count
	pthread_mutex_lock(&alive_thread_lock);
	threads_alive--;
	//printf("Thread %d has terminated. Decreasing alive count by 1. Now %d threads alive.\n", position, threads_alive);
	sched_yield();
	// If this is the last thread to terminate or this somehow escaped the waiting decrementation
	if (threads_alive == 0 || threads_alive == waiting){
		// Wait for bank thread to be waiting
		while (bank_thread_waiting == 0){
			continue;
		}
		// Signal for final update
		pthread_mutex_lock(&update_lock);
		//printf("Signaling for final update.\n");
		printf("cpid: %d in thread %d\n", cpid, position);
		// Signal for savings process to end
		while (bank_thread_waiting != 1){
			continue;
		}
		kill(cpid, SIGUSR2);
		pthread_cond_signal(&time_to_update);
		//update_time = 1;
		pthread_mutex_unlock(&update_lock);
	}
	// Relock alive thread lock
	pthread_mutex_unlock(&alive_thread_lock);
	//printf("Now exiting thread %d...\n", position);
	// Exit from thread
	pthread_exit(NULL);
	return arg;
}

void* update_balance(void* arg){
	// Signaling another thread is waiting to the barrier
	pthread_barrier_wait(&barrier);
	int *count = (int*) arg;
	//printf("count = %d\n", *count);
	while(1){
		// Signaling the bank thread is waiting and waiting for 5000 transactions to be reached
		//printf("Locking update lock in bank thread...\n");
		pthread_mutex_lock(&update_lock);
		//printf("Updating balanced based on reward rates...\n");
		bank_thread_waiting = 1;
		pthread_cond_wait(&time_to_update, &update_lock);
		//printf("Unlocking update lock in bank thread...\n");
		pthread_mutex_unlock(&update_lock);

		//printf("Locking wait lock in bank thread...\n");
		pthread_mutex_lock(&wait_lock);
		//printf("Locked wait lock in bank thread...\n");
		
		// Update information in each of the accounts and write to their corresponsing output file
		for (int i = 0; i<NUM_ACCTS; i++){
			acts[i].balance += acts[i].reward_rate*acts[i].transaction_tracter;
			acts[i].transaction_tracter = 0;
			//printf("Balance updated in account %s.\n", acts[i].account_number);
			FILE* fp;
			fp = fopen(acts[i].out_file, "a");
			char* string = malloc(sizeof(char)*124);
			sprintf(string, "Current Balance:\t%.2f\n", acts[i].balance);
			fprintf(fp, string);
			free(string);
			fclose(fp);
		}
		// Increment number of updates
		*count += 1;
		printf("cpid: %d in bank thread\n", cpid);
		kill(cpid, SIGUSR1);
		//pthread_mutex_lock(&trans_lock);
		printf("Resetting transaction count...\n");
		transaction_count = 0;
		//pthread_mutex_unlock(&trans_lock);
		waiting = 0;
		//printf("Waiting thread and transaction counts reset.\n");
		// Declare the bank thread is now not waiting
		bank_thread_waiting = 0;
		// Broadcast accounts have been updated
		pthread_cond_broadcast(&updated);
		//printf("Unlocking wait lock in bank thread...\n");
		pthread_mutex_unlock(&wait_lock);
		//printf("Unlocked wait lock in bank thread...\n");
		// If there are no more threads alive, break
		if (threads_alive == 0){
			break;
		}
		sched_yield();
	}


	pthread_exit(NULL);
	return count;
}

// Declare that a given signal has been recieved
void signal_recieved(int sig){
	printf("Recieved signal %d\n", sig);
}

int main(int argc, char* argv[]){
	// Ensure proper usage
	if (argc<2){
		fprintf(stderr, "Usage: ./bank file.txt\n");
		exit(-1);
	}
	// Get filename from standard input
	char *filename = NULL;
	filename = argv[1];
	// Get the total line count of the file
	line_count = get_line_count(filename);
	char* buf = NULL;
	FILE *fp;
	// Open the file
	fp = fopen(filename, "r");
	// If opening fails
	if (!fp){
		printf("File open failed!\n");
	}
	char cwd[124];
	// Get the current directory
	getcwd(cwd, sizeof(cwd));
	char output_folder[PATH_MAX];
	// Create new directory by concatenating current directory name with new directory name
	sprintf(output_folder, "%s/outputs", cwd);
	// Make new directory
	mkdir(output_folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	size_t size = 64;
	getline(&buf, &size, fp);
	//int num_accounts = atoi(buf);
	// Allocate memory for the number of accounts
	acts = malloc(sizeof(account)*10);
	for (int i = 0; i<NUM_ACCTS; i++){
		// Parse information from file, store it in the corresponding variable in the account struct
		account act;
		getline(&buf, &size, fp);
		char* ptr;
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		strcpy(act.account_number, buf);
		act_nums[i] = strdup(buf);
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		strcpy(act.password, buf);
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		act.balance = (double)atof(buf);
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		act.reward_rate = (double)atof(buf);
		// This is creating the individual files in the output folder
		char string[sizeof(output_folder)+20];
		sprintf(string,"%s/account%d.txt", output_folder, i);
		strcpy(act.out_file, string);
		FILE *fpp;
		// Opening the output file and writing the account number to it
		fpp = fopen(act.out_file, "w");
		char new_string[124];
		sprintf(new_string, "account %d:\n", i);
		fprintf(fpp, new_string);
		fclose(fpp);
		act.transaction_tracter = 0;
		// Initiliazing mutex lock for the account and storing it
		pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
		act.ac_lock = lock;
		acts[i] = act;
	}




	//printf("Accounts created!\n");
	// Allocate space for the total number of commands 
	commands = malloc(sizeof(command_line)*line_count);
	for (int i = 0; i<line_count; i++){
		// If end is reached
		if (getline(&buf, &size, fp)==-1){
			break;
		}
		// Otherwise, parse command into command_line variable
		else{
			command_line tokens = str_filler(buf, " ");
			commands[i] = tokens;
		}
	}
	// Free the buffer
	free(buf);
	// Close the file
	fclose(fp);
	// Initializer the barrier that all threads will call before running
	pthread_barrier_init(&barrier, NULL, NUM_ACCTS+2);

	//printf("Barrier created!\n");
	// Create the bank thread
	pthread_create(&bank_thread, NULL, &update_balance, &num_updates);

	//printf("Bank thread created!\n");
	// This variable stores the number of lines that will be passed into each thread
	line_interval = line_count/NUM_ACCTS;
	//int current = 0;
	// Memory mapping the double accounts array to global memory so the saving thread can use it
	double* saved_balances = mmap(NULL, sizeof(double)*NUM_ACCTS, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// Create the threads to process chunks of the file based on number specified
	for (int i = 0; i<NUM_ACCTS; i++){
		positions[i] = i;
		pthread_create(&tids[i], NULL, &process_transaction, &positions[i]);
		// Store 20% of initial balance in savings bank
		printf("Original Balance in account %d: %.2f\n", i, acts[i].balance);
		double new_balance = (double)(acts[i].balance * 0.2);
		printf("New Balance in account %d: %.2f\n", i, new_balance);
		saved_balances[i] = new_balance;
		//current += line_interval;
	}
	// Get parents pid
	ppid = getpid();
	cpid = 1;
	// Declare and initialize sigset
	sigset_t sigset;
	sigemptyset(&sigset);
	// Add user defined signals
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	signal(SIGUSR1, signal_recieved);
	signal(SIGUSR2, signal_recieved);

	// Fork to get parent and child process
	if (cpid>0){
		cpid = fork();
		printf("Parent thread --- cpid: %d, ppid: %d\n", cpid, ppid);
	}

	// If this is the child process
	if (cpid == 0){
		printf("Child thread --- cpid: %d, ppid: %d\n", cpid, ppid);
		// Create savings folder
		char savings_folder[PATH_MAX];
		sprintf(savings_folder, "%s/savings", cwd);
		int sucess = mkdir(savings_folder, 0777);
		if (sucess==-1){
			printf("Directory creation failed\n");
		}
		// Create an array of output files for the savings accounts
		double balances[NUM_ACCTS];
		char** output_files = malloc(sizeof(char*)*NUM_ACCTS);

		// Iterate over the number of savings accounts and created output files in the savings output directory
		for (int i = 0; i<NUM_ACCTS; i++){
			balances[i] = saved_balances[i];
			FILE *fpp;
			// Create savings file
			char* savings_file = malloc(sizeof(char)*124);
			sprintf(savings_file,"%s/account%d.txt", savings_folder, i);
			fpp = fopen(savings_file, "w");
			// If opening fails
			if (fpp == NULL) {
				printf("Failed to open file\n");
			}
			// Write output
			fprintf(fpp, "account ");
			char num[sizeof(int)*8+1];
			sprintf(num, "%d", i);
			fprintf(fpp, num);
			fprintf(fpp, ":\n");
			fclose(fpp);
			output_files[i] = savings_file;
		}

		// Send user defined signal that savings account have been created
		kill(ppid, SIGUSR1);


		int sig;

		while (1){
			// Wait for a signal
			sigwait(&sigset, &sig);
			printf("Signal recieved in child process\n");
			// Update everything based on reward rate
			for (int i = 0; i<NUM_ACCTS; i++){
				FILE *fp = fopen(output_files[i], "a");
				balances[i] *= 1.02;
				char savings_string[124];
				sprintf(savings_string, "Current Balance:\t%.2f\n", balances[i]);
				fprintf(fp, savings_string);
				fclose(fp);
			}
			printf("Child accounts updated\n");
			if (sig == SIGUSR2){
				break;
			}
		}

		// Print all savings account information to the savings output file
		FILE *fp;
		fp = fopen("savings_output.txt", "w");
		for (int i = 0; i<NUM_ACCTS; i++){
			char act_string[PATH_MAX];
			sprintf(act_string,"%d balance:\t%.2f\n\n", i, balances[i]);
			fprintf(fp, act_string);
			free(output_files[i]);
		}
		// Close file and free created strings
		fclose(fp);
		printf("Child accounts updated\n");
		printf("Exiting child process\n");
		free(output_files);
	}

	// If this is the parent
	if (cpid!=0){
		int sig;
		// Wait for signal from savings thread that all savings accounts have been created
		sigwait(&sigset, &sig);
		// Signal to the barrier that another thread is waiting (this should be the last and should start all other threads)
		pthread_barrier_wait(&barrier);

		//printf("Worker threads now beginning execution.\n");
		
		// Wait for all threads to finish
		for (int i= 0; i<NUM_ACCTS; i++){
			pthread_join(tids[i], NULL);
			//printf("Thread %d has now terminated.\n", i);
		}
		// Wait for bank thread to finish
		pthread_join(bank_thread, NULL);
		printf("Bank thread has now terminated.\n");

		int status = 0;
		// Wait for the child process to finish
		while (waitpid(cpid, &status, WNOHANG)){
			// If status determines it has finished, break
			if (WIFEXITED(status)>0){
				break;
			}
		}
		// Open the output file
		fp = fopen("output.txt", "w");
		// Print account balance to outputs
		for (int i = 0; i<NUM_ACCTS; i++){
			char string[PATH_MAX];
			sprintf(string,"%d balance:\t%.2f\n\n", i, acts[i].balance);
			fprintf(fp, string);
		}
		// Close file
		fclose(fp);
	}

	// Destroy the barrier to not leak memory
	pthread_barrier_destroy(&barrier);
	// Free all accounts numbers and destroy their mutex locks
	for (int i = 0; i<NUM_ACCTS; i++){
		free(act_nums[i]);
		pthread_mutex_destroy(&acts[i].ac_lock);
	}
	// Free every command_line allocated
	for (int i = 0; i<line_count; i++){
		free_command_line(&commands[i]);
	}
	// Free array of acts and commands
	free(acts);
	free(commands);
	// Destroy all created mutex locks
	pthread_mutex_destroy(&trans_lock);
	pthread_mutex_destroy(&update_lock);
	pthread_mutex_destroy(&wait_lock);
	pthread_mutex_destroy(&alive_thread_lock);
	// Unmap the shared global memory to free it
	munmap(&saved_balances, sizeof(double)*10);
	return 0;
}
