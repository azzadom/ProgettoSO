#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <crypt.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "globalVar.h"
#include "mainFunctions.h"
#include "messageFunctions.h"
#include "authFunctions.h"

const size_t lenght_code = 10; //lunghezza codice di comunicazione

const char *dir_messages = "FileMessaggi"; //directory dove vengono salvati i messaggi

const int max_key = MAX_KEY;

const size_t len_max = 4; //unghezza massima rapressentabile con 4 cifre è 9999
const char *padding = "########"; //carattere di padding

const char *authFile = "credenziali.txt"; //file dove vengono salvate le credenziali degli utenti

int online_clients = 0; //Numero di client connessi
int utenti_registrati = 0; //Numero di utenti registrati

//set di caratteri per generare la password e il sale
const char* pass_set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+=[]{}|;':,./<>?";
const char* salt_set = "abcdefghijklmnopqrstuvwxyz0123456789./ABCDEFGHIJKLMNOPQRSTUVWXYZ"; //da man page di crypt

//Funzione che permette la disconessione del client
void exit_client(int sock){
    if(pthread_mutex_lock(clients_mux)){
        printf("Errore nella pthread_mutex_lock in exit_client.\n");
        exit(EXIT_FAILURE);
    }
    online_clients--;
    if(pthread_mutex_unlock(clients_mux)){
        printf("Errore nella pthread_mutex_unlock in exit_client.\n");
        exit(EXIT_FAILURE);
    }
    close(sock);
    printf("Disconessione dal socket %d.\n", sock);
    pthread_exit(NULL);
}

/*---------------------------------------------------------------------------*/

//Gestione funzioni comunicazione
bool write_on_sock(int sock,const void *buffer, size_t n){

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
                fprintf(stderr, "Probabile disconessione da parte del client sul socket %d.\n", sock);
                return false;
            }
		}
		res -= ret;
		buff += ret;
	}
	return true;
}

bool read_from_sock(int sock,void *buffer, size_t n) {
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
                fprintf(stderr, "Probabile disconessione da parte del client sul socket %d.\n", sock);
                return false;
            }
		}
		res -= ret;
		buff += ret;
	}

    return true;
}

