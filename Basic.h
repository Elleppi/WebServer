#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <semaphore.h>
#include "sqlite3.h"
#include "UserAgentAnalyzer.h"


#define DEFAULT_PORT   9091
#define BACKLOG       256
#define MAXLINE    10000
#define TIMEOUT       15
#define THREADSPERCHILD 25
#define THREADLIMIT  64
#define MINIDLETHREAD 125
#define MAXIDLETHREAD 300
#define SERVERLIMIT 16
#define STARTSERVER 5
#define CACHELIMIT  15

/*Nome del server inviato nei 
messaggi di risposta HTTP*/
const char NameServer[] = "SWWS";

/*Usato come booleano, indica la possibilità
da parte dell'utente di effettuare il debug*/
unsigned short int debug;