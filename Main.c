#include "Basic.h"
#include "CacheList.h"
#include "Functions.h"

/*contatore numero processi totali esistenti*/
int *TotProcess;  

/*contatore numero totale threads esistenti*/
int *TotThreads; 

/*numero totale di threads idle presenti*/
int *TotIdleThreads; 

/*lista delle connessioni pronte ad essere esaudite:
  una per ogni processo e condivisa con i threads ad esso associati*/
int ConnectionList[THREADLIMIT + 1], iMore = 0, iLess = 0;      

/*file descriptor per il log.txt*/
int LogFd;  

/*indirizzo del socket*/
int listenFd;                  

/* semafori POSIX */

/*semaforo per listenFd */
sem_t *sem_listen;
/*semaforo per totIdleThread */              
sem_t *sem_totIdle;
/*semaforo per totThread */             
sem_t *sem_totThread;           
/*semaforo per totProcess */
sem_t *sem_totProcess;          
/*semaforo per array di strutture data_process */
sem_t *sem_dataProcess;         
/*semaforo per la sezione di codice in cui si esegue la convert */
sem_t *sem_convert;    

/*utilizzati per gestire la segnalazione ai threads worker della presenza di una nuova connessione pronta*/
pthread_mutex_t mutex_connfd = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_connfd = PTHREAD_COND_INITIALIZER;

/*PID del processo Master*/
pid_t Master_pid;


/*struttura dati separata per ogni processo ma condivisa con i threads ad esso associati*/
struct data_process
{
    /*numero threads totali associati al processo corrente*/
	int nThTot;  
    /*numero threads idle del processo corrente*/
	int nThIdle;  
};

/*puntatore alla struttura dati data_process*/
struct data_process *DP;  

