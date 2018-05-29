#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* Per ogni 'device' con la rispettiva larghezza_schermo 'w'
e altezza_schermo 'h', crea una riga nel file txt */
void crea_txt(char *device, char *w, char *h) 
{
	int fd;
	char buffer[100];
	sprintf(buffer, "%s (%sx%s)\n", device, w, h);

	/* Apre il file 'databaseWURFL.txt' e se non esiste lo crea */
	fd = open("databaseWURFL.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
	if(fd == -1) {
		perror("Error in open fd");
		return;
	}
	
	/* Scrive la stringa nel file indicato dal file descriptor 'fd' */
	if (write(fd, buffer, strlen(buffer)) < 0) {
        perror("error in write()\n");
        return;
    }

	close(fd);

	return;
}

/* Dalla stringa completa catturata 's', estrapola solo il nome_device
e mettilo in 'b' */
void modifica_stringa(char *s, char *b)
{
	int p = 0, z = 0;

	/* Togli i caratteri inutili */
	while(p < strlen(s)) {
		p++;
		if(s[p] != '"' && s[p] != '/' && s[p] != '>') {
			b[z] = s[p];
			z++;
		}
		if(s[p] == '/') {
			b[z] = '\0';
			break;
		}
	}
}

/* Dalla stringa completa catturata 'info', estrapola solo l'informazione
a noi utile (altezza/larghezza schermo) e mettilo in 'str' */
void cattura_info(char *info, char *str)
{
	int i, j=0, k=0;
	for(i=0; i<strlen(info); i++) {
		if(j == 3) {
			while(info[i] != '"') {
				str[k] = info[i];
				i++;
				k++;
			}
			str[k] = '\0';
		}
		if(info[i] == '"')
			j++;
	}
}

/* Sformatta il file Wurfl.xml estrapolando nome_device,
altezza_schermo, larghezza_schermo */
int main() {

	/* Apri il file 'Wurfl.xml' */
	FILE *f = fopen("Wurfl.xml", "r");

	char c[] = "  </device";
	char d[] = "      <capability name=\"model_name\" value=";
	char w[] = "      <capability name=\"resolution_width\" value=";
	char h[] = "      <capability name=\"resolution_height\" value=";
	char d1[43];
	char d2[50];
	char info[100];
	char device[50];
	char larghezza[10];
	char altezza[10];
	int k, j = 0, end, wi, he, width = 0, height = 0, fine = 0, count = 0;

	for(j=0; j<10; j++) {
		larghezza[j] = ' ';
		altezza[j] = ' ';
	}

	while(!feof(f)) {
		j = 0;
		width = 0;
		height = 0;
		fine = 0;
		fgets(d1, 43, f);  //cerca stringa "model_name"
		k = strcmp(d1, d);
		if(k == 0) {
			fgets(d2, 50, f);  //cattura stringa "model_name"
			modifica_stringa(d2, device);

			while(fine == 0) {
				fgets(info, 100, f);   //cattura stringhe
				wi = strncmp(info, w, 40);  //cerca larghezza
				he = strncmp(info, h, 40);  //cerca altezza
				end = strncmp(info, c, 10);  //fine info

				if(end == 0) {
					if(width == 0) 
						larghezza[0] = '\0';
					if(height == 0)
						altezza[0] = '\0';

					fine = 1;
				}

				if(wi == 0) {  //se è stata trovata la larghezza
					cattura_info(info, larghezza);
					width++;
				}

				if(he == 0) {  //se è stata trovata l'altezza
					cattura_info(info, altezza);
					height++;
				}
			}
			/* Se di tale dispositivo sono presenti i 3 valori a noi
			utili, allora stampali su file */
			if(larghezza[0] != '\0' || altezza[0] != '\0') {
				printf("%s (%sx%s)\n-------------\n", device, larghezza, altezza);
				crea_txt(device, larghezza, altezza);
				count++;
				usleep(1000);
			}	
		} 
	}

	printf("STAMPA ULTIMATA !!\nSTAMPATI %d DISPOSITIVI\n", count);

	return(EXIT_SUCCESS);
}