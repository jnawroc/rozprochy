/*
 * Emiter - to program monitorujący stan wybranych parametrów systemu np.:
 * liczba uruchomionych procesów, ilość wolnej pamięci, nazwa procesora 
 * (różne typy danych; minimum łańcuch znaków, liczba całkowita). Emiter 
 * posiada dwa kanały komunikacyjne: (1) kanał zarządzania pozwalający na 
 * uruchomienie pobierania stanu, (2) kanał dla danych, którym z zadaną 
 * częstością przesyłany jest stan węzła. Emiter otrzymuje kanałem 
 * zarządzania zgłoszenie zawierające adres transportowy (adres IP + 
 * nr portu; dopuszczalny multicast) oraz częstość emisji. Po otrzymaniu 
 * zgłoszenia transmituje dane kanałem danych na adres podany w zgłoszeniu.
 * */


#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdarg.h>
#include "pid_set.h"
#include "funct.h"
#include "comm_const.h"
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>



int mprint(const char *path,const char *format,...) {
	int ret;
	time_t t;
        va_list ap;
        va_start(ap,format);
	static char timebuf[100];

	t=time(NULL);
	strftime(timebuf,100,"[%y%m%d %H:%M:%S] ",localtime(&t));

        FILE *plik=fopen(path,"a");

	if (*format == '\n'){
		format++;
		fprintf(plik,"\n");
	}

	fprintf(plik,timebuf);
        ret=vfprintf(plik,format,ap);
        va_end(ap);
        fclose(plik);
	return ret;
}


#define MAX_STATS 16

pthread_mutex_t pid_set_mutex = PTHREAD_MUTEX_INITIALIZER;

//int clients_des[MAX_CLIENTS];
int sigusr;
struct sockaddr_in _addr;
int client_des;
int my_des;
pid_t parentpid;
struct sockaddr_in my_addr;
pid_set* childs_pids;
//pid_set* clients_connected;
volatile int boolean=1;
char* IP;
char* dir_path;
DIR* dir;
static char* logfile="demonlog.txt";
uint16_t UDP_port;
uint32_t UDP_IP;
int UDP_sock;
int UDP_des;
struct sockaddr_in UDP_addr;


void hand_bool(int sig){

	boolean=1;
}


void sighand(int sig){

	char * syg;
	if (sig==SIGINT) syg=" SIGINT";
	else
	if (sig==SIGTERM) syg=" SIGTERM";
	else syg="";

	mprint(logfile,"<M>: demon odebrał sygnał%s, kończy pracę oraz zamyka wszelkie procesy potomne\n",syg);

	pthread_mutex_lock(&pid_set_mutex);
	int i;
	for (i=0;i<childs_pids->length;i++){
		kill(childs_pids->set[i],sig);
	}
	pthread_mutex_unlock(&pid_set_mutex);

	exit(0);

}

void* wait_for_children(void* nth){
	while(1){
		sleep(1);
		pthread_mutex_lock(&pid_set_mutex);
		if (parentpid!=getpid()) {
			pthread_mutex_unlock(&pid_set_mutex);
			return NULL;
		}
		int i;
		pid_t pidw;
		for (i=0;i<childs_pids->length;i++){
			if((pidw=waitpid(childs_pids->set[i],NULL,WNOHANG))>0){
				
				set_remove(childs_pids,pidw);
				mprint(logfile,"<M>: Zakończony proces: %d\n",pidw);

			}
		}
		pthread_mutex_unlock(&pid_set_mutex);
	}
}


void send_load_stat(void){
	mprint(logfile,"<M>: Wysylam load_stat\n");
	FILE* load=fopen("/proc/loadavg","r");
	if (!load) {
		return;
	}
	UDP_msg msg;
	char buf[129];
	float buf_[3];
	fread(buf,1,128,load);
	char* p1=buf;
	char* p2;
	for (p2=buf;*p2!=' ';p2++);
	*p2 = 0;
	float min1_load = strtod(p1,NULL);
	for (p1=p2+1;*p1==' ';p1++);
	for (p2=p1;*p2!=' ';p2++);
	*p2 = 0;
	float min5_load = strtod(p1,NULL);
	for (p1=p2+1;*p1==' ';p1++);
	for (p2=p1;*p2!=' ';p2++);
	*p2 = 0;
	float min15_load = strtod(p1,NULL);

	msg.type=LOAD;

	buf_[0] = min1_load;
	buf_[1] = min5_load;
	buf_[2] = min15_load;
	memcpy(msg.data,buf_,sizeof(float)*3);
	sendto(UDP_sock,&msg,sizeof(struct _UDP_msg),0,
		   (struct sockaddr*)&UDP_addr,sizeof(struct sockaddr));
	fclose(load);
}

