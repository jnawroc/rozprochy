#ifndef PID_SET_H
#define PID_SET_H

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_CLIENTS 128


typedef struct p_set{
	pid_t set[MAX_CLIENTS];
	int length;
} pid_set;


pid_set* init_set();

int get(pid_set* set);

int contains(pid_set* set,pid_t i);

int set_add(pid_set* set,pid_t i);

int set_remove(pid_set* set,pid_t i);

#endif
