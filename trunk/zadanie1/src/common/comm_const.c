#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include "comm_const.h"


int send_head(int des,uint32_t type,unsigned char data){
	msg_header msg;
	msg.type=htonl(type);
	msg.data=(data);
	return send(des,&msg,sizeof(msg_header),0);
}


int send_msg(int des,char* name){
	int ret;
	char buf[MSG_LEN];
	strncpy(buf,name,MSG_LEN);
	ret = send(des,buf,MSG_LEN,0);
	return ret;
}

int send_msg_vl(int des,char* name,int length){
	int ret;
	char buf[length];
	memcpy(buf,name,length);
	ret = send(des,buf,length,0);
	return ret;
}



