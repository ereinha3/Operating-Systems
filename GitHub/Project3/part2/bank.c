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
//pthread_cond_t conditions[NUM_ACCTS];
pthread_t tids[NUM_ACCTS];
pthread_t bank_thread;
void* status[NUM_ACCTS];
int ids[NUM_ACCTS];
int positions[NUM_ACCTS];
command_line* commands;
int line_count;
int line_interval;
int threads_alive = NUM_ACCTS;
pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t terminated = PTHREAD_COND_INITIALIZER;

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
		printf("Invalid Account Number: %s\n", num);
	}
	return index;
}
		

void* process_transaction(void* arg){
	int position = *(int*)arg;
	int start_line = position * line_interval;
	for (int i = 0; i<(line_interval-1); i++){
		int curr_line = start_line + i;
		//printf("%d in thread %d \n", curr_line, position);
		//fflush(stdout);
		command_line buffer = commands[curr_line];
		char * com = strdup(buffer.command_list[0]);
		char * acc_num = strdup(buffer.command_list[1]);
		char * p_word = strdup(buffer.command_list[2]);
		char* ptr;
		int index = get_index(acc_num);
		int truth = validate(index, p_word);
		//pthread_cond_t cond = conditions[index];
		pthread_mutex_t *lock = &acts[index].ac_lock;
		if (truth==1){
			if (!strcmp(com, "C")){
				FILE* fp;
				fp = fopen(acts[index].out_file, "a");
				char* string = malloc(sizeof(char)*124);
				sprintf(string, "Current Balance:\t%f\n", acts[index].balance);
				fprintf(fp, string);
				free(string);
				fclose(fp);
			}
			else if (!strcmp(com, "W")){
				double amount = strtod(buffer.command_list[3], &ptr);
				pthread_mutex_lock(lock);
				//pthread_cond_wait(&cond, &lock); 
				acts[index].balance -= amount;
				acts[index].transaction_tracter += amount;
				//pthread_cond_signal(&cond);
				pthread_mutex_unlock(lock);
			}
			else if (!strcmp(com, "D")){
				double amount = strtod(buffer.command_list[3], &ptr);
				pthread_mutex_lock(lock);
				//pthread_cond_wait(&cond, &lock); 
				acts[index].balance += amount;
				acts[index].transaction_tracter += amount;
				//pthread_cond_signal(&cond);
				pthread_mutex_unlock(lock);
			}
			else if (!strcmp(com, "T")){
				int new_index = get_index(buffer.command_list[3]);
				double money = strtod(buffer.command_list[4], &ptr); 
				pthread_mutex_lock(lock);
				//pthread_cond_wait(&cond, &lock); 
				acts[index].balance -= money;
				acts[index].transaction_tracter += money;
				//pthread_cond_signal(&cond);
				pthread_mutex_unlock(lock);
				//pthread_cond_t new_cond = conditions[new_index];
				pthread_mutex_t* new_lock = &acts[new_index].ac_lock;
				pthread_mutex_lock(new_lock);
				//pthread_cond_wait(&new_cond, &new_lock); 
				acts[new_index].balance += money;
				//pthread_cond_signal(&new_cond);
				pthread_mutex_unlock(new_lock);
			}
		}

		free(com);
		free(acc_num);
		free(p_word);
	
	}
	pthread_mutex_lock(&thread_lock);
	threads_alive--;
	if (threads_alive == 0){
		pthread_mutex_lock(&update_lock);
		pthread_cond_signal(&terminated);
		pthread_mutex_unlock(&update_lock);
	}
	pthread_mutex_unlock(&thread_lock);
	printf("Thread %d has terminated\n", position);
	return arg;
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

void* update_balance(void *arg){
	int *count = (int*)arg;
	*count = 0;
	pthread_mutex_lock(&update_lock);
	pthread_cond_wait(&terminated, &update_lock);
	printf("Updating balances...\n");
	for (int i = 0; i<NUM_ACCTS; i++){
		pthread_mutex_lock(&acts[i].ac_lock);
		acts[i].balance += acts[i].reward_rate*acts[i].transaction_tracter;
		acts[i].transaction_tracter = 0;
		pthread_mutex_unlock(&acts[i].ac_lock);
		*count += 1;
	}
	pthread_mutex_unlock(&update_lock);
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
		printf("FAILED!!!\n");
	}
	char cwd[124];
	getcwd(cwd, sizeof(cwd));
	char output_folder[PATH_MAX];
	sprintf(output_folder, "%s/outputs", cwd);
	mkdir(output_folder, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	size_t size = 64;
	getline(&buf, &size, fp);
	int num_accounts = atoi(buf);
	acts = malloc(sizeof(account)*10);
	for (int i = 0; i<num_accounts; i++){
		account act;
		getline(&buf, &size, fp);
		char* ptr;
		char* ptr2;
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		strcpy(act.account_number, buf);
		act_nums[i] = strdup(buf);
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		strcpy(act.password, buf);
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		act.balance = strtod(buf, &ptr2);
		getline(&buf, &size, fp);
		strtok_r(buf, "\n", &ptr);
		act.reward_rate = strtod(buf, &ptr2);
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
		pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;
		act.ac_lock = mutex;
		acts[i] = act;
	}
	//char* ptr;
	commands = malloc(sizeof(command_line)*line_count);
	for (int i = 0; i<line_count; i++){
		if (getline(&buf, &size, fp)==-1){
			break;
		}
		else{
			//char* line = strtok_r(buf, "\n", &ptr);
			command_line tokens = str_filler(buf, " ");
			commands[i] = tokens;
			//printf("%s, %d\n", buf, tokens.num_token);
		}
	}
	fclose(fp);
	line_interval = line_count/NUM_ACCTS;
	for (int i = 0; i<NUM_ACCTS; i++){
		positions[i] = i;
		pthread_create(&tids[i], NULL, &process_transaction, &positions[i]);
		//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
		//conditions[i] = cond;
	}
	int count;
	pthread_create(&bank_thread, NULL, &update_balance, &count);
	void * bank_status;
	for (int i = 0; i<NUM_ACCTS; i++){
		pthread_join(tids[i], &status[i]);
		//printf("Thread %d has exited.\n", i);
	}
	pthread_join(bank_thread, &bank_status);
	printf("Bank thread has exited\n");

	fp = fopen("output.txt", "w");
	for (int i = 0; i<NUM_ACCTS; i++){
		char string[PATH_MAX];
		sprintf(string,"%d balance:\t%.2f\n\n", i, acts[i].balance);
		fprintf(fp, string);
	}
	fclose(fp);
	free(buf);
	for (int i = 0; i<NUM_ACCTS; i++){
		free(act_nums[i]);
	}
	for (int i = 0; i<line_count; i++){
		free_command_line(&commands[i]);
	}
	free(acts);
	free(commands);
	return 0;
}
