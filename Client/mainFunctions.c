#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "globalVar.h"

size_t lenght_code = 10;
size_t len_max = 4;    //lunghezza massima rapressentabile con 4 cifre è 9999
const char *padding = "########";

//Gestione funzioni comunicazione
void write_on_sock(int sock, void *buffer, size_t n){

	size_t res;
	ssize_t ret;
	const char *buff;

	buff = buffer;
	res = n;

	while(res>0){
		if((ret = write(sock, buff, res)) <= 0){
			if(errno == EINTR){
				ret = 0;
			}else{
				fprintf(stderr, "Errore nella write in write_on_sock.\n"
                                "Probabile disconessione da parte del server.\n");
				exit(EXIT_FAILURE);
			}
		}
		res -= ret;
		buff += ret;
	}
}

void read_from_sock(int sock, void *buffer, size_t n) {
	size_t res;
	ssize_t ret;
	char *buff;

	buff = buffer;
	res = n;

	while(res>0){
		if((ret = read(sock, buff, res)) <= 0){
			if(errno == EINTR){
				ret = 0;
			}else{
				fprintf(stderr, "Errore nella read in read_from_sock.\n"
                                "Probabile disconessione da parte del server.\n");
				exit(EXIT_FAILURE);
			}
		}
		res -= ret;
		buff += ret;
	}

}

//Funzione per generare la stringa contente la lunghezza della stringa da inviare al client
char* padstring(char* input){

    char *len_str;

    int len = strlen(input);
    char *len_pad = (char *)calloc(len_max+1, sizeof(char));

    len_str = (char *)calloc(len+1, sizeof(char));
    sprintf(len_str,"%d", len);

    int padLen = len_max - strlen(len_str);
    if(padLen < 0){
        free(len_str);
        return NULL; //Evita che la stringa sia più lunga di len_max
    }
    sprintf(len_pad,"%s%*.*s", len_str, padLen, padLen, padding);

    free(len_str);
    return len_pad;

}

//Funzione per leggere il testo e l'oggetto da inviare al client
char *getText(int maxlen){
    char *buffer, *string, *input;
    int ret, len, res;

    restart:
    len = 0;
    string = (char *)calloc(maxlen+1, sizeof(char));
    string[0] = '\0';
    buffer = (char *)calloc(MAXBUFF, sizeof(char));
    redo:
    buffer = fgets(buffer, MAXBUFF, stdin);
    if(errno==EINTR) goto redo;
    if(buffer!=NULL && strcmp(buffer,"$END$\n")!=0){
        len = len + strlen(buffer);
        if (len > maxlen){
            res = maxlen - (len-strlen(buffer));
            strncat(string, buffer, res);

            if(string[maxlen-1]=='\n') string[maxlen-1]='\0';
            printf("Hai raggiunto il numero limite di caratteri.\nIl testo acquisito è:\n%s\n",string);
            printf("Digitare YES se si vuole proseguire lo stesso, NO se si vuole ripetere l'inserimento del testo.\n");

            read_again:
            ret = scanf("%ms",&input);
            fflush(stdin);
            if(ret == EOF || errno == EINTR){
                free(input);
                goto read_again;
            }

            if(strcmp(input, "YES") == 0){
                free(buffer);
                free(input);
                return string;
            }else if(strcmp(input,"NO")==0){
                printf("Ridigitare il testo.\n");
                free(input);
                free(buffer);
                free(string);
                goto restart;
            }else{
                printf("Comando non riconosciuto.\n");
                goto read_again;
            }

        }
        strcat(string, buffer);
        goto redo;
    }else if(buffer!=NULL && strcmp(buffer,"$END$\n")==0){
        free(buffer);
        if(string[len-1]=='\n') string[len-1]='\0';
        return string;
    }else{
        printf("Errore nella fgets.\n");
        exit(EXIT_FAILURE);
    }
}

/*---------------------------------------------------------------------------*/

