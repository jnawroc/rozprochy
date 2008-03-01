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




pthread_mutex_t pid_set_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

//int clients_des[MAX_CLIENTS];
int sigusr;
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

int file_count;
char** file_paths;
struct stat** file_stats;
int screen_width;


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

int file_changed(struct stat* st,int i){
	if (st->st_ctime!=file_stats[i]->st_ctime) return 1;

	if (st->st_atime!=file_stats[i]->st_atime) return 1;

	if (st->st_mtime!=file_stats[i]->st_mtime) return 1;

	if (st->st_size!=file_stats[i]->st_size) return 1;

	return 0;
}

void send_stat(int i){
	char timebuf[100];
	char buf[MSG_LEN];

	struct stat *st=file_stats[i];


	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,file_paths[i]);	
	pthread_mutex_unlock(&send_mutex);

	sprintf(buf," aktualne dane:\nrozmiar: %llu\nczas dostepu: ",(long long unsigned)st->st_size);
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&st->st_atime));
	strcat(buf,timebuf);
	strcat(buf,"\nczas modyfikacji: ");
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&st->st_mtime));
	strcat(buf,timebuf);
	strcat(buf,"\nczas zmiany wlasciwosci:");
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S\n\n",localtime(&st->st_ctime));
	strcat(buf,timebuf);

	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,buf);
	pthread_mutex_unlock(&send_mutex);
}



void send_memory_stat(void){
	char buf[MSG_LEN];
	FILE *pipe=popen("free -o","r");
	int total,used,free;
	char rbuf[200];
	char timebuf[100];
	char stotal[18],sused[18],sfree[18];
	if (!pipe) {
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,"Blad - nie da sie pobrac statystyk pamieci\n\n");
		pthread_mutex_unlock(&send_mutex);
		return;
	}
	time_t t=time(NULL);
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&t));

	sprintf(buf,"Statystyki pamieci z %s:\n%37s%17s%17s\n",timebuf,"calkowita","zajeta","wolna");

	while(!strstr(rbuf,"Mem:")) fscanf(pipe,"%s",rbuf);
	fscanf(pipe,"%d",&total);
	fscanf(pipe,"%d",&used);
	fscanf(pipe,"%d",&free);

	sprintf(stotal,"%.0d%s%.0d%s%d%s",total>>20,(total>>20)?"G ":"",((total>>10)%1024),(total>>10)?"M ":"",total%1024,"K");
	sprintf(sused,"%.0d%s%.0d%s%d%s",used>>20,(used>>20)?"G ":"",((used>>10)%1024),(used>>10)?"M ":"",used%1024,"K");
	sprintf(sfree,"%.0d%s%.0d%s%d%s",free>>20,(free>>20)?"G ":"",((free>>10)%1024),(free>>10)?"M ":"",free%1024,"K");

	sprintf(buf+strlen(buf),"%20s%17s%17s%17s\n","Pamiec uzytkownika:",stotal,sused,sfree);

	while(!strstr(rbuf,"Swap:")) fscanf(pipe,"%s",rbuf);
	fscanf(pipe,"%d",&total);
	fscanf(pipe,"%d",&used);
	fscanf(pipe,"%d",&free);

	sprintf(stotal,"%.0d%s%.0d%s%d%s",total>>20,(total>>20)?"G ":"",((total>>10)%1024),(total>>10)?"M ":"",total%1024,"K");
	sprintf(sused,"%.0d%s%.0d%s%d%s",used>>20,(used>>20)?"G ":"",((used>>10)%1024),(used>>10)?"M ":"",used%1024,"K");
	sprintf(sfree,"%.0d%s%.0d%s%d%s",free>>20,(free>>20)?"G ":"",((free>>10)%1024),(free>>10)?"M ":"",free%1024,"K");

	sprintf(buf+strlen(buf),"%20s%17s%17s%17s\n\n","przestrzen wymiany:",stotal,sused,sfree);

	pclose(pipe);

	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,buf);
	pthread_mutex_unlock(&send_mutex);
}

	

