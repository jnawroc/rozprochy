#ifndef STALE_H
#define STALE_H

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

//#define port 9999

#define MAX 1024

#define LOAD 0

#define PROC 1

#define MEMORY 2

#define CPU_NAME 3

#define CONFIGURATION_END 5

#define CONNECTION_END 7

/*enum headers {LOAD=0, PROC=1, MEMORY=2,  CPU_NAME=3,
	CONFIGURATION_START=4, CONFIGURATION_END=5,CONNECTION_START=6, CONNECTION_END=7};
*/
const static char* head_names[] = {"obciążenie systemu","procesy","pamięć","nazwa procesora",
		"początek konfiguracji","koniec konfiguracji","początek połączenia",
		"koniec połączenia"};

#define UDP_MSG_LEN 60

#define PADS 1

#define PANEL 0

#define MSG_LEN 1024

#define MAX_EMITERS 32


typedef struct msg_h{
	char data;
	char nil[3];
	uint32_t type;
	
	
} msg_header;

typedef struct _UDP_msg{
	uint32_t type;
	char data[UDP_MSG_LEN] ;
} UDP_msg; 

struct uint32_16{
	uint16_t u16;
	uint32_t u32;
	char nil[2];
};
int send_head(int,uint32_t,unsigned char);

int send_msg(int ,char* );

int send_msg_vl(int des,char* name,int length);

char* adr_to_string(struct sockaddr_in adr);

#endif
