#ifndef FUNCT_H
#define FUNCT_H
#define MAX_STATS 16

void fun_init();
int fun_add(void (*fun)(),int);
int fun_execute();
 
int interval_gcd;
#endif
