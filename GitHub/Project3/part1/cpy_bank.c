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
int num_updates;

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
	command_line commands = *(command_line*)arg;
	char** command = commands.command_list;
	char * com = strdup(command[0]);
	char * acc_num = strdup(command[1]);
	char * p_word = strdup(command[2]);
	int index = get_index(acc_num);
	int truth = validate(index, p_word);
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
			double withdrawal = (double)atof(command[3]);
			acts[index].balance -= withdrawal;
			acts[index].transaction_tracter += withdrawal;
		}
		else if (!strcmp(com, "D")){
			double deposit = (double)atof(command[3]);
			acts[index].balance += deposit;
			acts[index].transaction_tracter += deposit;
		}
		else if (!strcmp(com, "T")){
			int new_index = get_index(command[3]);
			double money = (double)atof(command[4]); 
			acts[index].balance -= money;
			acts[index].transaction_tracter += money;
			acts[new_index].balance += money;
			acts[new_index].transaction_tracter += money;
		}
	}
	free(com);
	free(acc_num);
	free(p_word);
	return arg;
}


void* update_balance(void){
	num_updates = 0;
	for (int i = 0; i<NUM_ACCTS; i++){
		/*
		account *act = &acts[i];
		if ((*act).transaction_tracter!=0){
			(*act).balance += (*act).reward_rate*(*act).transaction_tracter;
			(*act).transaction_tracter = 0;
		}
		*/
		account act = acts[i];
		if (act.transaction_tracter!=0){
			act.balance += act.reward_rate*act.transaction_tracter;
			act.transaction_tracter = 0;
		}
		acts[i] = act;
	}
	num_updates += 1;
	return &num_updates;
}

int main(int argc, char* argv[]){
	if (argc<2){
		fprintf(stderr, "Usage: ./bank file.txt\n");
		exit(-1);
	}
	FILE *fp;
	char* buf = NULL;
	fp = fopen(argv[1], "r");
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
		acts[i] = act;
	}
	for (int i = 0; i<NUM_ACCTS; i++){
		account act = acts[i];
		printf("Account %d:\nacct_num %s\npassword %s\nbalance %f\nreward_rate %f\noutput file %s\n", i, act.account_number, act.password, act.balance, act.reward_rate, act.out_file);
	}
	char* ptr;
	int count = 0;	
	while (getline(&buf, &size, fp)!=-1){
		command_line * tokens = malloc(sizeof(command_line));
		*tokens = str_filler(buf, " ");
		process_transaction(tokens);
		free_command_line(tokens);
		free(tokens);
		count ++;
	}
	printf("%d\n", count);
	free(buf);
	fclose(fp);
	update_balance();
	fp = fopen("output.txt", "w");
	for (int i = 0; i<num_accounts; i++){
		account act = acts[i];
		//printf("Account %d:\nacct_num %s\npassword %s\nbalance %f\nreward_rate %f\noutput file %s\n", i, act.account_number, act.password, act.balance, act.reward_rate, act.out_file);
		char string[PATH_MAX];
		sprintf(string,"%d balance:\t%.2f\n", i, act.balance);
		fprintf(fp, string);
	}
	fclose(fp);
	for (int i = 0; i<num_accounts; i++){
		free(act_nums[i]);
	}
	free(acts);
	return 0;
}
