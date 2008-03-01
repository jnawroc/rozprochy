/*
 * 3. Obserwator - to program pozwalający zaprezentować stan jednego lub więcej 
 * węzłów Emitera lub Filtra użytkownikowi. Program obserwator podobnie jak 
 * Filtr powinien przyjmować 1 lub więcej adresów transportowych węzłów Emitera 
 * lub Filtra i podobnie jak program Filtr powinien nasłuchiwać w trybie unicast 
 * lub multicast. Przykładowa linia poleceń może wyglądać następująco:
 *     obserwator -m 239.2.3.4/50000 2.3.4.5/2355
 *     co powinno spowodować, że Obserwator zgłosi się do programu 
 *     (Emitera lub Filtra) i zażąda emisji w trybie multicast wyświetlając 
 *     otrzymywane informacje na ekranie. Informacja dla użytkownika powinna być 
 *     czytelna, przykładowo Obserwator może wyświetlić następujące dane:
 *         * 12:53:04 1.2.3.4 - liczba procesów: 17
 *         * 12:53:07 2.3.4.5 - liczba procesorów: 2
 *         * 12:53:12 2.2.2.2 - maksymalna ilość wolnej pamięci: 627 MB - host 1.2.3.4
 * */




#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <curses.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>
#include <pthread.h> 
#include "comm_const.h"

int sned_est(int des){
	struct sockaddr_in adr;
	unsigned size = sizeof(struct sockaddr_in) ;
	getsockname(des,(struct sockaddr*)&adr,&size);
	char buf[sizeof(uint16_t)+sizeof(uint32_t)];
	*((uint16_t*)buf) = htons(adr.sin_port);
	*((uint16_t*)buf + sizeof(uint32_t)) = htons(adr.sin_addr.s_addr);
	send_msg_vl(des,buf,sizeof(uint16_t)+sizeof(uint32_t));
	return 1;
}



int main(int argc,char* argv[]){
	
	return 0;
}