void send_cpu_stat(void){
		mprint(logfile,"<M>: Wysylam cpu_stat... ");
	FILE* cpuinfo=fopen("/proc/cpuinfo","r");
	if (!cpuinfo) {
		return;
	}
	char buf [UDP_MSG_LEN-1];
	UDP_msg msg;
	msg.type=CPU_NAME;
	while (fgets(buf,UDP_MSG_LEN-1,cpuinfo)){
		char *p=strstr(buf,"\n");
		if (p) *p=0;
		if (strstr(buf,"processor")){
			char *p1 = strstr(buf,": ")+2;
			msg.data[0]=(unsigned char) (atoi(p1));
		}
		if (strstr(buf,"model name")){
			char *p1 = strstr(buf,": ")+2;
			strcpy((msg.data+1),p1);
				mprint(logfile,"poszlo\n");
			sendto(UDP_sock,&msg,sizeof(struct _UDP_msg),0,
				   (struct sockaddr*)&UDP_addr,sizeof(struct sockaddr));
		}

	}
	fclose(cpuinfo);
}


void send_proc_stat(void){
		mprint(logfile,"<M>: Wysylam lproc_stat\n");
	FILE *pipe=popen("ps -eo user,pid,size,cmd | wc -l","r");
	if (!pipe) {
		return;
	}
	int pr;
	fscanf(pipe,"%d",&pr);
	UDP_msg msg;
	msg.type = PROC;
	*(uint32_t*)msg.data = htonl(pr);
	pclose(pipe);

	if (sendto(UDP_sock,&msg,sizeof(struct _UDP_msg),0,
			   (struct sockaddr*)&UDP_addr,sizeof(struct sockaddr_in))<=0){
		perror("wysyłanie");
	};
}

void send_memory_stat(void){
			mprint(logfile,"<M>: Wysylam memory_stat\n");
	FILE *pipe=popen("free -o","r");
	int total,used,free;
	char rbuf[200];
	if (!pipe) {
		return;
	}
	
	UDP_msg msg;
	msg.type = MEMORY;
		
	while(!strstr(rbuf,"Mem:")) fscanf(pipe,"%s",rbuf);
	fscanf(pipe,"%d",&total);
	fscanf(pipe,"%d",&used);
	fscanf(pipe,"%d",&free);
	uint32_t* upi;
	upi = (uint32_t*) msg.data;
	*(upi++) = htonl(total);
	*(upi++) = htonl(used);
	*(upi++) = htonl(free);
		
	while(!strstr(rbuf,"Swap:")) fscanf(pipe,"%s",rbuf);
	fscanf(pipe,"%d",&total);
	fscanf(pipe,"%d",&used);
	fscanf(pipe,"%d",&free);
	*(upi++) = htonl(total);
	*(upi++) = htonl(used);
	*(upi++) = htonl(free);
	
	sendto(UDP_sock,&msg,sizeof(struct _UDP_msg),0,
		   (struct sockaddr*)&UDP_addr,sizeof(struct sockaddr));
	pclose(pipe);
}

	


void recv_est(int des){
	struct uint32_16 buf;
	recv(des,&buf,sizeof(struct uint32_16),0);
	UDP_port = buf.u16;
	UDP_IP = buf.u32;
	if (!buf.u32){
		UDP_IP =(_addr.sin_addr.s_addr);
	}
	mprint(logfile,"połączenie z IP %s, na porcie %d\n",inet_ntoa(*((struct in_addr*) &UDP_IP)),ntohs(UDP_port));
}

void daemon_run(){
	msg_header msg_r;

	int recived;
	kill(getppid(),SIGUSR1);
	fun_init();
	
	while(!boolean);
	recv_est(client_des);
	dir_path=getenv("PWD");
	mprint(logfile,"<M>: Rozpoczeta obsługa połączenia z IP: %s PID procesu obsługującego: %d\n",IP,getpid());
//	recv( client_des,&msg_r,sizeof(msg_header) ,0);
	int b=1;
	sleep(2);
	while (b && ((  recived=recv( client_des,&msg_r,sizeof(struct msg_h) ,0))>0)){
		unsigned char timestamp = msg_r.data;
		printf("timestamp: %d, type: %d\n",(int)msg_r.data,ntohl(msg_r.type));
		switch ntohl(msg_r.type){

			case MEMORY:
				fun_add(&send_memory_stat,timestamp);
				break;

			case PROC:
				fun_add(&send_proc_stat,timestamp);
				break;

			case CPU_NAME:
				fun_add(&send_cpu_stat,timestamp);
				break;
			
			case LOAD:
				fun_add(&send_load_stat,timestamp);
				break;
		
			case CONFIGURATION_END:
				b=0;
				break;
			
				
		}
	}
	
	UDP_sock=socket(PF_INET,SOCK_DGRAM,0);
	u_int yes=1;

	//bind(UDP_sock,(struct sockaddr*)&my_addr,sizeof (struct sockaddr_in));

	UDP_addr.sin_family=AF_INET;
	UDP_addr.sin_port=(UDP_port);
	UDP_addr.sin_addr.s_addr=(UDP_IP);
	int live=1;
	//printf("zaczyna wysyłanie %s\n",adr_to_string(UDP_addr));
	while (live){
		int recived;
		sleep(interval_gcd);
		fun_execute();
		recived = recv( client_des,&msg_r,sizeof(msg_header) ,MSG_DONTWAIT);
		if (recived!=EAGAIN){
			if (ntohl(msg_r.type)==CONNECTION_END){
				live=0;
			}
		}
	}
	close(UDP_des);
	mprint(logfile,"<M>: Zakończona obsługa połączenia z IP: %s PID procesu obsługującego: %d\n",IP,getpid());

}

