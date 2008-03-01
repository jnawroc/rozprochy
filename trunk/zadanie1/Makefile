COMM = comm_const.o
CSRC = src/common/comm_const.h src/common/comm_const.c
ESRC = src/emiter/emiter.c 
OSRC = src/observer/observer.c
EOBJ = pid_set.o comm_const.o
OOBJ = comm_const.o
PSSRC = src/emiter/pid_set.c src/emiter/pid_set.h
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
	$(CC) -c $(CFLAGS) $(CSRC)

pid_set.o: $(PSSRC)
	$(CC) -c $(CFLAGS) $(PSSRC)

clean:
	rm -f $(COMM) emiter observer *.o
