#ifndef FUNCTIONS
#define FUNCTIONS

struct CacheInfo {
    char *image;
    unsigned short int *file_sent; 
    pthread_mutex_t *mutex_cache;
    pthread_cond_t *cond_cache;
} *CI;

/* Stampa la stringa inserita, con un eventuale intero, se la modalità
debug è attiva */
void print(char *string, int *i)
{
    if(debug == 1){
        if(i != NULL)
            fprintf(stderr, "%s%d\n", string, *i);
        else
            fprintf(stderr, "%s\n", string);
    }
}

/* Scrive nel File Descriptor indicato da "fd" il contenuto della variabile "buf"
di dimensioni pari a "n" */
int writen(int fd, const void *buf, int n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    nleft = n;
    ptr = buf;

    while(nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if((nwritten < 0) && (errno == EINTR))
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    return nleft;
}

/* Applica un lock() al semaforo "sem" */
void lock_sem(sem_t *sem)
{
    if (sem_wait(sem) == -1) {
        perror("sem_wait()\n");
        exit(EXIT_FAILURE);
    }
}

/* Sblocca il semaforo "sem" */
void unlock_sem(sem_t *sem)
{
    if (sem_post(sem) == -1) {
        perror("sem_post()\n");
        exit(EXIT_FAILURE);
    }
}

/* Invia un messaggio di risposta HTTP alla socket indicata da "connfd"
in caso di operazione di richiesta-risposta andata a buon fine. La 
variabile "flag" indica se si è trattata di una richiesta HTML (Index.html)
o di un'immagine JPEG */
int send200oK(int connfd, int flag)
{
    char *buffer = alloca(150 * sizeof(char));
    strcpy(buffer, "HTTP/1.1 200 OK\r\n");
    struct tm *tmp;
    time_t current_time;

    time(&current_time);
    tmp = localtime(&current_time);
    char date[50];
    strftime(date, 50, "%a, %d %b %Y %T %Z", tmp);
    strcat(buffer, date);

    /* Il file richiesto è un'immagine JPEG */
    if(flag == 1)
        strcat(buffer, "\r\nTransfer-Encoding: chunked\r\nContent-Type: image/jpeg\r\n\r\n");
    
    /* Il file richiesto è di tipo HTML */
    else
        strcat(buffer, "\r\nTransfer-Encoding: chunked\r\nContent-Type: text/html\r\n\r\n");

    if (writen(connfd, buffer, strlen(buffer)) < 0) {
        perror("error in send200oK\n");
        return -1;
    }

    return 0;
}

/* Invia un messaggio di risposta HTTP alla socket indicata da "connfd"
in caso di richiesta HTTP errata */
int send400BadRequest(int connfd)
{
    char *buffer = alloca(150 * sizeof(char));
    sprintf(buffer, "HTTP/1.1 400 Bad Request\r\nServer: %s\r\nDate: ", NameServer);
    struct tm *tmp;
    time_t current_time;

    time(&current_time);
    tmp = localtime(&current_time);
    char date[50];
    strftime(date, 50, "%a, %d %b %Y %T %Z", tmp);
    strcat(buffer, date);
    strcat(buffer, "\r\n\r\n");

    if (writen(connfd, buffer, strlen(buffer)) < 0) {
        perror("error in send400BadRequest\n");
        return -1;
    }

    return 0;
}

/* Invia un messaggio di risposta HTTP alla socket indicata da "connfd"
in caso di risorsa richiesta non esistente */
int send404NotFound(int connfd)
{
    char *buffer = alloca(150 * sizeof(char));
    sprintf(buffer, "HTTP/1.1 404 Not Found\r\nServer: %s\r\nDate: ", NameServer);
    struct tm *tmp;
    time_t current_time;

    time(&current_time);
    tmp = localtime(&current_time);
    char date[50];
    strftime(date, 50, "%a, %d %b %Y %T %Z", tmp);
    strcat(buffer, date);
    strcat(buffer, "\r\n\r\n");

    if (writen(connfd, buffer, strlen(buffer)) < 0) {
        perror("error in send404NotFound\n");
        return -1;
    }

    return 0;
}

/* Invia un messaggio di risposta HTTP alla socket indicata da "connfd"
in caso di */

int send408Timeout(int connfd)
{
    char *buffer = alloca(150 * sizeof(char));
    sprintf(buffer, "HTTP/1.1 408 Request Timeout\r\nServer: %s\r\nDate: ", NameServer);
    struct tm *tmp;
    time_t current_time;

    time(&current_time);
    tmp = localtime(&current_time);
    char date[50];
    strftime(date, 50, "%a, %d %b %Y %T %Z", tmp);
    strcat(buffer, date);
    strcat(buffer, "\r\n\r\n");

    if (writen(connfd, buffer, strlen(buffer)) < 0) {
        perror("error in send408Timeout\n");
        return -1;
    }

    return 0;
}

/* Invia un messaggio di risposta HTTP alla socket indicata da "connfd"
in caso di errore imprevisto che impedisce il servizio richiesto */
int send500ServerError(int connfd)
{
    char *buffer = alloca(150 * sizeof(char));
    sprintf(buffer, "HTTP/1.1 500 Internal Server Error\r\nServer: %s\r\nDate: ", NameServer);
    struct tm *tmp;
    time_t current_time;

    time(&current_time);
    tmp = localtime(&current_time);
    char date[50];
    strftime(date, 50, "%a, %d %b %Y %T %Z", tmp);
    strcat(buffer, date);
    strcat(buffer, "\r\n\r\n");

    if (writen(connfd, buffer, strlen(buffer)) < 0) {
        perror("error in send500ServerError\n");
        return -1;
    }

    return 0;
}

/* Ritorna il metodo GET o HEAD (corrispondente alla prima parola)
della richiesta HTTP */
char *get_method(char *buffer) 
{ 
	return strtok(buffer," ");
}

/* Ritorna l'URI del file richiesto dal client */
char *getFileName(char *buffer)
{   
    /* prendo tutto ciò che è prima dello '/' */
	char *getLine = strstr(buffer, ""); 

    /* prendo un eventuale percorso che c'è dopo lo '/' */
    sscanf(getLine, "%*s%s ", getLine); 

    /*se dopo lo '/' non c'è nulla allora ritorna l'index, 
    altrimenti ritorna lo URL dell'immagine */ 
    sscanf(getLine, "/%s", getLine);  
    
    if (strcmp(getLine, "/") == 0) 
        return "Index.html";  
    else
        return getLine;  
}

/* Apri il file indicato in "nm_file" con la modalità
descritta in "type" */
FILE* open_file(char *nm_file, char *type) 
{
	FILE *fp;
	fp = fopen(nm_file, type);   

	if (fp == NULL) 
		perror("Error in fopen()\n");
	
	return fp;
}

/* Azzera il buffer puntato da "buffer" di dimensioni pari a "n" */
void reset_buffer(char *buffer, int n)
{
    int i;

    for (i = 0; i < n; i++)
        buffer[i] = '\0';
}

/*Creazione e apertura passiva del socket di ascolto*/
int set_socket(struct sockaddr_in servaddr, int port)
{

	int listenfd;

    /*creazine del socket
    AF_INET - famiglia di protocolli
    SOCK_STREAM - tipo di socket
    */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
	    perror("error in socket\n");
	    exit(EXIT_FAILURE);
	}

    /*imposta sul listenfd il flag O_NONBLOCK*/
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

	  
	memset((void *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
    /*il server accetta connessioni su una qualunque delle sue intefacce di rete*/
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); /* il server accetta connessioni su una qualunque delle sue intefacce di rete */
	
    /*assegna il numero di porta: DEFAULT se non specificata dall'utente*/
    servaddr.sin_port = htons(port); /*numero di porta del server (di default)*/

	/* assegna l'indirizzo al socket */
	if ((bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) { 
		perror("error in bind()\n");
		exit(EXIT_FAILURE);
	}
    /*pone il socket specificato in modalità passiva con una coda per le connessioni in arrivo di lunghezza pari a backlog*/
	if (listen(listenfd, BACKLOG) < 0) {
		perror("error in listen()\n");
		exit(EXIT_FAILURE);
	}

	return listenfd;
}

/*crea la stringa che dovrà poi essere eseguita con la system() per la conversione dell'immagine*/
char *getComando(char *image, char *immagineOutput, char *qString, char *resolution)
{
    char *comando = calloc(200, sizeof(char));
    strcat(comando, "convert -quality ");
    strcat(comando, qString);
    strcat(comando, " ");

    if (resolution != NULL) {
        char *r = alloca(10 * sizeof(char));
        sprintf(r, "%s", resolution);
        strcat(comando, "-resize ");
        strcat(comando, r);
        strcat(comando, " ");
    }

    strcat(comando, image);
    strcat(comando, " ");
    strcat(comando, immagineOutput);

    return comando;
}

/*ottiene lo UserAgent dalla richiesta HTTP contenuta nel buffer*/
char *get_userAgent(char *buffer)
{
    char *userAgent;
    if ((userAgent = strstr(buffer, "User-Agent: ")) == NULL) {
        return NULL;
    }
    
    strtok(userAgent, " ");
    char *t = strtok(NULL, "\r\n");
    return t;
}

/*estrae la stringa "GET" dal buffer*/
char *get_Get(char *buffer)
{
    return strtok(buffer, "\r\n");
}

/*estrae il nome dell'HOST dal buffer*/
char *get_Host(char *buffer)
{
    char *Host;

    if ((Host = strstr(buffer, "Host: ")) == NULL) 
        return NULL;

    strtok(Host, " ");

    return strtok(NULL, "\r\n");
}

/* Cerca lo UA nel DB di wurfl e ritorna la risoluzione corrispondente al dato UA*/
char *get_resolution_fromDB1(char *UserAgent)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    char *resolution = calloc(10, sizeof(char));
    int width, height;

    if (sqlite3_open("DevicesDB", &db)) 
        perror("Error in sqlite3_open()\n");

    char *query = alloca((100 + strlen(UserAgent)) * sizeof(char));

    reset_buffer(query, strlen(query));
    strcat(query, "SELECT width, height FROM infodevices WHERE user_agent = ");
    strcat(query, "\"");
    strcat(query, UserAgent);
    strcat(query, "\"");

    int error = sqlite3_prepare_v2(db, query, -1, &res, NULL);

    if (error != SQLITE_OK) {
        print("Error in sqlite_prepare get_resolution_fromDB1", NULL);
        sqlite3_finalize(res);
        sqlite3_close(db);

        return NULL;
    }

    if (sqlite3_step(res) == SQLITE_ROW) {

        width = sqlite3_column_int(res, 0);
        height = sqlite3_column_int(res, 1);
        sqlite3_finalize(res);
        sqlite3_close(db);
        sprintf(resolution, "%dx%d", height, width);
        print(resolution, NULL);
        return resolution;
    }

    print("get_resolution_fromDB1: RESOLUTION NOT FOUND", NULL);
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return NULL;
}

/* Ottieni il nome_dispositivo mediante la scomposizione dello UA e cerca nel DB la risoluzione corrispondente al dato device*/
char *get_resolution_fromDB2(char *UserAgent)
{
    char *device;
    char *string = calloc(100, sizeof(char));
    reset_buffer(string, 100);
    device = os_find(UserAgent);

    sqlite3 *db;
    sqlite3_stmt *res;
    char *resolution = calloc(10, sizeof(char));
    int width, height;

    if (sqlite3_open("DevicesDB", &db)) 
        perror("Error in sqlite3_open()\n");

    char *query = alloca((100 + strlen(device)) * sizeof(char));

    reset_buffer(query, strlen(query));
    strcat(query, "SELECT width, height FROM infodevices WHERE device = ");
    strcat(query, "\"");
    strcat(query, device);
    strcat(query, "\"");

    int error = sqlite3_prepare_v2(db, query, -1, &res, NULL);

    if (error != SQLITE_OK) {
        print("Error in sqlite_prepare get_resolution_fromDB2", NULL);
        sqlite3_finalize(res);
        sqlite3_close(db);

        return NULL;
    }

    if (sqlite3_step(res) == SQLITE_ROW) {

        width = sqlite3_column_int(res, 0);
        height = sqlite3_column_int(res, 1);
        sqlite3_finalize(res);
        sqlite3_close(db);
        sprintf(resolution, "%dx%d", height, width);
        sprintf(string, "device: %s", device);
        print(string, NULL);
        sprintf(string, "resolution: %s", resolution);
        print(string, NULL);
        return resolution;
    }

    print("get_resolution_fromDB2: RESOLUTION NOT FOUND", NULL);
    sqlite3_finalize(res);
    sqlite3_close(db);
    
    return NULL;
}

/*estrai dal buffer la qualità necessaria per la conversione dell'immagine*/
float get_quality(char *buf)
{
    char *get_line;
    char *temp;
    char *q;
    char *p;
    float quality = 1;

    if ((get_line = strstr(buf, "Accept: ")) == NULL)
        return quality;

    else 
        sscanf(get_line, "%*s%s", get_line);

    if ((temp = strstr(get_line, "image/")) == NULL) {
        if ((temp = strstr(get_line, "*/*")) == NULL) {
            perror("Errore in strstr()\n");
        } 
        else {
            if ((q = strstr(temp, "q=")) != NULL) {
                sscanf(q, "%*[q=]%s", q);
                if ((quality = strtof(q, &p)) == 0)
                    perror("Error in strtof()\n");
            }
        }
    } 
    else {
        if ((q = strstr(temp, "q=")) != NULL) {
            sscanf(q, "%*[q=]%s", q);
            if ((quality = strtof(q, &p)) == 0)
                perror("Error in strtof()\n");
        }
    }

    return quality;
}

/*crea il nome con cui verrà salvata l'immagine una volta convertita*/
char *get_newName(char *img, char *quality_string, char *resolution)
{
	char *temp = alloca(50 * sizeof(char));
	strcpy(temp, img);
	char *output = calloc((50 + 10), sizeof(char));
	temp = strtok(temp, ".");
	strcat(output, temp);
	strcat(output, "_");
	strcat(output, quality_string);

	if (resolution != NULL) {
        strcat(output, "_");
        strcat(output, resolution);
	}

	strcat(output, ".jpg");

	return output;
}

/*scrive nel file log.txt le informazioni sui trasferimenti effettuati*/
void writeLog(char *buffer, int LogFd, int status)
{
    char *buffer2 = alloca(strlen(buffer) * sizeof(char));
    char *temp; 

    strcpy(buffer2, buffer);
    temp = get_Get(buffer2);
    char *GET = alloca(strlen(temp) * sizeof(char));
    strcpy(GET, temp);

    strcpy(buffer2, buffer);
    temp = get_Host(buffer2);
    char *HOST = alloca(strlen(temp) * sizeof(char));
    strcpy(HOST, temp);

    strcpy(buffer2, buffer);
    temp = get_userAgent(buffer2);
    char *userAgent = alloca(strlen(temp) * sizeof(char));
    strcpy(userAgent, temp);

    struct tm *t;
    time_t current_time;
    time(&current_time);
    t = localtime(&current_time); 

    char data[50];
    strftime(data, 50, "%a, %d %b %Y %T %Z", t); 

    char *stat = alloca(4 * sizeof(char));
    sprintf(stat, "%d", (int) (status));

    /*formatta la stringa che verrà scritta su log.txt*/
    char *out = alloca((strlen(HOST) + strlen(GET) + strlen(userAgent) + strlen(stat) + strlen(data) + 20) * sizeof(char));
    reset_buffer(out, (strlen(HOST) + strlen(GET) + strlen(userAgent) + strlen(stat) + strlen(data) + 20) * sizeof(char));
    
    strcat(out, HOST);
    strcat(out, " [");
    strcat(out, data);
    strcat(out, "] \"");
    strcat(out, GET);
    strcat(out, "\" ");
    strcat(out, stat);
    strcat(out, " \"");
    strcat(out, userAgent);
    strcat(out, "\"\n");
    
    errno = 0;

    /*sincronizza le scritture sul file*/
    if (flock(LogFd, LOCK_EX) < 0) {
        perror("error in flock(), LOCK_EX");
    }

    /*scrittura su file*/
    if (writen(LogFd, out, strlen(out)) < 0) {
        perror("error in writen\n");
        print("errno = ", &errno);
    }

    if (flock(LogFd, LOCK_UN) < 0) {
        perror("error in flock(), LOCK_UN\n");
    }
    
}

/*spezzetta il buffer letto dal socket in singole richieste HTTP*/
char *tokenRequest(char *buf)
{
    char *end = strstr(buf, "\r\n\r\n");

    if (end != NULL) {
        end = end + 4 * sizeof(char);
    }

    return end;
}

/*spdisco il file sul socket*/
int send_file(int connfd, FILE* fp, int logFd, char *buffer_req) 
{
	char buf[MAXLINE];
	char hex[10];
	int nread, nwritten;

	reset_buffer(buf, MAXLINE);

	while (!feof(fp)) {
	/* Legge nread byte dal file dell'immagine e li posiziona nel buffer */
        if ((nread = fread(buf, 1, MAXLINE - 2, fp)) < 0) {   /* -2 per far spazio a \r\n da inserire in buffer */
            perror("error in fread()\n");
            send500ServerError(connfd);
            writeLog(buffer_req, logFd, 500);

            return -1;
        }

        buf[nread] = '\r';
        buf[nread + 1] = '\n';

        reset_buffer(hex, sizeof(hex));
        sprintf(hex, "%X", nread);
        hex[strlen(hex)] = '\r';
        hex[strlen(hex)] = '\n';       

        /* Invia al client il numero di byte (in esadecimale) del prossimo chunk */
        if (writen(connfd, hex, strlen(hex)) < 0) {
            perror("error in writen1()\n");
            send500ServerError(connfd);
            writeLog(buffer_req, logFd, 500);

            return -1;
        }

        /* Invia al client nread byte dell'immagine */
        if ((nwritten = writen(connfd, buf, nread + 2)) < 0) {   /* il +2 tiene conto di \r\n all'interno di buffer */
            perror("error in writen2()\n");
            send500ServerError(connfd);
            writeLog(buffer_req, logFd, 500);

            return -1;
        }

        reset_buffer(buf, MAXLINE);
    }

    /* Invia al client il numero '0' per indicare la fine dell'invio dell'immagine */
    if (writen(connfd, "0\r\n\r\n", 5) < 0) {
        perror("error in writen3()\n");
        send500ServerError(connfd);
        writeLog(buffer_req, logFd, 500);

        return -1;
    }

    /*notifica l'avvenuta spedizione del file al thread della cache*/
    pthread_mutex_lock(CI->mutex_cache); 

    *(CI->file_sent) = 1;

    pthread_cond_signal(CI->cond_cache);

    pthread_mutex_unlock(CI->mutex_cache);

    return 0;
}


/*gestione delle richieste HTTP*/
int work(int connfd, int logFd, sem_t *sem_convert)
{	
    /*il thread termina rilasciando automaticamente tutte le sue risorse*/
    pthread_detach(pthread_self());  

	int nread, nselect, flag=1;
	float quality;

	/*buffer contenente una singola richiesta HTTP*/
	char *buffer_req = alloca(MAXLINE * sizeof(char));
    /*buffer di lettura dal socket*/
	char *buffer_read = alloca(MAXLINE * sizeof(char));

	char *buffer_temp = alloca(MAXLINE * sizeof(char));

	char *temp;

    char *method = alloca(5 * sizeof(char));
    char *resolution;
    char *image = alloca(50 * sizeof(char));
	char *out_image = alloca(50 * sizeof(char));
	char *quality_s = alloca(4 * sizeof(char));
    char *user_agent = alloca(MAXLINE * sizeof(char));
	char *command;
    char *start, *end;

	FILE *file;
	
	fd_set fdread;
	struct timeval timeout;

	
	for(;;) {

        /*imposta parametri della select*/
		timeout.tv_sec = TIMEOUT;
    	timeout.tv_usec = 0;
        FD_ZERO(&fdread);
    	FD_SET(connfd, &fdread);
    	
    	/*attendi la ricezione di richieste HTTP */
    	if ((nselect = select(connfd + 1, &fdread, NULL, NULL, &timeout)) == 0) {
    	    /*timeout scaduto*/
            send408Timeout(connfd);
    	    return 0;
		}

        if (nselect < 0) {
            perror("error in select()\n");
            send500ServerError(connfd);
            return -1;
        }

        reset_buffer(buffer_read, MAXLINE);

        /*lettura da socket*/        
		if ((nread = read(connfd, buffer_read, MAXLINE)) > 0) {
            start = buffer_read;
			end = tokenRequest(start);

            reset_buffer(buffer_req, MAXLINE);

            /*fin quando esistono nuove richieste HTTP*/
			while(end != NULL) {
				strncpy(buffer_req, buffer_read, (end - start) / sizeof(char));

                reset_buffer(buffer_temp, MAXLINE);

                /*cattura del metodo*/
                strcpy(buffer_temp, buffer_req);
                temp = get_method(buffer_temp);
                reset_buffer(method, 5);
                strcpy(method, temp);

				/*se arriva una richiesta che non sia Get o Head*/
				if(strncmp(method, "GET", strlen(method)) != 0 && strncmp(method, "HEAD", strlen(method)) != 0 ) {
					print("Invalid HTTP Request", NULL);
				    
                    /*notifica che la richiesta non è stata riconosciuta*/
					if(send400BadRequest(connfd) < 0) {
						send500ServerError(connfd);
                        writeLog(buffer_req, logFd, 500);

						return -1;
					}

                    writeLog(buffer_req, logFd, 400);
					start = end;
					end=tokenRequest(start);

                    /*passa alla prossima richiesta*/
					continue;
				}
			
				strcpy(buffer_temp, buffer_req);
                temp = getFileName(buffer_temp);
                reset_buffer(image, 50);
                strcpy(image, temp);


                if (strncmp(image, "favicon.ico", strlen("favicon.ico")) == 0) {

                    start = end;
                    end=tokenRequest(start);

                    /*passa alla prossima richiesta*/
                    continue;
                }

				
				/*gestione del metodo HEAD*/
				if(strncmp(method, "HEAD", strlen(method)) == 0) {
					file = fopen(image, "rb");
					if (file != NULL) {
            			fclose(file);

                        /*notifica la presenza del file*/
            			if (send200oK(connfd, 1) < 0) {
                            send500ServerError(connfd);
                            writeLog(buffer_req, logFd, 500);

                			return -1;
            			}

                        writeLog(buffer_req, logFd, 200);
                    } 
                    else {
                        /*notifica che il file non è stato trovato*/
            			if (send404NotFound(connfd) < 0) {
                			send500ServerError(connfd);
                            writeLog(buffer_req, logFd, 500);

                			return -1;
            			}

                        writeLog(buffer_req, logFd, 404);
                    }
            			
            		start = end;
                    end = tokenRequest(start);

                    /*passa alla prossima richiesta*/
                    continue;
                    
                }  /*chiusura metodo head*/

                /*gestione del metodo GET*/
                file = fopen(image, "rb");

                if (file == NULL) {  
                    /*notifica che il file non è stato trovato*/
                    if (send404NotFound(connfd) < 0) {
                        send500ServerError(connfd);
                        writeLog(buffer_req, logFd, 500);

                        return -1;
                    }

                    writeLog(buffer_req, logFd, 404);
                    start = end;
                    end = tokenRequest(start);

                    /*passa alla prossima richiesta*/
                    continue;
                }

                /*se il file richiesto non è il file Index.html, quindi un'immagine*/
                if (strncmp(image, "Index.html", strlen("Index.html")) != 0) {                        
                    fclose(file);
                    
                    /*cattura della qualità*/
                    strcpy(buffer_temp, buffer_req);
                    quality = get_quality(buffer_temp);

                    /*cattura dello userAgent*/
                    strcpy(buffer_temp, buffer_req);
                    temp = get_userAgent(buffer_temp);
                    reset_buffer(user_agent, MAXLINE);
                    strcpy(user_agent, temp);

                    /*cerca le dimensioni dello schermo 
                    del dispositivo client*/

                    /*cerca in base allo user_agent*/
                    resolution = get_resolution_fromDB1(user_agent);
                    
                    /*cerca in base al nome_dispositivo*/
                    if(resolution == NULL) 
                        resolution = get_resolution_fromDB2(user_agent);
                        
                    /*creazione del nome del file di output*/
                    reset_buffer(out_image, 50);
                    reset_buffer(quality_s, 4);
                    sprintf(quality_s, "%d", (int) (quality * 100));
                    out_image = get_newName(image, quality_s, resolution);

                    /*Prepara il comando di conversione dell'immagine*/
                    command = getComando(image, out_image, quality_s, resolution);
                             
                    /*Notifica il thread della cache che una nuova immagine
                    è stata richiesta. Aggiorna quindi la variabile
                    contenente il nome dell'immagine*/

                    pthread_mutex_lock(CI->mutex_cache); 

                    strncpy(CI->image, out_image, strlen(out_image));

                    pthread_cond_signal(CI->cond_cache);

                    pthread_mutex_unlock(CI->mutex_cache);

                    /*controllo se la copia richiesta è in cache */
                    file = fopen(out_image, "rb");
                    /*la copia non è in cache: eseguo la convert utilizzando system() */
                    if (file == NULL) {
                        lock_sem(sem_convert);
                        /*se durante l'attesa sul semaforo un altro thread ha creato il file*/
                        file = fopen(out_image, "rb");
                        /*se il file ancora non viene trovato*/
                        if (file == NULL) {
                            errno = 0;

                            /*esegui il comando per la conversione*/
                            if (system(command) < 0) {
                                perror("Error in system()\n");
                                print("errno = ", &errno);
                                send500ServerError(connfd);
                                writeLog(buffer_req, logFd, 500);

                                return -1;
                            }
                        
                        }
                        unlock_sem(sem_convert);
                        /*apertura del file convertito*/
                        file = fopen(out_image, "rb");
                        if (file == NULL) {
                            perror("error in fopen()\n");
                            send500ServerError(connfd);
                            writeLog(buffer_req, logFd, 500);

                            return -1;
                        }
                    }

                    flag=1;

                }
                else
                    flag=0;
            
                /*invia il 200 OK*/
                if (send200oK(connfd, flag) < 0) {
                    send500ServerError(connfd);
                    writeLog(buffer_req, logFd, 500);
                    
                    return -1;
                }

                /*invia il file*/
				if (send_file(connfd, file, logFd, buffer_req) == -1) 
					perror("error in send_file()\n");
                				
				fclose(file);
                writeLog(buffer_req, logFd, 200);

                /*passa al richiesta successiva*/
				start = end;
	            end = tokenRequest(start); 

            } /*chiusura while*/    
            
        
        } /*fine read()*/
            
        if (nread == -1) {
        	perror("Error in read()\n"); 
            writeLog(buffer_req, logFd, 500);

            return -1;				
        }
           	
        if (nread == 0)
        	break;
        
    } /*chiusura for()*/


	return 1;
}



#endif
