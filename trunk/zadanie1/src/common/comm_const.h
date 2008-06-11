#ifndef STALE_H
#define STALE_H

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

//#define port 9999

#define MAX 1024

enum headers {LOAD=0, PROC, MEMORY,  CPU_NAME,
	CONFIGURATION_START, CONFIGURATION_END,CONNECTION_START, CONNECTION_END};

const static char* head_names[] = {"obciążenie systemu","procesy","pamięć","nazwa procesora",
		"początek konfiguracji","koniec konfiguracji","początek połączenia",
		"koniec połączenia"};

#define UDP_MSG_LEN 64

#define PADS 1

#define PANEL 0

#define MSG_LEN 1024

#define MAX_EMITERS 32


typedef struct msg_h{
	uint32_t type;
	unsigned char data;
} msg_header;

typedef struct _UDP_msg{
	uint32_t type;
	char data[UDP_MSG_LEN] ;
} UDP_msg; 

struct uint32_16{
	uint16_t u16;
	uint32_t u32;
};
int send_head(int,uint32_t,unsigned char);

int send_msg(int ,char* );

int send_msg_vl(int des,char* name,int length);

#endif
