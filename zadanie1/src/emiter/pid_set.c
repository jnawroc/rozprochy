#include "pid_set.h"

pid_set* init_set(){
	int i;
	pid_set* set = malloc(sizeof(pid_set));
	for(i=0;i<MAX_CLIENTS;i++){
		set->set[i]=0;
	}
	set->length=0;

	return set;

}

int get(pid_set* set){
	if(set->length<=0)
		return -1;
	return set->set[--set->length];
}


int contains(pid_set* set,pid_t i){
	int j;
	for(j=0;j<set->length;j++)
		if (set->set[j]==i) return 1;
	return 0;
}

int set_add(pid_set* set,pid_t i){
	if (contains(set,i)) return 1;
	if (set->length>=MAX_CLIENTS) return 0;
	set->set[set->length++]=i;
	return 1;
}

int set_remove(pid_set* set,pid_t i){
	int j;
	if (!contains(set,i)) return 0;
	for (j=0;(set->set[j]!=i);j++);
	set->set[j]=set->set[--set->length];
	return 1;
}


