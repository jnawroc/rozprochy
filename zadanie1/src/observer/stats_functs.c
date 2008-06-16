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

void load(char*ip,char* dat){
	char timebuf[64];
	float data[3];
	memcpy(data, dat,sizeof(float)*3);
	time_t t=time(NULL);
	ip_check(ip);
	strftime(timebuf,63,"%y-%m-%d %H:%M:%S",localtime(&t));
	printf("\t%s ",timebuf);
	//uint32_t pr = ntohl(*(uint32_t*)data);
	float load15,load5,load1;
	load1 = data[0];
	
	load5 = data[1];
	
	load15 = data[2];
	printf("średnie obciążenie systemu 1min: %f, 5min: %f, 15min: %f\n",load1,load5,load15);
}

void memory(char*ip,char* data){
	uint32_t* p = data;
	char timebuf[64];
	time_t t=time(NULL);
	ip_check(ip);
	strftime(timebuf,63,"%y-%m-%d %H:%M:%S",localtime(&t));
	printf("\t%s ",timebuf);
	
	printf("pamięć całkowita:%d, używana: %d, wolna: %d\n",ntohl(p[0]),ntohl(p[1]),ntohl(p[2]));
	printf("\t%s ",timebuf);
	printf("pamięć swap całkowita:%d, używana: %d, wolna: %d\n",ntohl(p[3]),ntohl(p[4]),ntohl(p[5]));
}	

void cpu(char*ip,char* data){
	char timebuf[64];
	time_t t=time(NULL);
	ip_check(ip);
	strftime(timebuf,63,"%y-%m-%d %H:%M:%S",localtime(&t));
	printf("\t%s ",timebuf);
	printf("procesor: %d %s\n",data[0],data+1);
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
	stat_func[LOAD]=&load;
	stat_func[CPU_NAME] = &cpu;
	stat_func[MEMORY] = &memory;
}	  
#endif
