#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "globalVar.h"

#define DOMINIO AF_INET
#define PORT 8080
#define IPADDRESS "127.0.0.1"

int conn_s;

void sig_handler(int a, siginfo_t *b, void *c){
    printf("\nArresto volontario del client.\n");
    close(conn_s);
    exit(EXIT_SUCCESS);
}

void sigpipe_handler(int a, siginfo_t *b, void *c){
    printf("\nIl server è stato chiuso.\n");
    close(conn_s);
    exit(EXIT_SUCCESS);
}

int main(){
    struct    sockaddr_in clientaddr;
    struct	  hostent *he;
    struct sigaction act;
    sigset_t set;


    he=NULL;


    if ( (conn_s = socket(DOMINIO, SOCK_STREAM, 0)) < 0 ) {
      fprintf(stderr, "Errore durante la creazione della socket.\n");
      exit(EXIT_FAILURE);
    }

    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family      = DOMINIO;
    clientaddr.sin_port        = htons(PORT);

    if ( inet_pton(DOMINIO, IPADDRESS, &clientaddr.sin_addr) <= 0 ) {
      printf("Indirizzo IP non valido.\nRisoluzione nome...");
      
      if ((he=gethostbyname(IPADDRESS)) == NULL) {
        printf("fallita.\n");
        exit(EXIT_FAILURE);
      }
      printf("riuscita.\n");
      clientaddr.sin_addr = *((struct in_addr *)he->h_addr);
    }


    if ( connect(conn_s, (struct sockaddr *) &clientaddr, sizeof(clientaddr) ) < 0 ) {
      printf("Errore durante la connect.\n"
             "Il server è probabilmente spento.\n"
             "Riprova più tardi.\n");
      exit(EXIT_FAILURE);
    }

    //setto i segnali attarverso il quale il client può terminare il processo durante l'esecuzione senza usare l'opzione LOGOUT
    if(sigfillset(&set)){
        fprintf(stderr, "Errore nella sigfillset.\n");
        exit(EXIT_FAILURE);
    }
    act.sa_sigaction = sig_handler;
    act.sa_mask = set;
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) || sigaction(SIGTERM, &act, NULL) || sigaction(SIGQUIT, &act, NULL)) {
        fprintf(stderr, "Errore nella sigaction.\n");
        exit(EXIT_FAILURE);
    }
    act.sa_sigaction = sigpipe_handler;
    act.sa_mask = set;
    act.sa_flags = 0;
    if(sigaction(SIGPIPE, &act, NULL)) {
        fprintf(stderr, "Errore nella sigaction.\n");
        exit(EXIT_FAILURE);
    }

    char CODE_buffer[lenght_code];
    read_from_sock(conn_s, CODE_buffer, lenght_code);
    CODE_buffer[lenght_code] = '\0';
    if(strcmp(CODE_buffer,CODE_OKCONN)!=0 && strcmp(CODE_buffer,CODE_ABNCLOSE)!=0){
        printf("Errore nella lettura del codice.\n");
        exit(EXIT_FAILURE);
    }else if(strcmp(CODE_buffer,CODE_ABNCLOSE)==0){
        printf("Il server ha raggiunto il limite di utenti connessi contemporaneamente.\n"
               "Riprova più tardi.\n");
        exit(EXIT_FAILURE);
    }

    printf("Benvenuti nel sistema di messaggistica.\n");
    printf("(Per uscire dal sistema premere Ctrl+C o Ctrl+\\)\n");
    autenticazione(conn_s);
    
    while(1){
		char *input;
		int option, res;
    read_again:
        printf("Digitare una delle seguenti opzioni:\n");
        printf("1 - Visualizza messaggi\n");
        printf("2 - Invia messaggio\n");
        printf("3 - Elimina messaggio\n");
        printf("4 - Logout\n");
        printf("Inserire il numero corrispondente all'opzione desiderata: ");
        res = scanf("%ms",&input);
        fflush(stdin);
		if (res==EOF || errno==EINTR){
			free(input);
			goto read_again;
		}
		
		option = atoi(input);
		free(input);

        switch(option){
            case 1:
                write_on_sock(conn_s, CODE_SHOWMSG, lenght_code);
                visualizza_messaggi(conn_s);
                break;
            case 2:
                write_on_sock(conn_s, CODE_SNDMSG, lenght_code);
                invia_messaggio(conn_s);
                break;
            case 3:
                write_on_sock(conn_s, CODE_RMVMSG, lenght_code);
                elimina_messaggio(conn_s);
                break;
            case 4:
                write_on_sock(conn_s, CODE_LOGOUT, lenght_code);
                close(conn_s);
                exit(EXIT_SUCCESS);
            default:
                printf("Opzione non valida.\n");
                goto read_again;
        }
    }
}