//Funzione che permette l'invio di un messaggio
void invia_messaggio(int conn_s) {

    char *input;
    char *padlen;
    char CODE_buffer[lenght_code + 1];
    int res;

    printf("Per inviare un messaggio è necessario inserire un destinatario appartente al sistema, un oggetto di massimo %d caratteri e un testo di massimo %d caratteri.\n",MAX_OBJECT_LENGHT, MAX_TEXT_LENGHT);
    printf("Per terminare l'inserimento del testo o dell'oggetto digitare in una nuova linea \"$END$\".\n");
    write_on_sock(conn_s, CODE_SNDUSRDST, lenght_code);

    redo_read_userdest:
    printf("Inserire destinatario:  \n");
    res = scanf("%ms", &input);
    fflush(stdin);
    if (res == EOF || errno == EINTR){
        free(input);
        goto redo_read_userdest;
    }
    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_userdest;
    }
    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer, CODE_OKUSRDST) == 0){
        printf("Destinatario valido.\n");
    }else if(strcmp(CODE_buffer, CODE_ERRORUSRDST) == 0){
        printf("Destinatario non valido.\n");
        goto redo_read_userdest;
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }

    write_on_sock(conn_s, CODE_SNDOBJ, lenght_code);

redo_read_object:
    printf("Inserire oggetto: \n");
    input = getText(MAX_OBJECT_LENGHT);
    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_object;
    }

    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer, CODE_OKOBJ) == 0){
        printf("Oggetto valido.\n");
    /*}else if(strcmp(CODE_buffer, CODE_ERROROBJ) == 0){
        printf("Username non valido.\n");
        goto redo_read_object;
    */}else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }

    write_on_sock(conn_s, CODE_SNDTXT, lenght_code);

redo_read_text:
    printf("Inserire testo:  \n");
    input = getText(MAX_TEXT_LENGHT);
    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_text;
    }

    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer, CODE_OKTXT) == 0){
        printf("Testo valido.\n");
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if (strcmp(CODE_buffer, CODE_OKSNDMSG) == 0) {
        printf("Messaggio inviato con successo.\n");
    }else if(strcmp(CODE_buffer, CODE_ERRSNDMSG) == 0){
        printf("Errore nell'invio del messaggio.\n"
               "Il destinatario non può ricevere nuovi messaggi\n");
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }
}

//Funzione che permette di eliminare un messaggio
void elimina_messaggio(int conn_s) {

    char *input;
    char *padlen;
    char CODICE_buffer[lenght_code+1];
    int res;

redo_ID:
    printf("Inserire ID del messaggio da eliminare: ");
    res = scanf("%ms", &input);
    fflush(stdin);
    if(res==EOF || errno==EINTR){
        free(input);
        goto redo_ID;
    }

    if(atoi(input)==0){ //i messaggi iniziano da ID 1
        printf("Numero messaggio non valido.\n");
        free(input);
        goto redo_ID;
    }

    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_ID;
    }
    write_on_sock(conn_s, CODE_IDMSG, lenght_code);
    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

    read_from_sock(conn_s, CODICE_buffer, lenght_code);
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_OKDEL)==0) {
        printf("Il messaggio è stato eliminato con successo.\n");
    }else if(strcmp(CODICE_buffer,CODE_ERRORDEL)==0) {
        printf("Il messaggio non esiste.\n");
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }
}

