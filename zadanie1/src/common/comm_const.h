#ifndef STALE_H
#define STALE_H

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#define port 8899

enum headers {LOAD=0,USERS, PROC, MEMORY, UPTIME, CONNECTION_START, 
	CONFIGURATION_START, CONFIGURATION_END, CONNECTION_END};

#define PADS 1

#define PANEL 0

#define MSG_LEN 1024



typedef struct msg_h{
	uint32_t type;
	unsigned char data;
} msg_header;







int send_head(int,int,unsigned char);

int send_msg(int ,char* );

int send_msg_vl(int des,char* name,int length);

#endif
