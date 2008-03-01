#define MAX_STATS 16
#include "funct.h"
#include <stdlib.h>

void (**functions)();
int* intervals;
int fun_count;
int initialized=0;
long long unsigned timestamp;
int interval_lcm=1;

int gcd(int a,int b){
	int c;
	while (b != 0){
		c = a % b;
		a = b;
		b = c;
	}
	return a;
}

int lcm(int a,int b){
	return a * b / gcd(a,b);
}



void fun_init(){
	functions = malloc(MAX_STATS * sizeof(void*));
	intervals = malloc(MAX_STATS * sizeof(int*));
	fun_count = 0;
	initialized = 1;
	interval_gcd = 1;
	timestamp = 0;
	
}

int fun_add(void (*fun)(),int interval){
	if (!initialized) return 0;
	if (MAX_STATS == fun_count) return 0;
	functions[fun_count] = fun;
	intervals[fun_count++] = interval;
	interval_lcm = lcm(interval_lcm,interval);
	interval_gcd = gcd(interval_gcd,interval);
	return 1;
}

int fun_execute(){
	if (!initialized) return 0;
	int i;
	for(i=0;i<fun_count;i++){
		if (!(timestamp%intervals[i])){
			functions[i]();
		}
	}
	timestamp+=interval_gcd;
	return 1;
}