//Funzione che mostra i messaggi
void visualizza_messaggi(int sock) {

    int len;
    char *buffer;

    char CODICE_buffer[lenght_code+1];
    char LEN_buffer[len_max+1];

    read_from_sock(sock, CODICE_buffer, lenght_code);
    CODICE_buffer[lenght_code] = '\0';

    while(strcmp(CODICE_buffer,CODE_CONVIS)==0){
        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_IDMSG)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }
        read_from_sock(sock, LEN_buffer, len_max);
        LEN_buffer[len_max] = '\0';
        len = atoi(LEN_buffer);

        buffer = (char *) calloc(len+1, sizeof(char));
        read_from_sock(sock, buffer, len);
        buffer[len] = '\0';
        printf("---------------------------------------------------------------------------\n");
        printf("ID Messaggio - %s\n", buffer);
        free(buffer);

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_SNDUSRMTT)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }
        read_from_sock(sock, LEN_buffer, len_max);
        LEN_buffer[len_max] = '\0';
        len = atoi(LEN_buffer);

        buffer = (char *) calloc(len+1, sizeof(char));
        read_from_sock(sock, buffer, len);
        buffer[len] = '\0';
        printf("MITTENTE\n%s\n", buffer);
        free(buffer);

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_SNDOBJ)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }
        read_from_sock(sock, LEN_buffer, len_max);
        LEN_buffer[len_max] = '\0';
        len = atoi(LEN_buffer);

        buffer = (char *) calloc(len+1, sizeof(char));
        read_from_sock(sock, buffer, len);
        buffer[len] = '\0';
        printf("OGGETTO\n%s\n", buffer);
        free(buffer);

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_SNDTXT)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit(EXIT_FAILURE);
        }
        read_from_sock(sock, LEN_buffer, len_max);
        LEN_buffer[len_max] = '\0';
        len = atoi(LEN_buffer);

        buffer = (char *) calloc(len+1, sizeof(char));
        read_from_sock(sock, buffer, len);
        buffer[len] = '\0';
        printf("TESTO\n%s\n", buffer);
        free(buffer);

        read_from_sock(sock, CODICE_buffer, lenght_code);
        CODICE_buffer[lenght_code] = '\0';
    }

    printf("---------------------------------------------------------------------------\n");

    if(strcmp(CODICE_buffer,CODE_ENDVIS)==0) {
        printf("FINE MESSAGGI\n");
        printf("---------------------------------------------------------------------------\n");
    }
}

/*---------------------------------------------------------------------------*/

//Funzione che permette il login
bool login(int conn_s){
   
    int res;
    char *padlen;
    char *input;
    char CODE_buffer[lenght_code+1];

    write_on_sock(conn_s, CODE_SNDUSRNM, lenght_code);

redo_read_username:
    printf("Inserire username:  \n");
    res = scanf("%ms", &input);
    fflush(stdin);
    if(res==EOF || errno==EINTR) goto redo_read_username;
    printf("Input: %s\n", input);

    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_username;
    }
    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer, CODE_OKUSRNM) == 0){
        printf("Username valido.\n");
    }else if(strcmp(CODE_buffer, CODE_ERRORUSRNM) == 0){
        printf("Username non valido.\n");
        return false;
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }

    write_on_sock(conn_s, CODE_SNDPASS, lenght_code);

redo_read_password: //da fare controllo numero di volte che ci provo
	printf("Inserire password: ");
    res = scanf("%ms", &input);
    fflush(stdin);
    if (res == EOF || errno == EINTR){
        free(input);
        goto redo_read_password;
    }

    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_password;
    }
    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer, CODE_OKPASS) == 0){
        printf("Password valida.\n");
        return true;
    }else if(strcmp(CODE_buffer, CODE_ERRORPASS) == 0){
        printf("Password non valida.\n");
        return false;
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }
}

