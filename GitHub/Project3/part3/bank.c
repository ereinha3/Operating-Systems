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
#define NUM_ACCTS 10

account *acts;
char *act_nums[NUM_ACCTS];
pthread_t tids[NUM_ACCTS];
pthread_t bank_thread;
int ids[NUM_ACCTS];
command_line* commands;
int line_count;
int line_interval;
pthread_barrier_t barrier;
pthread_barrier_t barrier2;
int transaction_count = 0;
int threads_alive = NUM_ACCTS;
int waiting = 0;
int positions[NUM_ACCTS];
pthread_mutex_t trans_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t alive_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t time_to_update = PTHREAD_COND_INITIALIZER;
pthread_cond_t updated = PTHREAD_COND_INITIALIZER;
int num_updates = 0;
int bank_thread_waiting = 0;
int bank_thread_locked = 0;

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

int validate(int index, char* pword){
	return (!strcmp(acts[index].password, pword));
}

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
		

void* process_transaction(void* arg){
	pthread_barrier_wait(&barrier);
	int position = positions[*(int*)arg];
	int curr_line = position*line_interval;
	//printf("Currently in thread %d, starting at line number %d\n", position, curr_line);
	for (int i = 0; i<line_interval; i++){
		command_line buffer = commands[i+curr_line];
		//printf("On line number %d\n", i+curr_line+51);
		//fflush(stdout);
		if (buffer.num_token>=3){
			char * com = strdup(buffer.command_list[0]);
			char * acc_num = strdup(buffer.command_list[1]);
			char * p_word = strdup(buffer.command_list[2]);
			int truth = 1;
			if (com==NULL || acc_num == NULL || p_word == NULL){
				printf("Failed! with start line %d at %d in thread %d\n", curr_line, i+curr_line, position);
				truth = 0;
			}
			//printf("%s\n", acc_num);
			int index = -1;
			for (int i = 0; i<NUM_ACCTS; i++){
				if (!strcmp(acc_num, act_nums[i])){
					index = i;
					break;
				}
			}
			if (index == -1){
				printf("Invalid Account Number: %s at line number: %d\n", acc_num, i+curr_line);
			}
			else {
				truth = validate(index, p_word);
			}

			pthread_mutex_t* lock = &acts[index].ac_lock;
			if (truth==1){
				int valid_transaction = 0;
				if (!strcmp(com, "C")){
					//printf("Current Balance for account %s:\t%f\n", acc_num,acts[index].balance);
					//printf("%s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2]);
				}

				else if (!strcmp(com, "W")){
					//printf("%s %s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2], buffer.command_list[3]);
					double amount = (double)atof(buffer.command_list[3]);
					pthread_mutex_lock(lock);
					acts[index].balance -= amount;
					acts[index].transaction_tracter += amount;
					pthread_mutex_unlock(lock);
					valid_transaction = 1;
					//printf("Withdrew %f from account %s\n",amount, acc_num);
				}

				else if (!strcmp(com, "D")){
					//printf("%s %s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2], buffer.command_list[3]);
					double amount = (double)atof(buffer.command_list[3]);
					pthread_mutex_lock(lock);
					acts[index].balance += amount;
					acts[index].transaction_tracter += amount;
					pthread_mutex_unlock(lock);
					valid_transaction = 1;
					//printf("Deposited %f in account %s\n",amount, acc_num);
				}
				else if (!strcmp(com, "T")){
					//printf("%s %s %s %s %s\n", buffer.command_list[0], buffer.command_list[1], buffer.command_list[2], buffer.command_list[3], buffer.command_list[4]);
					int new_index = get_index(buffer.command_list[3]);
					double money = (double)atof(buffer.command_list[4]); 
					pthread_mutex_lock(lock);
					acts[index].balance -= money;
					acts[index].transaction_tracter += money;
					pthread_mutex_unlock(lock);
					pthread_mutex_t* new_lock = &acts[new_index].ac_lock;
					pthread_mutex_lock(new_lock);
					acts[new_index].balance += money;
					pthread_mutex_unlock(new_lock);
					valid_transaction = 1;
					//printf("Transfered %f from account %s to account %s\n",money, acts[index].account_number, acts[new_index].account_number);
				}

				if (valid_transaction == 1){
					pthread_mutex_lock(&trans_lock);
					transaction_count ++;
					//printf("Transaction count increased for thread %d.\n", position);
					pthread_mutex_unlock(&trans_lock);
				}
				if (transaction_count >= 5000){
					pthread_mutex_lock(&wait_lock);
					//printf("Waiting thread count increased.\n");
					waiting++;
					//printf("waiting %d, alive %d in thread %d\n", waiting, threads_alive, position);
					sched_yield();
					if (threads_alive == waiting){
						//printf("Locking update lock in thread %d...\n", position);
						while (bank_thread_waiting == 0){
							continue;
						}
						pthread_mutex_lock(&update_lock);
						//printf("Signaling to update...\n");
						while (bank_thread_waiting == 0){
							continue;
						}
						//printf("Condition validated, update signaled...\n");
						pthread_cond_broadcast(&time_to_update);
						//printf("Unlocking update lock in thread %d...\n", position);
						pthread_mutex_unlock(&update_lock);
					}
					//printf("Waiting for updated signal in thread %d\n", position);
					pthread_cond_wait(&updated, &wait_lock);
					//printf("Resumed thread %d\n", position);
					pthread_mutex_unlock(&wait_lock);
				}

			}
			free(com);
			free(acc_num);
			free(p_word);
		}
		else{
			//printf("Password did not match or faulty transaction!\n");
		}
	}
	pthread_mutex_lock(&alive_thread_lock);
	threads_alive--;
	//printf("Thread %d has terminated. Decreasing alive count by 1. Now %d threads alive.\n", position, threads_alive);
	sched_yield();
	if (threads_alive == 0 || threads_alive == waiting){
		while (bank_thread_waiting == 0){
			continue;
		}
		pthread_mutex_lock(&update_lock);
		printf("Signaling for final update.\n");
		while (bank_thread_waiting != 1){
			continue;
		}
		pthread_cond_signal(&time_to_update);
		//update_time = 1;
		pthread_mutex_unlock(&update_lock);
	}
	pthread_mutex_unlock(&alive_thread_lock);
	//printf("Now exiting thread %d...\n", position);
	pthread_exit(NULL);
	return arg;
}

