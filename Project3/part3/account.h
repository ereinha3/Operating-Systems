#ifndef ACCOUNT_H_
#define ACCOUNT_H_
#include<pthread.h>

typedef struct
{
	char account_number[17];
	char password[9];
	double balance;
	double reward_rate;
	double transaction_tracter;
	char out_file[64];
	pthread_mutex_t ac_lock;
}account;

#endif /* ACCOUNT_H_ */