void send_users_stat(void){
	char buf[MSG_LEN];
	int max_l=0;
	char rbuf[200];
	char format[100];

	FILE *pipe=popen("who | cut -d\" \" -f1 |sort|uniq","r");
	if (!pipe) {
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,"Blad - nie da sie pobrac statystyk uzytkownikow\n\n");
		pthread_mutex_unlock(&send_mutex);
		return;
	}

	while(fscanf(pipe,"%s",rbuf)>0) 
		if (strlen(rbuf)>max_l) max_l=strlen(rbuf);
	pclose(pipe);

	pipe=popen("who | cut -d\" \" -f1 |sort|uniq","r");
	if (!pipe) {
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,"Blad - nie da sie pobrac statystyk uzytkownikow\n\n");
		pthread_mutex_unlock(&send_mutex);
		return;
	}

	max_l+=2;
	int n=screen_width/max_l;
	int i=0;

	char timebuf[100];
	time_t t=time(NULL);
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&t));

	sprintf(buf,"Uzytkownicy zalogowani z %s:\n",timebuf);
	while(fscanf(pipe,"%s",rbuf)>0){
		i++; 
		if (i>=n) i=0;
		if ((strlen(buf)+max_l)>=(MSG_LEN-1)){
			pthread_mutex_lock(&send_mutex);
			send_msg(client_des,buf);
			pthread_mutex_unlock(&send_mutex);
			buf[0]=buf[1]=0;
		}
		sprintf(format,"%c%ds%s",'%',max_l,(i)?"":"\n");
		sprintf(buf+strlen(buf),format,rbuf);
	}
	strcat(buf,"\n\n");


	pclose(pipe);
	if (buf[1]){
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,buf);
		pthread_mutex_unlock(&send_mutex);
	}
}

void send_proc_stat(void){
	char buf[MSG_LEN];


	FILE *pipe=popen("ps -eo user,pid,size,cmd ","r");
	if (!pipe) {
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,"Blad - nie da sie pobrac statystyk procesow\n\n");
		pthread_mutex_unlock(&send_mutex);
		return;
	}

	char timebuf[100];
	time_t t=time(NULL);
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&t));

	sprintf(buf,"Informacje o procesach z %s\n(wlasciciel, PID, zajeta pamiec i linia komend):\n",timebuf);

	fgets(buf+strlen(buf),MSG_LEN-strlen(buf),pipe);
	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,buf);
	pthread_mutex_unlock(&send_mutex);


	while ((fgets(buf,MSG_LEN,pipe))){
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,buf);
		pthread_mutex_unlock(&send_mutex);
	}
	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,"\n");
	pthread_mutex_unlock(&send_mutex);


	pclose(pipe);
}

void send_uptime_stat(){
	FILE* fuptime=fopen("/proc/uptime","r");
	if (!fuptime) {
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,"Blad - nie da sie pobrac uptime\n\n");
		pthread_mutex_unlock(&send_mutex);
		return;
	}

	double uptime;
	double idletime;
	char buf[MSG_LEN];
	char timebuf[100];
	time_t t=time(NULL);
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&t));


	fscanf(fuptime,"%lf",&uptime);
	fscanf(fuptime,"%lf",&idletime);	

	int up=(int) uptime;
	int upss=(int)100*( uptime-((double) up ) );
	int idle=(int) idletime;
	int idless=(int)100*( idletime-((double) idle ) );



	sprintf(buf,"%s:\nSystem dziala przez: %.0d%s%.0d%s",timebuf,up/(3600*24), (up/(3600*24))?" dni, ":"" ,(up/(3600))%24,((up/(3600))%24)?" godzin, ":"");
	sprintf(buf+strlen(buf),"%.0d%s%d sekund, %d setnych\nBezczynny przez: ",(up/60)%60,(up/60)?" minut, ":"",up%60,upss);


	sprintf(buf+strlen(buf),"%.0d%s%.0d%s",idle/(3600*24), (idle/(3600*24))?" dni, ":"" ,(idle/(3600))%24,((idle/(3600))%24)?" godzin, ":"");
	sprintf(buf+strlen(buf),"%.0d%s%d sekund, %d setnych\n\n",(idle/60)%60,(idle/60)?" minut, ":"",idle%60,idless);



	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,buf);
	pthread_mutex_unlock(&send_mutex);


	pclose(fuptime);

}


