#ifndef CACHELIST
#define CACHELIST

/* Prototipo della funzione print() definita in 'Functions.h' */
void print(char *string, int *i);

/* Struttura relativa ad un nodo della lista */
struct node_t {
    char *str;  //nome immagine
    struct node_t *next; //nodo successivo
};

/* Alloca la quantità di spazio necessaria
a contenere una variabile di tipo 'node_t' */ 
struct node_t *alloc_node(void)
{
    struct node_t *p;

    p = malloc(sizeof(struct node_t));
    if (p == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    p->str = calloc(20, sizeof(char));

    return p;
}

/* Libera la memoria occupata dal nodo indicato */
void free_node(struct node_t *p)
{
    free(p->str);
    free(p);
}

/* Inserisce la stringa 's' dopo il nodo 'pnext' */
void insert_after_node(char *s, struct node_t **pnext)
{
    struct node_t *new;
    new = alloc_node();

    new->str = calloc(100, sizeof(char));

    strcpy(new->str, s);

    new->next = *pnext;
    *pnext = new;
}

/* Rimuove il nodo successivo a 'pnext' */
void remove_after_node(struct node_t **pnext)
{
    struct node_t *d = *pnext;
    *pnext = d->next;
    
    free_node(d);
}

/* Aggiorna la lista cache */ 
int refresh_list(char *s, struct node_t **pHead)
{
    struct node_t *p;

    unsigned short int new_image = 0; //indica l'esistenza (1) o meno (0) nella lista
    unsigned short int found = 0; //indica se la stringa 's' era già presente nella lista

    /*Scandisci la lista e cerca una stringa uguale a 's'*/
    for (p = *pHead; p != NULL; pHead = &p->next, p = p->next) {
        /*se tale stringa è stata trovata...*/
        if(strcmp(p->str, s) == 0) {
            /*...se era in coda, mantieni l'attuale lista e ritorna subito*/
            if(p->next == NULL)
                return 0;
            /*...rimuovi tale nodo dalla posizione corrente...*/
            remove_after_node(pHead);
            found = 1;
        }
    }

    if(!found)
        new_image = 1;

    /*...inseriscilo in coda*/
    insert_after_node(s, pHead);

    return new_image;
}

/* Stampa le stringhe della lista puntata da 'h' */
void print_list(struct node_t *h)
{
    struct node_t *p;

    print("\nCACHE LIST:", NULL);

    for (p = h; p != NULL; p = p->next)
        printf("%s\n", p->str);

    printf("\n");
}

/* Torna il comando di rimozione dell'immagine posta
in cima alla lista (la meno utilizzata) */
char *delete(struct node_t **pHead, char *command)
{
	struct node_t *p = *pHead;

	char c[200];
    sprintf(c, "DELETING: %s...", p->str);
    print(c, NULL);
    strcat(command, "rm ");
    strcat(command, p->str);

	remove_after_node(pHead);
    
    return command;
}

#endif