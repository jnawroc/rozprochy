
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "comm_const.h"
#include <argp.h>
#include "stats_functs.h"

const char *argp_program_version = "observer 1.0";
const char *argp_program_bug_address = "<none>";
struct sockaddr_in emiters_addr[MAX_EMITERS];
struct sockaddr_in emiters_UDP_addr[MAX_EMITERS];
int emiters_des[MAX_EMITERS];
int UDP_des;
struct ip_mreq mreq;

int (*debug)(const char *format,...);
int (*verbouse)(const char *format,...);

int none(const char *format,...){
	return 0;
}

struct arguments {
	uint32_t IPs[MAX_EMITERS];
	int ports[MAX_EMITERS];
	int IPcount;
	uint32_t multicast_IP;
	int multicast_port;
	unsigned char timestamps[4];
};

struct arguments *args;
struct sockaddr_in my_UDP_addr;
void sighand (int sig)
{
	int i;

	for (i = 0; i < (args->IPcount); i++) {
		debug("%s do %s\n",head_names[CONNECTION_END],inet_ntoa( (emiters_addr[i]).sin_addr));
		send_head (emiters_des[i], CONNECTION_END, 0);
	}

	exit (0);
}

	 /*
	  * Program documentation. 
	  */
static char doc[] =
	"Observer - program obserwujacy emiterow, badz filtry\n\
  adres_IP - IP emiterow/filtrow, nalezy podac conajmniej jeden.";

	 /*
	  * A description of the arguments we accept. 
	  */
static char args_doc[] =
	"[adres_IP...]";

	 /*
	  * The options we understand. 
	  */
