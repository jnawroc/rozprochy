#ifndef _STATS_FUNCTS
#define _STATS_FUNCTS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>
#include "comm_const.h"
#include "stats_functs.h"
#include <time.h>

static char *last_ip=NULL;

static void ip_check(char* ip){
	if (last_ip){
		if (strcmp(last_ip , ip)){
			free (last_ip);
			last_ip = strdup(ip);
			printf("z adresu IP %s:\n",ip);

		}
		
	}else{
		last_ip = strdup(ip);
		printf("z adresu IP %s:\n",ip);
	
	}
}

void proc(char*ip,char* data){
	char timebuf[64];
	time_t t=time(NULL);
	ip_check(ip);
	strftime(timebuf,63,"%y-%m-%d %H:%M:%S",localtime(&t));
	printf("\t%s ",timebuf);
	uint32_t pr = ntohl(*(uint32_t*)data);
	printf("uruchomionych procesów: %d\n",pr);
}	


void init_stats_functs(){
	stat_func = malloc(sizeof(void*)*16);
	combined_func = malloc(sizeof(void*)*16);
	stat_func[PROC]=&proc;
}	  
#endif
