#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "globalVar.h"
#include "mainFunctions.h"

pthread_mutex_t *userTable_mux;
pthread_mutex_t *authTable_mux;
pthread_mutex_t *tempUserNameTable_mux;
pthread_mutex_t *file_auth_mux;
pthread_mutex_t *clients_mux;
pthread_mutex_t *user_register_mux;

auth_entry *authTable[MAX_KEY];
user_entry *userTable[MAX_KEY];
char *tempUserNameTable[MAX_KEY];

#define DOMINIO AF_INET
#define PORT 8080
#define IPADDRESS INADDR_ANY
#define LISTENQ 5

int main(void) {
    int       list_s;               
    int       conn_s;               
    struct    sockaddr_in servaddr;  
    struct	  sockaddr_in their_addr;
	uint 	  sin_size;
	pthread_t tid;
	pthread_attr_t attr;

    sigset_t set;

    if ( (list_s = socket(DOMINIO, SOCK_STREAM, 0)) < 0 ) {
	fprintf(stderr, "Errore nella creazione della socket.\n");
	exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = DOMINIO;
    servaddr.sin_addr.s_addr = htonl(IPADDRESS);
    servaddr.sin_port        = htons(PORT);

    const int enable = 1;
    if(setsockopt(list_s, SOL_SOCKET, SO_REUSEADDR,&enable,sizeof(int)) < 0){
        fprintf(stderr, "Errore nella setsockopt.\n");
        exit(EXIT_FAILURE);
    }


    if(bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
		fprintf(stderr, "Errore durante la bind.\n");
		exit(EXIT_FAILURE);
    }

    if(listen(list_s, LISTENQ) < 0 ) {
		fprintf(stderr, "server: errore durante la listen.\n");
		exit(EXIT_FAILURE);
    }

	//settaggio mutexes
    authTable_mux = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    userTable_mux = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    tempUserNameTable_mux = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    file_auth_mux = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    clients_mux = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    user_register_mux = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    if(authTable_mux == NULL || userTable_mux == NULL || file_auth_mux == NULL || clients_mux == NULL || user_register_mux == NULL || tempUserNameTable_mux == NULL){
        fprintf(stderr, "Errore nella calloc dei mutexes.\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_mutex_init(authTable_mux, NULL) || pthread_mutex_init(userTable_mux, NULL) || pthread_mutex_init(file_auth_mux, NULL) || pthread_mutex_init(clients_mux, NULL) || pthread_mutex_init(user_register_mux, NULL) || pthread_mutex_init(tempUserNameTable_mux, NULL)){
        fprintf(stderr, "Errore nella inizializzazione dei mutexes..\n");
        exit(EXIT_FAILURE);
    }
	//Settaggio Detached del thread
	if(pthread_attr_init(&attr)){
		fprintf(stderr, "Errore nella pthread_attr_init.\n");
		exit(EXIT_FAILURE);
	}
	if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)){ //evito stato zombie dopo pthread exit
		fprintf(stderr, "Errore nella pthread_attr_setdetachstate.\n");
		exit(EXIT_FAILURE);
	}

    // Ignora tutti i segnali tranne SIGKILL (non gestibile dal codice), SIGILL, SIGSEGV
    //Posso così garantire che il server non venga terminato da un segnale a meno che non si invia SIGKILL o viene esguito qualcosa di illegale
    if(sigfillset(&set)){
        fprintf(stderr, "Errore nella sigfillset.\n");
        exit(EXIT_FAILURE);
    }
    if(sigdelset(&set, SIGILL) || sigdelset(&set, SIGSEGV)){
        fprintf(stderr, "Errore nella sigdelset.\n");
        exit(EXIT_FAILURE);
    }
    if(sigprocmask(SIG_BLOCK, &set, NULL)){
        fprintf(stderr, "Errore nella sigprocmask.\n");
        exit(EXIT_FAILURE);
    }

    //Carico le tabelle hash
    loadFromFile(authFile, authTable, userTable);

    while(1){
    redo_client_conn:
		sin_size = sizeof(struct sockaddr_in);
		if ( (conn_s = accept(list_s, (struct sockaddr *)&their_addr, &sin_size) ) < 0 ) {
		    fprintf(stderr, "Errore nella accept.\n");
            exit(EXIT_FAILURE);
		}

        if(pthread_mutex_lock(clients_mux)){
            fprintf(stderr, "Errore nella pthread_mutex_lock del server.\n");
            exit(EXIT_FAILURE);
        }
        if((online_clients+1) > MAX_USERS_ONLINE){
            fprintf(stderr, "Troppi client connessi.\n");
            if(!write_on_sock(conn_s, CODE_ABNCLOSE, lenght_code)){
                close(conn_s);
                if(pthread_mutex_unlock(clients_mux)){
                    fprintf(stderr, "Errore nella pthread_mutex_unlock del server.\n");
                    exit(EXIT_FAILURE);
                }
                goto redo_client_conn;
            }
            close(conn_s);
            if(pthread_mutex_unlock(clients_mux)){
                fprintf(stderr, "Errore nella pthread_mutex_unlock del server.\n");
                exit(EXIT_FAILURE);
            }
            goto redo_client_conn;
        }
        online_clients++;
        if(pthread_mutex_unlock(clients_mux)){
            fprintf(stderr, "Errore nella pthread_mutex_unlock del server.\n");
            exit(EXIT_FAILURE);
        }

        if(!write_on_sock(conn_s, CODE_OKCONN, lenght_code)){
            exit_client(conn_s);
            goto redo_client_conn;
        }

		if(pthread_create(&tid, &attr, client_thread, (void *)((long)conn_s))){
			fprintf(stderr, "Errore nella creazione del client_thread.\n");
			exit(EXIT_FAILURE);
		}
	}
}