static struct argp_option options[] = {
	{"multicast", 'm', "MulticastIP", 0,
		"Ustawia adres na multicast"},
	{"proc", 'p', "timestamp", 0, "liczba procesow"},
	{"memory", 'M', "timestamp", 0, "statystyki pamieci"},
	{"cpu", 'c', "timestamp", 0, "nazwa procesorow"},
	{"load", 'l', "timestamp", 0, "obcizzenie systemu"},
	{"all", 'a', "timestamp", 0, "wszystkie stat"},
	{"verbouse",'v',0,0,"badz rozmowny"},
	{"debug",'d',0,0,"wlacza komunikaty debbugowania"},
	{0}
};


	 /*
	  * Used by main to communicate with parse_opt. 
	  */


	 /*
	  * Parse a single option. 
	  */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	/*
	 * Get the input argument from argp_parse, which we know is a pointer
	 * to our arguments structure. 
	 */
	struct arguments *arguments = state->input;
	struct hostent *h;

	char *p;

	switch (key) {
		case 'm':
			p = strstr (arg, "/");
			*p = 0;
			p++;
			h = gethostbyname (arg);

			arguments->multicast_IP =(*((uint32_t*)h->h_addr_list[0]));
				//ntohl (*((unsigned long int *) (h->h_addr_list[0])));
			arguments->multicast_port = htons( atoi (p));
			break;
		case 'p':
			arguments->timestamps[PROC] = (unsigned char) atoi (arg);
			break;
		case 'a':
			memset (arguments->timestamps, (unsigned char) atoi (arg), 4);
			break;
		case 'M':
			arguments->timestamps[MEMORY] = (unsigned char) atoi (arg);
			break;
			break;
		case 'c':
			arguments->timestamps[CPU_NAME] = (unsigned char) atoi (arg);
			break;
		case 'l':
			arguments->timestamps[LOAD] = (unsigned char) atoi (arg);
			break;
		case 'v':
			verbouse=&printf;
			break;
		case 'd':
			verbouse=&printf;
			debug=&printf;
			break;

		case ARGP_KEY_ARG:
			if (state->arg_num >= MAX_EMITERS)
				/*
				 * Too many arguments. 
				 */
				argp_usage (state);
			p = strstr (arg, "/");
			*p = 0;
			p++;
			
			h = gethostbyname (arg);

			arguments->IPs[state->arg_num] = *((uint32_t*)h->h_addr_list[0]);
		
			arguments->ports[state->arg_num] = atoi (p);

			arguments->IPcount = state->arg_num + 1;

			break;

		case ARGP_KEY_END:
			if (!state->arg_num)
				argp_usage (state);
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


	 /*
	  * Our argp parser. 
	  */
static struct argp argp = { options, parse_opt, args_doc, doc };


int send_est (int des, uint32_t mip, uint16_t mipp)
{
	struct sockaddr_in *adr = &my_UDP_addr;
	struct sockaddr_in addr;
	socklen_t size = sizeof (struct sockaddr_in);
	if (getsockname (des, (struct sockaddr *) &addr, &size)){
		perror("getsockname");
	}
	(*adr)=addr;

	adr->sin_family = AF_INET;
	adr->sin_port = htons(8888);
	if (mip) {
		(*debug)("\t\tjest multicast\n");
		adr->sin_port = (mipp);
		adr->sin_addr.s_addr = htonl(INADDR_ANY);
	}
	struct uint32_16 uu;
	uu.u16 = (uint16_t) (mip ? mipp : adr->sin_port);
	uu.u32 = (uint32_t) (mip ? mip  : 0);//adr->sin_addr.s_addr);

	//((uint16_t *) buf)[0] =  (mip ? mipp : (uint16_t)adr->sin_port);
	//((uint32_t *) (buf + sizeof (uint16_t)))[0] =(uint32_t)
	//	 (mip ? mip : adr->sin_addr.s_addr);
	debug("\t\tmy_UDP_addr %s\n",inet_ntoa( my_UDP_addr.sin_addr));
	return send(des, (const void*) &uu, sizeof (struct uint32_16),0);
}


void take_stats ()
{
	debug("zaczyna zbierac %s\n",adr_to_string(my_UDP_addr));
	UDP_des = socket (PF_INET, SOCK_DGRAM, 0);
	if (bind (UDP_des, (struct sockaddr *) &my_UDP_addr,
		sizeof (struct sockaddr_in))){
		perror("bindowanie UDP");
		exit(1);
	}
	u_int yes=1;
    if (setsockopt(UDP_des,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
       perror("Reusing ADDR failed");
       exit(1);
    }
	if (args->multicast_IP){
	mreq.imr_multiaddr.s_addr=(args->multicast_IP);
    mreq.imr_interface.s_addr=htonl(INADDR_ANY);
    if (setsockopt(UDP_des,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		perror("setsockopt");
		exit(1);
    }
	}
	UDP_msg msg;
	volatile int i;

	while (1) {
				
		for (i = 0; i < args->IPcount; i++) {
		
			socklen_t len = sizeof (struct sockaddr_in);
			debug("próba odczytu z %s\n",adr_to_string(emiters_addr[i]));
			int res ;
			if (args->multicast_IP){
				if ((res=recvfrom (UDP_des, &msg, sizeof (struct _UDP_msg),
						MSG_DONTWAIT, (struct sockaddr *) &my_UDP_addr,
						&len))<0){
					
					sleep(1);
			//	perror("recvfrom");
				}
			}else{
				if ((res=recvfrom (UDP_des, &msg, sizeof (struct _UDP_msg),
						MSG_DONTWAIT, (struct sockaddr *) &emiters_addr[i],
						&len))<0){
					sleep(1);
			//	perror("recvfrom");
				}	
			}
			if ( res>0) {

				char* ip=strdup(inet_ntoa(emiters_addr[i].sin_addr));
				debug("zakonczona pomyslnie\n");
				if (msg.type & 1024) {
					(*combined_func[msg.type ^ 1024]) (ip,msg.data);
				} else {
					(*stat_func[msg.type]) (ip,msg.data);
				}
			}
			
		}
	}
}

int main (int argc, char **argv)
{
	verbouse=&none;
	debug=&none;
	struct arguments arguments;
	signal(SIGTERM,&sighand);

	signal(SIGINT,&sighand);
	

	/*
	 * Default values. 
	 */
	memset (arguments.IPs, 0, sizeof (uint32_t) * MAX_EMITERS);
	arguments.multicast_IP = 0;
	memset (arguments.timestamps, 0, 4);
	arguments.IPcount = 0;

	/*
	 * Parse our arguments; every option seen by parse_opt will be
	 * reflected in arguments. 
	 */

	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	args = &arguments;
	(*debug)("inicjacja funkcji do zbierania statystyk\n");
	init_stats_functs();
	volatile int i;
	
	for (i = 0; i < args->IPcount; i++) {

		(emiters_addr[i]).sin_family = AF_INET;
		(emiters_addr[i]).sin_port = htons (args->ports[i]);
		(emiters_addr[i]).sin_addr.s_addr =  (args->IPs[i]);
		(*verbouse)("laczenie z klientem o IP: %s na porcie %d\n",
					inet_ntoa((emiters_addr[i]).sin_addr),args->ports[i]);
		(*verbouse)("\ttworzenie socketu\n");

		emiters_des[i] = socket (PF_INET, SOCK_STREAM, 0);
		int t = 1;
		
		setsockopt (emiters_des[i], SOL_SOCKET, SO_REUSEADDR, &t, sizeof (int));
		(*verbouse)("\tłączenie\n");
		if (connect (emiters_des[i],
				(struct sockaddr *) (emiters_addr + i),
				sizeof (struct sockaddr_in)) != 0) {
			perror ("connect");
			return 0;
		}
		(*verbouse)("\twysylanie kanalem zarzadzania opcje polaczenia\n");
		send_est (emiters_des[i], args->multicast_IP, args->multicast_port);
		int k=0;
		for(k=0;k<4;k++){
			(*debug)("k=%d\n",k);
			if (args->timestamps[k]) {
				(*debug)("\twysylanie %s z timestampem: %d\n",head_names[k],
					  args->timestamps[k]);
				send_head (emiters_des[i], k, args->timestamps[k]);

			}
		}
		send_head (emiters_des[i], CONFIGURATION_END, 0);
		debug("wyslane %s\n",head_names[CONFIGURATION_END]);

	}

	take_stats();

	return 0;
}