int port;

int atoport(char* str){
	char* p;
	//printf("str: %s\n",str);
	int res;
	for (p=str;*p;p++){
		if ((*p)<'0' || (*p)>'9'){
			printf("potr powinien być liczbą %c\n",*p);
			return 0;
		}
	}
	res = atoi(str);
	if (res<=1024 || res>65535){
		printf("port powinien należeć do zakresu (1024, 65535]\n");
		return 0;
	}
	return res;
}
	 

int main(int argc,char* argv[]){
//	daemon(1,0);
	childs_pids=init_set();
	if (argc !=2 || !(port = atoport(argv[1]))){
		printf("Usage: emiter [port]\n");
		exit(2);
	}
	//printf("port: %d\n",port);
	parentpid=getpid();
	pthread_t thread;
	pthread_create(&thread,NULL,&wait_for_children,NULL);
	mprint(logfile,"\n<M>: Start Demona\n");
	signal(SIGTERM,&sighand);

	signal(SIGUSR1,&hand_bool);

	signal(SIGINT,&sighand);



	if ( (my_des = socket(PF_INET, SOCK_STREAM, 0)) <0){
		int er=errno;
		mprint(logfile,"<E>: błąd przy tworzeniu socketu: %s Demon kończy pracę\n",strerror(er));
		exit(-1);     
	}

	
	int t=1;

	setsockopt(my_des,SOL_SOCKET,SO_REUSEADDR,&t,sizeof(t));
	
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(my_des,SOL_SOCKET,SO_REUSEADDR,&t,sizeof(t));
	if( bind(my_des,(struct sockaddr*)&my_addr,sizeof (struct sockaddr_in)) != 0 ){
		int er=errno;
		mprint(logfile,"<E>: błąd przy bindowaniu socketu: %s Demon kończy pracę\n",strerror(er));
		exit(-2);     
	}
	
	if(listen(my_des,MAX_CLIENTS) <0){
		int er=errno;
		mprint(logfile,"<E>: błąd przy nasłuchiwaniu: %s Demon kończy pracę\n",strerror(er));
		exit(-3);     
	}

	pid_t child=0;
	struct sockaddr_in addr;	 
	socklen_t addrlen=sizeof(struct sockaddr_in);
	while(boolean){
		if ((client_des=accept(my_des,(struct sockaddr*)&addr,&addrlen))<0){
			int er=errno;
			mprint(logfile,"<E>: błąd przy akceptacji: %s\n",strerror(er));
	
		}else{
			boolean=0;

			IP=inet_ntoa(addr.sin_addr);
			mprint(logfile,"<M>: przychodzące połączenie z adresu IP %s\n",IP);
				
			child=fork();
			if (child<0){
				int er=errno;
				mprint(logfile,"<E>: błąd przy tworzeniu procesu potomnego: %s\n",strerror(er));
				boolean=1;
			}

			if (child>0){

				while(!boolean);
				boolean=1;
				pthread_mutex_lock(&pid_set_mutex);

				if (! set_add(childs_pids,child)){
					mprint(logfile,"<E>: przekroczono maxymalną ilośc połączeń\n");
					kill(child,SIGINT);
					waitpid(child,0,0);
				}
					
				

				kill(child,SIGUSR1);

				pthread_mutex_unlock(&pid_set_mutex);
			}
			if (!child) {
				signal(SIGINT,SIG_DFL);
				signal(SIGTERM,SIG_DFL);

				_addr = addr;	 
				pthread_mutex_lock(&pid_set_mutex);
				free(childs_pids);
				pthread_mutex_unlock(&pid_set_mutex);

				

			}
		}
		sleep(1);
	}
	if (!child) daemon_run();
			
	return 0;
}
