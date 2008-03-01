#ifndef STALE_H
#define STALE_H

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#define port 8899

#define SET_WIDTH 4

#define LOAD 8

#define MONIT 16

#define USERS 32

#define PROC 64

#define MEMORY 128

#define UPTIME 256

#define PADS 1

#define PANEL 0

#define MSG_LEN 1024



typedef struct msg_h{
	uint32_t type;
	unsigned char is_msg;
} msg_header;







int send_head(int,int,unsigned char);

int send_msg(int ,char* );


#endif
