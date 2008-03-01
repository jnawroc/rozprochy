#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include "comm_const.h"


int send_head(int des,int type,unsigned char is_msg){
	msg_header msg;
	msg.type=htonl(type);
	msg.is_msg=(is_msg);
	return send(des,&msg,sizeof(msg_header),0);
}

int send_msg(int des,char* name){
	int ret;
	char buf[MSG_LEN];
	strcpy(buf,name);
	ret = send(des,buf,MSG_LEN,0);
	return ret;
}