void send_load_stat(){
	FILE* load=fopen("/proc/loadavg","r");
	if (!load) {
		pthread_mutex_lock(&send_mutex);
		send_msg(client_des,"Blad - nie da sie pobrac obciazenia systemu\n\n");
		pthread_mutex_unlock(&send_mutex);
		return;
	}

	double load1;
	double load5;
	double load15;
	char buf[MSG_LEN];
	char timebuf[100];
	time_t t=time(NULL);
	strftime(timebuf,100,"%y-%m-%d %H:%M:%S",localtime(&t));


	fscanf(load,"%lf",&load1);
	fscanf(load,"%lf",&load5);	
	fscanf(load,"%lf",&load15);	




	sprintf(buf,"%s: obciazenie systemu przez\n    ostatnia minute: %2.2lf,\n    5 minut: %2.2lf,\n    15 minut:%2.2lf\n\n",timebuf,load1,load5,load15);

	pthread_mutex_lock(&send_mutex);
	send_msg(client_des,buf);
	pthread_mutex_unlock(&send_mutex);


	pclose(load);

}


void* file_monitor(void* nth){
	int i;
	int f[1024];
	struct stat st;
	while(1){
		sleep(1);
		pthread_mutex_lock(&file_mutex);
		for (i=0;i<file_count;i++){
			if (stat(file_paths[i],&st)>=0){
				f[i]=file_changed(&st,i);
			}else f[i]=0;
		}
		pthread_mutex_unlock(&file_mutex);
		for (i=0;i<file_count;i++)
			if (f[i]){
				struct stat* sold;
				struct stat* snew=malloc(sizeof(struct stat));
				sold=file_stats[i];
				stat(file_paths[i],snew);
				file_stats[i]=snew;
				free(sold);
				send_stat(i);
			}
	}
}

void daemon_run(){
	char buf[MSG_LEN];
	char buf2[MSG_LEN];
	msg_header msg_r;
	struct stat st;
	pthread_t thread1;
	pthread_create(&thread1,NULL,&file_monitor,NULL);



	file_count=0;
	file_paths=malloc(sizeof(char*)*1024);
	file_stats=malloc(sizeof(void*)*1024);

	int recived;
	kill(getppid(),SIGUSR1);

	while(!boolean);
	dir_path=getenv("PWD");
	mprint(logfile,"<M>: Rozpoczeta obsługa połączenia z IP: %s PID procesu obsługującego: %d\n",IP,getpid());

	while ((  recived=recv( client_des,&msg_r,sizeof(msg_header) ,0))){
		switch ntohl(msg_r.type){
			case MONIT:
				if (recv(client_des,buf,MSG_LEN,0)>0){
					if ((file_count<1024)&&(stat(buf,&st)>=0)){
						file_stats[file_count]=malloc(sizeof(struct stat));
						(*file_stats[file_count])=st;
						pthread_mutex_lock(&file_mutex);
						file_paths[file_count]=strdup(buf);
						pthread_mutex_unlock(&file_mutex);
						
						send_stat(file_count++);
						
						
						
					}else{
						int er=errno;
						send_msg(client_des,buf);
						pthread_mutex_lock(&send_mutex);
						if (file_count>=1024) send_msg(client_des,":\nNie bede monitorowal pliku , poniewaz monitoruje juz zbyt wiele\n");
						else{
							sprintf(buf2,":\nNie udalo sie uzyskac statystyk pliku: %s\n",strerror(er));
							send_msg(client_des,buf2);
						}
						pthread_mutex_unlock(&send_mutex);
						
					}
				}
				break;
			case USERS:
				send_users_stat();
				break;

			case MEMORY:
				send_memory_stat();					
				break;

			case PROC:
				send_proc_stat();
				break;

			case UPTIME:
				send_uptime_stat();
				break;

			case LOAD:
				send_load_stat();
				break;

			case SET_WIDTH:
				screen_width=msg_r.is_msg;
				break;
		}
	}


	mprint(logfile,"<M>: Zakończona obsługa połączenia z IP: %s PID procesu obsługującego: %d\n",IP,getpid());

}



int main(int argc,char* argv[]){
	daemon(1,0);
	childs_pids=init_set();
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
