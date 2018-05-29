#ifndef USERAGENTANALYZER
#define USERAGENTANALYZER

/* Prototipo della funzione print() definita in 'Functions.h' */
void print(char *string, int *i);

/* Il campo relativo al modello del telefono è sempre
 il 5° argomento all'interno della parentesi dello UserAgent */
char *os_linux(char *UA) 
{
    char *device = calloc(50, sizeof(char));
    char str[] = "; ";
    char *token;

    token = strtok(UA, str);

    while(token != NULL) {
        if(strncmp(token, "Build", 5) == 0)
            break;

        device = token;
        token = strtok(NULL, str);
    }
    
    device[strlen(device)] = '\0';

    return device;
}

/* Il campo relativo al modello del telefono viene
 sempre preceduto dalla keyword 'Nokia' */
char *os_symbian(char *UA)  
{
    char *model;
    char *device = calloc(50, sizeof(char));
    unsigned int k, j=0;

    model = strstr(UA, "Nokia");
    model += 5;

    for (k=0; k<strlen(model); k++) {
        if(model[k] == '/') {
            device[j] = '\0';
            break;
        } else {
            device[j] = model[k];
            j++;
        }
    }

    device[strlen(device)] = '\0';

    return device;
}

/* Il campo relativo al modello del telefono viene
 sempre preceduto dalla keyword 'NOKIA;' */
char *os_winPh(char *UA)
{
    char *device = calloc(50, sizeof(char));
    unsigned int i=0;

    char *str = strstr(UA, "NOKIA");
    str += 7;  //togli dalla stringa la keyword "NOKIA"

    while(i < strlen(str)) {  //avanza fin quando non trovi il carattere ')'
        if(str[i] == ')')
            break;

        device[i] = str[i];

        i++;
    }
    
    device[strlen(device)] = '\0';

    return device;
}

/* Il campo relativo al modello del telefono 
è riconoscibile dala keyword 'BlackBerry ' */
char *os_BB(char *UA)  
{
    char *device = calloc(50, sizeof(char));
    unsigned int i = 0;

    char *str = strstr(UA, "BlackBerry ");

    while(i < strlen(str)) {
        if(str[i] == ';')
            break;

        device[i] = str[i];
        i++;
    }

    device[strlen(device)] = '\0';

    return device;
}

/* Cerca all'interno dello UserAgent la stringa
indicante il sistema operativo del dispositivo.
Ritorna il nome del dispositivo */
char *os_find(char *UA)
{
    char *device = calloc(50, sizeof(char));

    if(strstr(UA, "Linux") != NULL) {
        print("\nOS: Linux/Android", NULL);
        device = os_linux(UA);
        
    } else if(strstr(UA, "Symbian") != NULL) {
        print("\nOS: Symbian", NULL);
        device = os_symbian(UA);
        
    } else if(strstr(UA, "Windows Phone") != NULL) {
        print("\nOS: Windows Phone", NULL);
        device = os_winPh(UA); 
        
    } else if(strstr(UA, "BlackBerry") != NULL) {
        print("\nOS: BlackBerry", NULL);
        device = os_BB(UA);
    }

    return device;
}

#endif