//Funzione per generare la stringa contente la lunghezza della stringa da inviare al client
char* padstring(char* input){

    char *len_str;

    int len = strlen(input);
    char *len_pad = (char *)calloc(len_max+1, sizeof(char));
    if(len_pad == NULL){
        printf("Errore allocazione memoria in padstring.\n");
        exit(EXIT_FAILURE);
    }

    len_str = (char *)calloc(len+1, sizeof(char));
    if(len_str == NULL){
        printf("Errore allocazione memoria in padstring.\n");
        exit(EXIT_FAILURE);
    }
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

//Funzione hash per la tabella hash che ha come key l'username
int hash(char* key, int ind, int num_keys){
    int k = 0;
    int value = 0;
    int len = strlen(key);

    for (int i = 0; i < len; i++) {
        k += key[i];
    }

    value = ((k % num_keys) + ind*((k % (num_keys-2)) + 1)) % num_keys;

    return value;
}

/*---------------------------------------------------------------------------*/

//Funzione che mostra i messaggi
void mostra_messaggi(int sock, char *user, user_entry **hashTable){

    int fd, fd_index;
    int len;
    index_t *ind;
    message *msg;
    user_entry *utente;

    int num_msg;

    char *padlen;
    char *ID;

    char LEN_buffer[len_max+1];
    char CODICE_buffer[lenght_code+1];
    char PATH_buffer[MAX_PATHNAME_LEN+1];
    char PATH_IND_buffer[MAX_PATHNAME_LEN_IND+1];

    sprintf(PATH_buffer, "%s/%s", dir_messages, user);
    sprintf(PATH_IND_buffer, "%s/%s_ind",dir_messages, user);

    if(pthread_mutex_lock(userTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    utente = verificaEntry_user(user, hashTable); //Serve per prendere il mutex da bloccare
    if(pthread_mutex_unlock(userTable_mux)){
        printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_lock(utente->mutex)){
        printf("Errore nella pthread_mutex_lock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    fd = open(PATH_buffer, O_RDWR);
    fd_index = open(PATH_IND_buffer, O_RDWR);
    if(fd == -1 || fd_index == -1){
        printf("Errore apertura file utente %s o file index %s in invia_messaggio.\n", PATH_buffer, PATH_IND_buffer);
        exit(EXIT_FAILURE);
    }
    ind = carica_index(fd_index);
    num_msg = ind->num_messaggi;
    for(int i = num_msg; i>0; i--){
        if(!write_on_sock(sock, CODE_CONVIS, lenght_code)){
            exit_client(sock);
        }

        msg = visualizza_messaggio(fd, ind, i);

        ID = (char *) calloc(len_max+1, sizeof(char));
        if(ID == NULL){
            printf("Errore allocazione memoria in invia_messaggio.\n");
            exit(EXIT_FAILURE);
        }
        sprintf(ID, "%d", i);
        padlen = padstring(ID);
        if(!write_on_sock(sock, CODE_IDMSG, lenght_code) || !write_on_sock(sock, CODE_LENBUFF, lenght_code) || !write_on_sock(sock, padlen, len_max) || !write_on_sock(sock, ID, strlen(ID))){
            free(msg);
            free(ID);
            free(padlen);
            free(ind);
            exit_client(sock);
        }
        free(padlen);
        free(ID);

        padlen = padstring(msg->mittente);
        if(!write_on_sock(sock, CODE_SNDUSRMTT, lenght_code) || !write_on_sock(sock, CODE_LENBUFF, lenght_code) || !write_on_sock(sock, padlen, len_max) || !write_on_sock(sock, msg->mittente, strlen(msg->mittente))){
            free(padlen);
            free(msg);
            free(ind);
            exit_client(sock);
        }
        free(padlen);

        padlen = padstring(msg->oggetto);
        if(!write_on_sock(sock, CODE_SNDOBJ, lenght_code) || !write_on_sock(sock, CODE_LENBUFF, lenght_code) || !write_on_sock(sock, padlen, len_max) || !write_on_sock(sock, msg->oggetto, strlen(msg->oggetto))){
            free(padlen);
            free(msg);
            free(ind);
            exit_client(sock);
        }
        free(padlen);

        padlen = padstring(msg->testo);
        if(!write_on_sock(sock, CODE_SNDTXT, lenght_code) || !write_on_sock(sock, CODE_LENBUFF, lenght_code) || !write_on_sock(sock, padlen, len_max) || !write_on_sock(sock, msg->testo, strlen(msg->testo))){
            free(padlen);
            free(msg);
            free(ind);
            exit_client(sock);
        }
        free(padlen);

        free(msg);
    }
    close(fd);
    close(fd_index);
    if(pthread_mutex_unlock(utente->mutex)){
        printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }

    if(!write_on_sock(sock, CODE_ENDVIS, lenght_code)){
        free(ind);
        exit_client(sock);
    }
    free(ind);
}

//Funzione che permette l'invio di un messaggio
void invia_messaggio(char *usermtt, int sock, user_entry **hashTable){

    int fd, fd_index;
    int len;
    index_t *ind;
    message msg;
    user_entry *utente;
    bool result;

    char *userdst;
    char *object;
    char *text;
    char *padlen;

    char LEN_buffer[len_max+1];
    char CODICE_buffer[lenght_code+1];
    char PATH_buffer[MAX_PATHNAME_LEN+1];
    char PATH_IND_buffer[MAX_PATHNAME_LEN_IND+1];

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDUSRDST)!=0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }
redo_read_username:
    if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    userdst = (char *) calloc(len+1, sizeof(char));
    if(userdst == NULL){
        printf("Errore allocazione memoria in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, userdst, len)){
        free(userdst);
        exit_client(sock);
    }
    userdst[len] = '\0';

    if(pthread_mutex_lock(userTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    utente = verificaEntry_user(userdst, hashTable);
    if(pthread_mutex_unlock(userTable_mux)){
        printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    if (utente == NULL) {
        if(!write_on_sock(sock, CODE_ERRORUSRDST, lenght_code)) {
            free(userdst);
            exit_client(sock);
        }
        free(userdst);
        goto redo_read_username;
    }

    if(!write_on_sock(sock, CODE_OKUSRDST, lenght_code)){
        free(userdst);
        exit_client(sock);
    }

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        free(userdst);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDOBJ)!=0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(userdst);
        exit_client(sock);
    }

    if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
        free(userdst);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(userdst);
        exit_client(sock);
    }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        free(userdst);
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    if(len>MAX_OBJECT_LENGHT){
        free(userdst);
        exit_client(sock);
    }
    object = (char *) calloc(len+1, sizeof(char));
    if(object == NULL){
        printf("Errore allocazione memoria in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, object, len)){
        free(userdst);
        free(object);
        exit_client(sock);
    }
    object[len] = '\0';

    if(!write_on_sock(sock, CODE_OKOBJ, lenght_code)){
        free(userdst);
        free(object);
        exit_client(sock);
    }

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        free(userdst);
        free(object);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDTXT)!=0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(userdst);
        free(object);
        exit_client(sock);
    }

    if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
        free(userdst);
        free(object);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(userdst);
        free(object);
        exit_client(sock);
    }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        free(userdst);
        free(object);
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    if(len>MAX_TEXT_LENGHT){
        free(userdst);
        free(object);
        exit_client(sock);
    }
    text = (char *) calloc(len+1, sizeof(char));
    if(text == NULL){
        printf("Errore allocazione memoria in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, text, len)){
        free(userdst);
        free(object);
        free(text);
        exit_client(sock);
    }
    text[len] = '\0';

    if(!write_on_sock(sock, CODE_OKTXT, lenght_code)){
        free(userdst);
        free(object);
        free(text);
        exit_client(sock);
    }

    strcpy(msg.mittente, usermtt);
    strcpy(msg.oggetto, object);
    strcpy(msg.testo, text);

    sprintf(PATH_buffer, "%s/%s", dir_messages, userdst);
    sprintf(PATH_IND_buffer, "%s/%s_ind", dir_messages, userdst);

    if(pthread_mutex_lock(utente->mutex)){
        printf("Errore nella pthread_mutex_lock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    fd = open(PATH_buffer, O_RDWR);
    fd_index = open(PATH_IND_buffer, O_RDWR);
    if(fd == -1){
        printf("Errore apertura file utente %s in invia_messaggio.\n", PATH_buffer);
        exit(EXIT_FAILURE);
    }
    if(fd_index == -1){
        printf("Errore apertura file utente index %s in invia_messaggio.\n", PATH_IND_buffer);
        exit(EXIT_FAILURE);
    }

    result = inserisci_messaggio(fd, fd_index, &msg);
    close(fd);
    close(fd_index);
    if(pthread_mutex_unlock(utente->mutex)){
        printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }

    if(result){
        if(!write_on_sock(sock, CODE_OKSNDMSG, lenght_code)){
            free(userdst);
            free(object);
            free(text);
            exit_client(sock);
        }
    }else{
        if(!write_on_sock(sock, CODE_ERRSNDMSG, lenght_code)){
            free(userdst);
            free(object);
            free(text);
            exit_client(sock);
        }
    }
    free(userdst);
    free(object);
    free(text);
}

//Funzione che permette di eliminare un messaggio
void rimuovi_messaggio(int sock, char *user, user_entry *utente){

    int len;
    int fd, fd_index;
    char CODICE_buffer[lenght_code+1];
    char LEN_buffer[len_max+1];
    char PATH_buffer[MAX_PATHNAME_LEN+1];
    char PATH_IND_buffer[MAX_PATHNAME_LEN_IND+1];

    char *ID_buffer;
    int ID;

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_IDMSG)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);

    ID_buffer = (char *) calloc(len+1, sizeof(char));
    if(ID_buffer == NULL){
        printf("Errore allocazione memoria in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, ID_buffer, len)){
        free(ID_buffer);
        exit_client(sock);
    }
    ID_buffer[len] = '\0';
    ID = atoi(ID_buffer);
    free(ID_buffer);

    sprintf(PATH_buffer, "%s/%s", dir_messages, user);
    sprintf(PATH_IND_buffer, "%s/%s_ind", dir_messages, user);

    if(pthread_mutex_lock(utente->mutex)){
        printf("Errore nella pthread_mutex_lock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    fd = open(PATH_buffer, O_RDWR);
    fd_index = open(PATH_IND_buffer, O_RDWR);
    if(fd == -1){
        printf("Errore apertura file utente %s in invia_messaggio.\n", PATH_buffer);
        exit(EXIT_FAILURE);
    }
    if(fd_index == -1){
        printf("Errore apertura file utente index %s in invia_messaggio.\n", PATH_IND_buffer);
        exit(EXIT_FAILURE);
    }

    if(elimina_messaggio(&fd, &fd_index, ID, PATH_buffer, PATH_IND_buffer)){
        if(!write_on_sock(sock, CODE_OKDEL, lenght_code)){
            close(fd);
            close(fd_index);
            if(pthread_mutex_unlock(utente->mutex)){
                printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
                exit(EXIT_FAILURE);
            }
            exit_client(sock);
        }
    }else{
        if(!write_on_sock(sock, CODE_ERRORDEL, lenght_code)){
            close(fd);
            close(fd_index);
            if(pthread_mutex_unlock(utente->mutex)){
                printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
                exit(EXIT_FAILURE);
            }
            exit_client(sock);
        }
    }
    close(fd);
    close(fd_index);
    if(pthread_mutex_unlock(utente->mutex)){
        printf("Errore nella pthread_mutex_unlock in invia_messaggio.\n");
        exit(EXIT_FAILURE);
    }
}

/*---------------------------------------------------------------------------*/

//Salva alla fine del file di autenticazione il nuovo utente registrato
void saveToFile(const char *filename, char *user, auth_entry **hashTable) {

    auth_entry *utente = verificaEntry_auth(user, hashTable);

    FILE *file = fopen(filename, "r+");

    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        fprintf(file, "%s %s %s\n", utente->username, utente->usersalt, utente->userpass);
        fflush(file);
        fclose(file);
    }else{
        printf("Errore apertura file credenziali in saveToFile.\n");
        exit(EXIT_FAILURE);
    }
}

//Carica i dati dal file di autenticazione e li inserisce nelle tabelle hash
void loadFromFile(const char *filename, auth_entry **hashTable_auth, user_entry **hashTable_user) {
    int res;

    FILE *file = fopen(filename, "r");

    if (file != NULL) {
        while(1){
            char *user = (char *) calloc(MAX_USERNAME_LENGHT+1, sizeof(char));
            char *salt = (char *) calloc(SALT_LEN+CODE_LEN+1, sizeof(char));
            char *pass = (char *) calloc(CODE_LEN+SALT_LEN+MAX_PASSWORD_LENGHT_STORED+2, sizeof(char)); //include anche il secondo $ dopo il salt
            if(user == NULL || salt == NULL || pass == NULL){
                printf("Errore allocazione memoria.\n");
                exit(EXIT_FAILURE);
            }

            res = fscanf(file, "%s %s %s", user, salt, pass);
            if(res != EOF) {
                utenti_registrati++;
                if(utenti_registrati>(max_key/2)){
                    printf("AVVISO: Fattore di carico per le strutture dati maggiore di 0.5.\n"
                           "Raddoppiare la variabile max_key e ricompilare il server.\n");
                }
                newEntry_auth(user, salt, pass, hashTable_auth);
                newEntry_user(user, hashTable_user);
            }else {
                free(user);
                free(salt);
                free(pass);
                break;
            }
        }
        fclose(file);
    }else{
        printf("Errore apertura file credenziali in loadFromFile.\n");
        exit(EXIT_FAILURE);
    }
}

//Funzione che permette il login
char *login(int sock, auth_entry **hashTable){

    int len;
    auth_entry *utente;
    char *username;
    char *password;
    char LEN_buffer[len_max+1];
    char CODICE_buffer[lenght_code+1];

    struct crypt_data data;
    data.initialized = 0;

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDUSRNM)!=0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }
    //redo_read_username:
    if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    username = (char *) calloc(len+1, sizeof(char));
    if(username == NULL){
        printf("Errore allocazione memoria in login.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, username, len)){
        free(username);
        exit_client(sock);
    }
    username[len] = '\0';

    if(pthread_mutex_lock(authTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    utente = verificaEntry_auth(username, hashTable);
    if(pthread_mutex_unlock(authTable_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    if (utente == NULL) {
        if(!write_on_sock(sock, CODE_ERRORUSRNM, lenght_code)){
            free(username);
            exit_client(sock);
        }
        free(username);
        return NULL;
    }

    if(!write_on_sock(sock, CODE_OKUSRNM, lenght_code)){
        free(username);
        exit_client(sock);
    }

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        free(username);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDPASS)!=0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(username);
        exit_client(sock);
    }
    //redo_read_password:
        if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
            free(username);
            exit_client(sock);
        }
        CODICE_buffer[lenght_code] = '\0';
        if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            free(username);
            exit_client(sock);
        }
        if(!read_from_sock(sock, LEN_buffer, len_max)){
            free(username);
            exit_client(sock);
        }
        LEN_buffer[len_max] = '\0';
        len = atoi(LEN_buffer);
        password = (char *) calloc(len+1, sizeof(char));
        if(password == NULL){
            printf("Errore allocazione memoria in login.\n");
            exit(EXIT_FAILURE);
        }
        if(!read_from_sock(sock, password, len)){
            free(username);
            free(password);
            exit_client(sock);
        }
        password[len] = '\0';

        if(strcmp(crypt_r(password, utente->usersalt,(struct crypt_data *)&data), utente->userpass) == 0) {
            if(!write_on_sock(sock, CODE_OKPASS, lenght_code)){
                free(username);
                free(password);
                exit_client(sock);
            }
        }else{
            if(!write_on_sock(sock, CODE_ERRORPASS, lenght_code)){
                free(username);
                free(password);
                exit_client(sock);
            }
            free(username);
            free(password);
            return NULL;
        }

        free(password);
        return username;
}

//Funzione che permette la registrazione
char *registrazione(int sock, const char *filename, auth_entry **hashTable_auth, user_entry **hashTable_user, char **hashTable_temp){

    char *padlen;
    auth_entry *utente;
    char *username;
    char *password_len;
    char *password_buff;
    char *salt_crypt;
    char *salt_buff;
    int len;

    char LEN_buffer[len_max+1];
    char CODICE_buffer[lenght_code+1];
    char PATH_buffer_main[MAX_PATHNAME_LEN+1];
    char PATH_buffer_ind[MAX_PATHNAME_LEN_IND+1];

    struct crypt_data data;
    data.initialized = 0;

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDUSRNM)!=0){
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }
redo_read_username:
    if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
            fprintf(stderr, "Errore nella lettura del codice.\n");
            exit_client(sock);
        }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    if(len<MIN_USERNAME_LENGHT || len>MAX_USERNAME_LENGHT){
        exit_client(sock);
    }
    username = (char *) calloc(len+1, sizeof(char));
    if(username == NULL){
        printf("Errore allocazione memoria in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, username, len)){
        free(username);
        exit_client(sock);
    }
    username[len] = '\0';

    if (!verifica_username(username, strlen(username))) {
        if(!write_on_sock(sock, CODE_ERRORUSRNM, lenght_code)){
            free(username);
            exit_client(sock);
        }
        free(username);
        goto redo_read_username;
    }

    if(pthread_mutex_lock(authTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    utente = verificaEntry_auth(username, hashTable_auth);
    if(pthread_mutex_unlock(authTable_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    if (utente != NULL) {
        if(!write_on_sock(sock, CODE_OLDUSRNM, lenght_code)){
            free(username);
            exit_client(sock);
        }
        free(username);
        goto redo_read_username;
    }

    if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    if(verificaTempUsername(username, hashTable_temp)){
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        if(!write_on_sock(sock, CODE_TMPOLDUSRNM, lenght_code)){
            free(username);
            exit_client(sock);
        }
        free(username);
        goto redo_read_username;
    }else{
        newTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
    }

    if(!write_on_sock(sock, CODE_OKUSRNM, lenght_code)){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        free(username);
        exit_client(sock);
    }

    if(!read_from_sock(sock, CODICE_buffer, lenght_code)){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        } 
        free(username);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_SNDPASSLEN)!=0){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(username);
        exit_client(sock);
    }

    if(!read_from_sock(sock,CODICE_buffer, lenght_code)){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        free(username);
        exit_client(sock);
    }
    CODICE_buffer[lenght_code] = '\0';
    if(strcmp(CODICE_buffer,CODE_LENBUFF)!=0) {
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "Errore nella lettura del codice.\n");
        free(username);
        exit_client(sock);
    }
    if(!read_from_sock(sock, LEN_buffer, len_max)){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        free(username);
        exit_client(sock);
    }
    LEN_buffer[len_max] = '\0';
    len = atoi(LEN_buffer);
    password_len = (char *) calloc(len+1, sizeof(char));
    if(password_len == NULL){
        printf("Errore allocazione memoria in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    if(!read_from_sock(sock, password_len, len)){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        free(username);
        free(password_len);
        exit_client(sock);
    }
    password_len[len] = '\0';
    len = atoi(password_len);
    if(len<MIN_PASSWORD_LENGHT || len>MAX_PASSWORD_LENGHT){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        free(password_len);
        free(username);
        exit_client(sock);
    }
    free(password_len);

    password_buff = (char *) calloc(len+1, sizeof(char));
    salt_buff = (char *) calloc(SALT_LEN+1, sizeof(char));
    if(password_buff == NULL || salt_buff == NULL){
        printf("Errore allocazione memoria in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    generatore_valido(password_buff, len, pass_set, verifica_pass);

    padlen = padstring(password_buff); //Non verifico NULL perchè la password avrà sempre lunghezza rappresentabile
    if(!write_on_sock(sock, CODE_SNDPASS, lenght_code) || !write_on_sock(sock, CODE_LENBUFF, lenght_code) || !write_on_sock(sock, padlen, len_max) || !write_on_sock(sock, password_buff, len)){
        if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
        }
        freeTempUsername(username, hashTable_temp);
        if(pthread_mutex_unlock(tempUserNameTable_mux)){
            printf("Errore nella pthread_mutex_unlock in registrazione.\n");
            exit(EXIT_FAILURE);
        }
        free(padlen);
        free(password_buff);
        free(salt_buff);
        free(username);
        exit_client(sock);
    }
    free(padlen);

    generatore_valido(salt_buff, SALT_LEN, salt_set, verifica_salt);

    salt_crypt = (char *) calloc(CODE_LEN + SALT_LEN + 1, sizeof(char));
    if(salt_crypt == NULL){
        printf("Errore allocazione memoria in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    sprintf(salt_crypt, "%s%s", CODE_CRYPT, salt_buff);
    free(salt_buff);

    password_buff = strdup(crypt_r(password_buff, salt_crypt, (struct crypt_data  *)&data));

    sprintf(PATH_buffer_main, "%s/%s", dir_messages, username);
    if(creat(PATH_buffer_main, S_IRUSR | S_IWUSR) == -1){
        printf("Errore creazione file utente %s in registrazione.\n", PATH_buffer_main);
        exit(EXIT_FAILURE);
    }
    sprintf(PATH_buffer_ind, "%s/%s_ind", dir_messages, username);
    if(creat(PATH_buffer_ind, S_IRUSR | S_IWUSR) == -1){
        printf("Errore creazione file utente index %s PATH_buffer_ind in registrazione.\n", PATH_buffer_ind);
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_lock(authTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    newEntry_auth(username, salt_crypt, password_buff, hashTable_auth);
    if(pthread_mutex_lock(file_auth_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    saveToFile(filename, username, hashTable_auth);
    if(pthread_mutex_unlock(authTable_mux) || pthread_mutex_unlock(file_auth_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_lock(userTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    newEntry_user(username, hashTable_user);
    if(pthread_mutex_unlock(userTable_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    if(pthread_mutex_lock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    freeTempUsername(username, hashTable_temp);
    if(pthread_mutex_unlock(tempUserNameTable_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    if(!write_on_sock(sock, CODE_OKPASS, lenght_code)){
        exit_client(sock);
    }

    if(pthread_mutex_lock(user_register_mux)){
        printf("Errore nella pthread_mutex_lock in registrazione.\n");
        exit(EXIT_FAILURE);
    }
    utenti_registrati++;
    if(utenti_registrati>(max_key/2)) {
        printf("AVVISO: Fattore di carico per le strutture dati maggiore di 0.5.\n"
               "Raddoppiare la variabile max_key e ricompilare il server.\n");
    }
    if(pthread_mutex_unlock(user_register_mux)){
        printf("Errore nella pthread_mutex_unlock in registrazione.\n");
        exit(EXIT_FAILURE);
    }

    return username;
}

char *autenticazione(int sock){

    char *username;
    char CODICE_buffer[lenght_code+1];

redo_read_code:
    read(sock, CODICE_buffer, lenght_code);
    CODICE_buffer[lenght_code] = '\0';

    if(strcmp(CODICE_buffer,CODE_LOGIN)==0){
        username = login(sock,authTable);
        if(username == NULL) goto redo_read_code;
        return username;
    }else if(strcmp(CODICE_buffer,CODE_SIGNUP)==0){
        username = registrazione(sock,authFile, authTable, userTable, tempUserNameTable);
        return username;
    }else{
        fprintf(stderr, "Errore nella lettura del codice.\n");
        exit_client(sock);
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/

//Gestione funzione thread
void *client_thread(void *param) {

    char *username;
    int client_socket = (long)param;

    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)) < 0) {
        fprintf(stderr, "Errore nella setsockopt.\n");
        exit(EXIT_FAILURE);
    }

    username = autenticazione(client_socket);

    while(1){
        user_entry *utente;
        char CODE_buff[lenght_code + 1];
        if(!read_from_sock(client_socket, CODE_buff, lenght_code)){
            exit_client(client_socket);
        }
        CODE_buff[lenght_code] = '\0';

        if (strcmp(CODE_buff, CODE_LOGOUT) == 0) {
            exit_client(client_socket);
        } else if (strcmp(CODE_buff, CODE_SNDMSG) == 0) {
            invia_messaggio(username, client_socket, userTable);
        } else if (strcmp(CODE_buff, CODE_RMVMSG) == 0) {
            utente = verificaEntry_user(username, userTable);
            rimuovi_messaggio(client_socket, username, utente);
        } else if (strcmp(CODE_buff, CODE_SHOWMSG) == 0) {
            mostra_messaggi(client_socket, username, userTable);
        } else {
            fprintf(stderr, "Errore nella lettura del codice in client_thread.\n");
            exit_client(client_socket);
        }
    }
}
