COMM = comm_const.o
CSRC = src/common/comm_const.c
ESRC = src/emiter/emiter.c 
OSRC = src/observer/observer.c
EOBJ = pid_set.o comm_const.o funct.o
OOBJ = comm_const.o stats_functs.o
PSSRC = src/emiter/pid_set.c 
FSRC = src/emiter/funct.c
SFSRC = src/observer/stats_functs.c
CC = gcc
INCLUDES = -I src/common
CFLAGS = -O2 -Wall
LIBS = -lpthread

all: emiter observer

emiter: $(ESRC) $(EOBJ)
	$(CC) -o emiter $(CFLAGS) $(LIBS) $(ESRC) $(EOBJ) $(INCLUDES)

observer: $(OSRC) $(OOBJ)
	$(CC) -o observer $(CFLAGS) $(LIBS) $(OSRC) $(OOBJ) $(INCLUDES)

$(COMM): $(CSRC)
	$(CC) -c $(CFLAGS) $(CSRC) $(INCLUDES)
 
pid_set.o: $(PSSRC)
	$(CC) -c $(CFLAGS) $(PSSRC) $(INCLUDES)


funct.o: $(FSRC)
	$(CC) -c $(CFLAGS) $(FSRC) $(INCLUDES)

stats_functs.o: $(SFSRC)
	$(CC) -c $(CFLAGS) $(SFSRC) $(INCLUDES)

clean:
	rm -f $(COMM)  *.o