void* update_balance(void* arg){
	pthread_barrier_wait(&barrier);
	int *count = (int*) arg;
	//printf("count = %d\n", *count);
	while(1){
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
		*count += 1;
		//pthread_mutex_lock(&trans_lock);
		//printf("Resetting transaction count...\n");
		transaction_count = 0;
		//pthread_mutex_unlock(&trans_lock);
		waiting = 0;
		//printf("Waiting thread and transaction counts reset.\n");
		bank_thread_waiting = 0;
		pthread_cond_broadcast(&updated);
		//printf("Unlocking wait lock in bank thread...\n");
		pthread_mutex_unlock(&wait_lock);
		//printf("Unlocked wait lock in bank thread...\n");
		if (threads_alive == 0){
			break;
		}
		sched_yield();
	}


	pthread_exit(NULL);
	return count;
}

int main(int argc, char* argv[]){
	if (argc<2){
		fprintf(stderr, "Usage: ./bank file.txt\n");
		exit(-1);
	}
	char *filename = NULL;
	filename = argv[1];
	line_count = get_line_count(filename);
	char* buf = NULL;
	FILE *fp;
	fp = fopen(filename, "r");
	if (!fp){
		printf("File open failed!\n");
	}
	char cwd[124];
	getcwd(cwd, sizeof(cwd));
	char output_folder[PATH_MAX];
	sprintf(output_folder, "%s/outputs", cwd);
	mkdir(output_folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	size_t size = 64;
	getline(&buf, &size, fp);
	//int num_accounts = atoi(buf);
	acts = malloc(sizeof(account)*10);
	for (int i = 0; i<NUM_ACCTS; i++){
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
		char string[sizeof(output_folder)+20];
		sprintf(string,"%s/account%d.txt", output_folder, i);
		strcpy(act.out_file, string);
		FILE *fpp;
		fpp = fopen(act.out_file, "w");
		char new_string[124];
		sprintf(new_string, "account %d:\n", i);
		fprintf(fpp, new_string);
		fclose(fpp);
		act.transaction_tracter = 0;
		pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
		act.ac_lock = lock;
		acts[i] = act;
	}

	//printf("Accounts created!\n");

	commands = malloc(sizeof(command_line)*line_count);
	for (int i = 0; i<line_count; i++){
		if (getline(&buf, &size, fp)==-1){
			break;
		}
		else{
			command_line tokens = str_filler(buf, " ");
			commands[i] = tokens;
		}
	}

	fclose(fp);

	pthread_barrier_init(&barrier, NULL, NUM_ACCTS+2);

	//printf("Barrier created!\n");

	pthread_create(&bank_thread, NULL, &update_balance, &num_updates);

	//printf("Bank thread created!\n");

	line_interval = line_count/NUM_ACCTS;
	//int current = 0;
	for (int i = 0; i<NUM_ACCTS; i++){
		positions[i] = i;
		pthread_create(&tids[i], NULL, &process_transaction, &positions[i]);
		//current += line_interval;
	}
	pthread_barrier_wait(&barrier);

	//printf("Worker threads now beginning execution.\n");
		
	for (int i= 0; i<NUM_ACCTS; i++){
		pthread_join(tids[i], NULL);
		//printf("Thread %d has now terminated.\n", i);
	}
	pthread_join(bank_thread, NULL);
	//printf("Bank thread has now terminated.\n");

	fp = fopen("output.txt", "w");
	for (int i = 0; i<NUM_ACCTS; i++){
		char string[PATH_MAX];
		sprintf(string,"%d balance:\t%.2f\n\n", i, acts[i].balance);
		fprintf(fp, string);
	}
	pthread_barrier_destroy(&barrier);
	fclose(fp);
	free(buf);
	for (int i = 0; i<NUM_ACCTS; i++){
		free(act_nums[i]);
		pthread_mutex_destroy(&acts[i].ac_lock);
	}
	for (int i = 0; i<line_count; i++){
		free_command_line(&commands[i]);
	}
	free(acts);
	free(commands);
	pthread_mutex_destroy(&trans_lock);
	pthread_mutex_destroy(&update_lock);
	pthread_mutex_destroy(&wait_lock);
	pthread_mutex_destroy(&alive_thread_lock);
	pthread_exit(NULL);
	return 0;
}