//Funzione che permette la registrazione
void registrazione(int conn_s){

    int res;
    int len;
    char *input;
    char *padlen;
    char *password;
    char LEN_buffer[len_max+1];
	char CODE_buffer[lenght_code+1];

    printf("Per effetuare la registrazione è necessario inserire un username composto da caratteri alfanumerici minuscoli e di lunghezza compresa tra %d e %d caratteri inclusi.\n",
           MIN_USERNAME_LENGHT, MAX_USERNAME_LENGHT);
    printf("Inoltre è necessario inserire la lunghezza della password compresa tra %d e %d caratteri inclusi.\n",
           MIN_PASSWORD_LENGHT, MAX_PASSWORD_LENGHT);	

	write_on_sock(conn_s, CODE_SNDUSRNM, lenght_code);

redo_read_username:
    printf("Inserire username:  \n");
    res = scanf("%ms", &input);
    fflush(stdin);
    if (res == EOF || errno == EINTR){ 
		free(input);
		goto redo_read_username;
	}
    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_username;
    }

	if(strlen(input) < MIN_USERNAME_LENGHT || strlen(input) > MAX_USERNAME_LENGHT){
		printf("Lunghezza username non valida.\n");
		free(input);
		goto redo_read_username;
	}

    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
    free(input);
    free(padlen);

	read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer, CODE_OKUSRNM) == 0){
        printf("Username valido.\n");
	}else if(strcmp(CODE_buffer, CODE_OLDUSRNM) == 0){
		printf("Username già esistente.\n");
		goto redo_read_username;
    }else if(strcmp(CODE_buffer, CODE_TMPOLDUSRNM) == 0){
        printf("Username già in corso di registrazione.\n");
        goto redo_read_username;
    }else if(strcmp(CODE_buffer, CODE_ERRORUSRNM) == 0){
        printf("Username non valido.\n");
        goto redo_read_username;
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }

	write_on_sock(conn_s, CODE_SNDPASSLEN, lenght_code);
	
redo_read_password:
    printf("Lunghezza password: ");
    res = scanf("%ms", &input);
    fflush(stdin);
    if (res == EOF || errno == EINTR){ 
		free(input);
		goto redo_read_password;
	}

    padlen = padstring(input);
    if(padlen == NULL){
        printf("Lunghezza input superiore della grandezza massima dei dati gestibili.\n");
        free(input);
        free(padlen);
        goto redo_read_password;
    }

    if (atoi(input) < MIN_PASSWORD_LENGHT || atoi(input) > MAX_PASSWORD_LENGHT) {
        printf("Lunghezza non valida. Minima di %d e massima di %d.\n", MIN_PASSWORD_LENGHT, MAX_PASSWORD_LENGHT);
        goto redo_read_password;
    }else{
		printf("Lunghezza password valida.\n");
	}

    write_on_sock(conn_s, CODE_LENBUFF, lenght_code);
    write_on_sock(conn_s, padlen, len_max);
    write_on_sock(conn_s, input, strlen(input));
	free(input);
    free(padlen);

	read_from_sock(conn_s, CODE_buffer, lenght_code);
	CODE_buffer[lenght_code] = '\0';
	
	if(strcmp(CODE_buffer, CODE_SNDPASS) != 0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }
    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer,CODE_LENBUFF)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }
    read_from_sock(conn_s, LEN_buffer, len_max);
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    password = (char *) calloc(len+1, sizeof(char));
    read_from_sock(conn_s, password, len);
    password[len] = '\0';
    printf("Password generata: %s\n", password);
    free(password);

    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if (strcmp(CODE_buffer, CODE_OKPASS) != 0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }
    printf("Registrazione completata con successo.\n");
}

void autenticazione(int conn_s){

    char *buffer;
    int ret;

redo_access:
    printf("Scrivere SIGNUP per registrarsi o LOGIN per effettuare il login.\n");

    read_again:
    ret = scanf("%ms",&buffer);
    fflush(stdin);
    if(ret == EOF || errno == EINTR){
        free(buffer);
        goto read_again;
    }

    if(strcmp(buffer, "SIGNUP") == 0){
        write_on_sock(conn_s, CODE_SIGNUP, lenght_code);
        registrazione(conn_s);
        free(buffer);
    }else if(strcmp(buffer, "LOGIN") == 0){
        write_on_sock(conn_s, CODE_LOGIN,lenght_code);
        if(login(conn_s)==false){
            free(buffer);
            goto redo_access;
        }
        free(buffer);
    }else{
        printf("Comando non valido.\n");
        goto read_again;
    }
}
