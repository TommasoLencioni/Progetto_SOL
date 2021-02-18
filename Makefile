INCDIR			= ./include
LIBDIR          = ./lib
SRCDIR          = ./src
BINDIR          = ./bin
ANALISI			= ./analisi.sh

CC				=  gcc
CFLAGS	        += -std=c99 -Wall -D_POSIX_C_SOURCE=199309L
OBJ				= -o
COMP			= -c
AR              =  ar
ARFLAGS         =  rvs
INCLUDES        = -I $(INCDIR)
LIBS			= -lpthread -llinkedlist
LDFLAGS         = -L $(LIBDIR)
DBGFLAGS        = -g -DDEBUG=1
FLAGS           = $(DBGFLAGS)

TARGET		= $(BINDIR)/supermercato

.PHONY: all clean run test test1

all: $(TARGET)

#CREAZIONE ESEGUIBILE SUPERMERCATO CON LINKING DEI FILE OGGETTO DI CASSIERE E CLIENTE
$(BINDIR)/supermercato: $(SRCDIR)/supermercato.c $(SRCDIR)/cassiere.o $(SRCDIR)/cliente.o $(LIBDIR)/liblinkedlist.a
	$(CC) $(CFLAGS) ./$< ./$(SRCDIR)/cassiere.o ./$(SRCDIR)/cliente.o $(LDFLAGS) $(LIBS) $(INCLUDES) $(OBJ) ./$@

#CREAZIONE FILE OGGETTO CASSIERE
$(SRCDIR)/cassiere.o: $(SRCDIR)/cassiere.c
	$(CC) $(COMP) ./$< $(LDFLAGS) $(INCLUDES) $(OBJ) ./$@

#CREAZIONE FILE OGGETTO CLIENTE
$(SRCDIR)/cliente.o: $(SRCDIR)/cliente.c
	$(CC) $(COMP) ./$< $(LDFLAGS) $(INCLUDES) $(OBJ) ./$@

#CREAZIONE LIBRERIA STATICA LINKED LIST
$(LIBDIR)/liblinkedlist.a: $(SRCDIR)/linkedlist.o
	$(AR) $(ARFLAGS) ./$@ $<

#CREAZIONE FILE OGGETTO LINKED LIST
$(SRCDIR)/linkedlist.o: $(SRCDIR)/linkedlist.c
	$(CC) $(COMP) ./$< $(INCLUDES) $(OBJ) ./$@

#PHONY DI PULIZIA
clean:
	-rm -f $(TARGET)
	-rm -f ./log/*
	-find . -name "*.a" -type f -delete
	-find . -name "*.o" -type f -delete

#PHONY DI TESTING
run:
	@-rm -f ./log/*
	$(TARGET)

test:
	-rm -f ./log/*
	$(TARGET) & \
	sleep 25; \
	killall -SIGHUP supermercato; \
	wait $(shell $!); \
	bash $(ANALISI)

test1:
	-rm -f ./log/log.txt
	$(TARGET) & \
	sleep 15; \
	killall -SIGQUIT supermercato; \
	wait $(shell $!); \
	bash $(ANALISI)