void initialize_CI()
{
    /*creazione struttura dati condivisa per le info della cache*/
    CI = mmap(NULL, sizeof(struct CacheInfo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (CI == MAP_FAILED) {
        perror("mmap in CacheInfo");
        exit(EXIT_FAILURE);
    }

    /*inizializzazione e condivisione variabile contenente il nome dell'immagine richiesta*/
    CI->image = mmap(NULL, 100*sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (CI->image == MAP_FAILED) {
        perror("mmap in CI->image");
        exit(EXIT_FAILURE);
    }

    /*inizializzazione e condivisione variabile indicante l'invio del file*/
    CI->file_sent = mmap(NULL, sizeof(unsigned short int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (CI->file_sent == MAP_FAILED) {
        perror("mmap in CI->file_sent");
        exit(EXIT_FAILURE);
    }

    /*inizializzazione e condivisione mutex per la cache*/
    CI->mutex_cache = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (CI->mutex_cache == MAP_FAILED) {
        perror("mmap in CI->mutex_cache");
        exit(EXIT_FAILURE);
    }

    /*inizializzazione e condivisione condition per la cache*/
    CI->cond_cache = mmap(NULL, sizeof(pthread_cond_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (CI->cond_cache == MAP_FAILED) {
        perror("mmap in CI->cond_cache");
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(CI->mutex_cache, &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(CI->cond_cache, &condAttr);
}

/*crea e inizializza semaforo condiviso tra i processi*/
sem_t *initialize_semaphore() 
{
    /*creazione semaforo condiviso*/
	sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem == MAP_FAILED) {
        perror("mmap()");
        exit(EXIT_FAILURE);
    }
    /*inizializzaizione semaforo
      pshared = 1: condiviso tra processi
      value = 1:   valore iniziale  */
    if (sem_init(sem, 1, 1) == -1) {
        perror("sem_init()");
        exit(EXIT_FAILURE);
    }

    return sem;
}

/*crea e inizializza semaforo condiviso tra i threads*/
sem_t *initialize_semaphore_t() 
{
    /*creazione semaforo condiviso*/
    sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem == MAP_FAILED) {
        perror("mmap()");
        exit(EXIT_FAILURE);
    }
    /*inizializzaizione semaforo
      pshared = 0: condiviso tra threads
      value = 1:   valore iniziale  */
    if (sem_init(sem, 0, 1) == -1) {
        perror("sem_init()");
        exit(EXIT_FAILURE);
    }

    return sem;
}

/*crea e inizializza contatore condiviso tra i processi*/
int *initialize_counter() 
{
    /*creazione contatore condiviso*/
    int *n = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (n == MAP_FAILED) {
    	perror("mmap()");
    	exit(EXIT_FAILURE);
	}

	*n = 0;

    return n;
}

/*crea e inizializza struttura dati data_process*/
void initialize_DP()
{
    /*creazione struttura dati condivisa*/
	DP = mmap(NULL, sizeof(struct data_process), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (DP == MAP_FAILED) {
        perror("mmap in data_process");
        exit(EXIT_FAILURE);
    }

	DP->nThTot = 0;
	DP->nThIdle = 0;

}


/*invia un segnale al processo con il PID specificato*/
void send_signal(int sig, pid_t pid)
{
    if (kill(pid, sig) == -1) {
        print("Error send signal SIGUSR1", NULL);
        exit(EXIT_FAILURE);
    }
}

/*ciclo di vita di un thread worker*/
void *Thread_Main(void *arg)
{
    (void) arg;
    /*dealloca le risorse associate al thread quando esso termina*/
    pthread_detach(pthread_self());
    /*fd della connessione da servire*/
    int connfd;

    /*acquisisco il lock sul mutex_connfd*/
    pthread_mutex_lock(&mutex_connfd);
    /*attendo che ci sia una nuova connessione*/
    while (iLess == iMore) {
        pthread_cond_wait(&cond_connfd, &mutex_connfd);
    }
    /*prelevo la prossima connessione da servire*/
    connfd = ConnectionList[iLess];
    /*aggiorno la posizione su ConnectionList*/
    if (++iLess == THREADLIMIT + 1) {
        /*ritorna alla pos iniziale del buffer circolare*/    
        iLess = 0;                  
    }
    /*rilascio il lock sul mutex_connfd*/
    pthread_mutex_unlock(&mutex_connfd);

    /*chiamata a work() per la gestione delle richieste HTTP */
    work(connfd, LogFd, sem_convert);

    /*chiudi la connessione*/
    close(connfd);


    lock_sem(sem_dataProcess);
    pthread_mutex_lock(&mutex_connfd);
    lock_sem(sem_totIdle);
    lock_sem(sem_totThread);
    lock_sem(sem_totProcess);
    DP->nThTot--;
    (*TotThreads)--;


   /*il thread worker verifica se può terminare l'intero processo a cui è associato*/
    if ((iLess == iMore && DP->nThTot == DP->nThIdle && ((*TotIdleThreads) - DP->nThIdle) >= MINIDLETHREAD && (*TotProcess) > STARTSERVER) 
        || (DP->nThTot == 0 && (*TotProcess) > STARTSERVER)) {

        /*aggiorna i contatori totali*/
        (*TotIdleThreads) = (*TotIdleThreads) - DP->nThIdle;
        (*TotThreads) = (*TotThreads) - DP->nThIdle;
        (*TotProcess)--;

        unlock_sem(sem_totProcess);
        unlock_sem(sem_totThread);
        unlock_sem(sem_totIdle);
        pthread_mutex_unlock(&mutex_connfd);
        unlock_sem(sem_dataProcess);

        /*termina il processo */
        exit(EXIT_SUCCESS);
    }

    /*se necessario il thread worker crea thread idle per mantenere il numero totale degli idle maggiore o uguale a MINIDLETHREAD */
    if ((*TotIdleThreads) < MINIDLETHREAD && DP->nThTot < THREADLIMIT) {

        int i;
        pthread_t tid;

        /*crea il numero di threads necessario*/
        if ((MINIDLETHREAD - (*TotIdleThreads)) < THREADLIMIT - DP->nThTot) {
            int cont = 0;
            for (i = 0; i < (MINIDLETHREAD - (*TotIdleThreads)); i++) {
                if (pthread_create(&tid, NULL, &Thread_Main, NULL) != 0) {
                    perror("Error in pthread_create()");
                } 
                else {
                    cont++;
                }
            }
            (*TotThreads) = (*TotThreads) + cont;
            DP->nThIdle = DP->nThIdle + cont;
            DP->nThTot = DP->nThTot + cont;
            (*TotIdleThreads) = (*TotIdleThreads) + cont;
        }
        /*se non è possibile crearli tutti, creane quanto possibile*/
        if ((MINIDLETHREAD - (*TotIdleThreads)) >= THREADLIMIT - DP->nThTot) {
            int i, cont = 0;

            for (i = 0; i < THREADLIMIT - DP->nThTot; i++) {
                if (pthread_create(&tid, NULL, &Thread_Main, NULL) != 0) {
                    perror("Error in pthread_create()");
                } 
                else {
                    cont++;
                }
            }

            (*TotIdleThreads) = (*TotIdleThreads) + cont;
            DP->nThIdle = DP->nThIdle + cont;
            (*TotThreads) = (*TotThreads) + cont;
            DP->nThTot = DP->nThTot + cont;


        }
    }

    unlock_sem(sem_totProcess);
    unlock_sem(sem_totThread);
    unlock_sem(sem_totIdle);
    pthread_mutex_unlock(&mutex_connfd);
    unlock_sem(sem_dataProcess);

    /*terminazione del thread*/
    pthread_exit(NULL);
}


/*ciclo di esecuzione di un processo figlio*/
int Child_Main(int listenFd)
{   
    /*registra il gestore di default per il segnale SIGCHLD*/
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
        perror("error in signal()");
        exit(EXIT_FAILURE);
    }

    /*inizializzo struttura dati data_process*/
    initialize_DP();
    /*inizializzo semaforo condiviso tra i threads*/
    sem_dataProcess = initialize_semaphore_t();

    pthread_t tid;
    int i, cont=0, connfd;

    /*genero un numero iniziale THREADSPERCHILD di threads*/
    for (i = 0; i < THREADSPERCHILD; i++) {
        if (pthread_create(&tid, NULL, &Thread_Main, NULL) != 0) {
            perror("Error in pthread_create()");
        } else {
            cont++;
        }
    }
    
    lock_sem(sem_dataProcess);
    lock_sem(sem_totIdle);
    lock_sem(sem_totThread);

    /*aggiorno contatori*/
    DP->nThTot = cont;
    DP->nThIdle = cont;
    *TotIdleThreads += cont;
    *TotThreads += cont;

    unlock_sem(sem_totThread);
    unlock_sem(sem_totIdle);
    unlock_sem(sem_dataProcess);

    fd_set FD_Set;

    for(;;) {

        FD_ZERO(&FD_Set);
        FD_SET(listenFd, &FD_Set);

        /* il processo rimane in attesa fino a quando non viene
        individuata la presenza di connessioni pronte */
        if((select(listenFd + 1, &FD_Set, NULL, NULL, NULL)) < 0) {
            print("Error in select()", NULL);
            continue;
        }

        /*tento di acquisire il lock sul semaforo di listenFd*/
        errno = 0;
        if (sem_trywait(sem_listen) == -1) {  
            if (errno == EAGAIN) {
                /*lock su listenFd occupato: dormi 1 ms dando a qualcun'altro l'opportunità di prendere la connessione*/
                usleep(1000);

                continue;
            } else {
                perror("sem_trywait()");
                exit(EXIT_FAILURE);
            }
        }

        
        /*lock su listenFd acquisito */
        lock_sem(sem_dataProcess);
        lock_sem(sem_totIdle);

        /*controllo se posso effettivamente accettare la connessione*/
        if ((DP->nThTot == THREADLIMIT && DP->nThIdle == 0) || (DP->nThTot < THREADLIMIT && DP->nThIdle == 0 && *TotIdleThreads == MAXIDLETHREAD)) {
            /*raggiunto max numero di thread per processo: dormi 1 ms dando a qualcun'altro l'opportunità di prendere la connessione*/
            unlock_sem(sem_totIdle);
            unlock_sem(sem_dataProcess);
            unlock_sem(sem_listen);
            
            usleep(1000);
            
            continue;
        }

        unlock_sem(sem_totIdle);
        
        errno = 0;

        /*accetta la connessione*/ 
        connfd = accept(listenFd, NULL, NULL);
        /*libera lock su listenFd */
        unlock_sem(sem_listen);

        /*controllo eventuali errori*/
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                unlock_sem(sem_dataProcess);
                continue;
            }
            unlock_sem(sem_dataProcess);
            continue;
        }

        lock_sem(sem_totIdle);
        lock_sem(sem_totThread);

        /*aggiungo connfd nell'array ConnectionList[] condiviso*/
        pthread_mutex_lock(&mutex_connfd);
        ConnectionList[iMore] = connfd;
        if (++iMore == THREADLIMIT + 1)
            /*torno all'inizio del buffer circolare*/
            iMore = 0;
        if (iMore == iLess)
            /*buffer circolare pieno*/
            print("iMore = iLess = ", &iMore);

        /*segnalo la presenza di una nuova connessione ai threads worker*/
        pthread_cond_signal(&cond_connfd);      
        pthread_mutex_unlock(&mutex_connfd);


        /*creazione del thread idle sostitutivo*/
        if ((DP->nThTot < THREADLIMIT && DP->nThIdle == 0) || (DP->nThTot < THREADLIMIT && *TotIdleThreads <= MINIDLETHREAD)) {
            if (pthread_create(&tid, NULL, &Thread_Main, NULL) != 0) {
                perror("Error in pthread_create()");
            } 
            else {
                /*aggiorna contatori*/
                (*TotThreads)++;
                (*TotIdleThreads)++;
                DP->nThIdle++;
                DP->nThTot++;
            }

        }
        /*se il processo non può creare il thread idle sostitutivo, invia il segnale SIGUSR1 al processo Master*/
        else if (DP->nThTot == THREADLIMIT && *TotIdleThreads <= MINIDLETHREAD) {
            /*invio segnale*/
            send_signal(SIGUSR1, Master_pid);
        }
        
        DP->nThIdle--;
        (*TotIdleThreads)--;

        unlock_sem(sem_totThread);
        unlock_sem(sem_totIdle);
        unlock_sem(sem_dataProcess);
        
    }

    return EXIT_SUCCESS;
}

/*creazione e avvio di un processo figlio*/
void Make_Child(int listenFd) {

    /*creazione processo figlio*/
    pid_t pid = fork();

    if(pid == -1) {
        perror("Error in fork()");
        exit(EXIT_FAILURE);
    }

    if(pid == 0) {
        /*il processo figlio inizia il suo ciclo di vita*/        
   		Child_Main(listenFd);
    }

    /*il processo Master ritorna*/
}

/*gestore dei segnali SIGUSR1 e SIGCHLD*/
void usr_handler(int sig) {

    /*se arriva di un segnale SIGCHLD*/
    if(sig == SIGCHLD) {
        int status;
        pid_t pid;

        /*attesa ed eliminazione dei processi zombie*/
        while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0) { }
    }
    else if (sig == SIGUSR1) {
        
        lock_sem(sem_totProcess);
        /*se il numero di processi è minore del limite consentito*/
        if ((*TotProcess) < SERVERLIMIT) {
            /*creo un nuovo processo*/
            Make_Child(listenFd);
            (*TotProcess)++;
        }
        unlock_sem(sem_totProcess);
    }
    
    return;
}


/*registrazione del gestore dei segnali*/
void register_sighandler(void)
{
    if (signal(SIGUSR1, usr_handler) == SIG_ERR || signal(SIGCHLD, usr_handler) == SIG_ERR) {
        print("Error in signal()", NULL);
        exit(EXIT_FAILURE);
    }
}

/*Thread utilizzato per la gestione della lista cache*/
void *Thread_Cache(void *arg)
{
    arg = arg;
    /*puntatore al primo elemento della lista*/
    struct node_t *list_head = NULL;
    /*numero di elementi nella cache*/
    unsigned short int cache_count = 0; 
    /*indica il raggiungimento del limite della cache (1 = full)*/
    unsigned short int cache_full = 0;

    /*conterrà il comando di rimozione dell'immagine superflua*/
    char *command = alloca(100 * sizeof(char));;

    reset_buffer(CI->image, 100);
    *(CI->file_sent) = 0;
    
    for(;;) {

        reset_buffer(command, 100);

        pthread_mutex_lock(CI->mutex_cache);

        /*rimani in attesa di una nuova immagine da inserire nella cache*/
        while (*(CI->image) == '\0') {
            pthread_cond_wait(CI->cond_cache, CI->mutex_cache);
        }

        /*'new_image' indica che l'immagine richiesta non era presente nella cache*/
        int new_image = refresh_list(CI->image, &list_head);

        /*incrementa il numero di elementi della cache*/
        if(new_image == 1 && cache_full == 0) 
            cache_count++;

        /*notifica il raggiungimento del limite della cache*/
        if(cache_count > CACHELIMIT) {
            cache_full = 1;
            print("\nCACHE FULL", NULL);
        }
        
        /*notifica la presenza del file richiesto dal client nella cache*/
        if (new_image == 0)
            print("\nEXISTING FILE", NULL);

        /*se il limite è stato raggiunto, cerca l'immagine da eliminare*/
        if(cache_full == 1 && new_image == 1) 
            delete(&list_head, command); 

        /*esegui il comando di rimozione dell'immagine*/
        lock_sem(sem_convert);
        errno = 0;

        if (system(command) == -1) {
            if(errno != ECHILD) {
                perror("Error in system()");
                pthread_exit(NULL);
            } 
        }

        unlock_sem(sem_convert);

        reset_buffer(CI->image, 100);

        pthread_mutex_unlock(CI->mutex_cache);
        pthread_mutex_lock(CI->mutex_cache);

        /*attendi l'avvenuta spedizione del file
        per stampare la lista cache aggiornata*/
        while(*(CI->file_sent) == 0) {
            pthread_cond_wait(CI->cond_cache, CI->mutex_cache);
        }

        print_list(list_head);

        print("FILE SENT", NULL);
        print("\n-------------------\n", NULL);

        *(CI->file_sent) = 0;

        pthread_mutex_unlock(CI->mutex_cache);
    }

    pthread_exit(NULL);
} 

/*ciclo di vita del processo Master*/
int main(int argc, char **argv) {
	
	int listenFd, i;
	struct sockaddr_in servaddr;
    int port = 0; 
    debug = 0;
    pthread_t tid;

    /*verifica se l'utente ha inserito l'opzione di debug*/
    if(argc == 2) {
        
        if(strcmp(argv[1], "-d") != 0) {
            fprintf(stderr, "Usage:\n<%s -d> or\n<%s -p PORT> or\n<%s -d -p PORT> or\n<%s -p PORT -d>\n", argv[0], argv[0], argv[0], argv[0]);
            return -1;
        }
        else 
            debug = 1;      
    }
    /*verifica se l'utente ha inserito l'opzione per l'inserimento della porta*/  
    else if(argc == 3) {
        if(strcmp(argv[1], "-p") == 0) {
            port = atoi(argv[2]);
            /*verifica che la porta inserita sia un intero*/
            if(port == 0) {
                fprintf(stderr, "number of port must to be an integer !!\n");
                return -1;
            }
            /*verifica che la porta inserita sia idonea*/
            if(port < 1024 || port > 65535) {
                fprintf(stderr, "number of port must to be between 1024 and 65535\n");
                return -1;
            }
        }
        else {
            fprintf(stderr, "Usage:\n<%s -d> or\n<%s -p PORT> or\n<%s -d -p PORT> or\n<%s -p PORT -d>\n", argv[0], argv[0], argv[0], argv[0]);
            return -1;
        }
    }
    /*verifica se l'utente ha inserito (contemporaneamente) le opzioni di
     debug e dell'inserimento della porta*/
    else if(argc == 4) {
        if(strcmp(argv[1], "-d") == 0 && strcmp(argv[2], "-p") == 0) {
            debug = 1;
            port = atoi(argv[3]);
            if(port == 0) {
                fprintf(stderr, "number of port must to be an integer !!\n");
                return -1;
            }
            if(port < 1024 || port > 65535) {
                fprintf(stderr, "number of port must to be between 1024 and 65535\n");
                return -1;
            }
        }
        else {
            fprintf(stderr, "Usage:\n<%s -d> or\n<%s -p PORT> or\n<%s -d -p PORT> or\n<%s -p PORT -d>\n", argv[0], argv[0], argv[0], argv[0]);
            return -1;
        }
    }
    else if(argc > 4) {
        fprintf(stderr, "Usage:\n<%s -d> or\n<%s -p PORT> or\n<%s -d -p PORT> or\n<%s -p PORT -d>\n", argv[0], argv[0], argv[0], argv[0]);
        return -1;
    }

    /*se la porta non è stata impostata dall'utente,
    impostala di default*/
    if(port == 0) 
        port = DEFAULT_PORT;

    if(debug == 1)
        print("---DEBUG MODE---", NULL);
    
    print("port: ", &port);

    /*apertura passiva del socket*/
    listenFd = set_socket(servaddr, port);

    /*semaforo sulla listenFd */
    sem_listen = initialize_semaphore();

    /*semaforo su totIdleThread */
    sem_totIdle = initialize_semaphore();

    /*semaforo su totThread */
    sem_totThread = initialize_semaphore();

    /* semaforo su totProcess */
    sem_totProcess = initialize_semaphore();

    /*semaforo su convert */
    sem_convert = initialize_semaphore();

    /*inizializzo i contatori*/
	TotIdleThreads = initialize_counter();

    TotThreads = initialize_counter();

    TotProcess = initialize_counter();

    /*inizializzo la struttura contenente
    le info della lista cache*/
    initialize_CI();

    print("Parameter initialized", NULL);

	LogFd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
	if(LogFd == -1) {
		perror("Error in open LogFd");
		return (EXIT_FAILURE);
	}

    print("LOG opened", NULL);

    /*memorizzo il PID del processo Master*/
    Master_pid = getpid();

    /*registro il gestore dei segnali*/
    register_sighandler();

    /*creo e avvio un numero iniziale STARTSERVER di processi figli*/
	for(i=0; i < STARTSERVER; i++) {
		Make_Child(listenFd);
		lock_sem(sem_totProcess);
        (*TotProcess)++;
        unlock_sem(sem_totProcess);
    }

    print("Childs created", NULL);

    /*Creazione del thread di aggiornamento della lista cache*/
    if (pthread_create(&tid, NULL, &Thread_Cache, NULL) != 0) {
        perror("Error in pthread_create()");
    } 

    for(;;) {

        /*attendo l'arrivo di un segnale*/
        pause();

    }
	
	return EXIT_SUCCESS;
}
