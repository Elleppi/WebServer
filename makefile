
CC=gcc
CFLAGS=-I. -Wall -Wextra -O2
DEPS = Basic.h Functions.h CacheList.h UserAgentAnalyzer.h
LIBS=-lsqlite3

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

webserver: Main.o 
	gcc Main.o $(CFLAGS) $(LIBS) -pthread -o SWWS

clean:
	rm -f $(PROGS) *.o
