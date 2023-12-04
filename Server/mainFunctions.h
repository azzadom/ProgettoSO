#ifndef SERVER_HEADER_H
#define SERVER_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define TIMEOUT 300 //secondi

#define MAX_USERS_ONLINE 10

#define MAX_PATHNAME_LEN (13+MAX_USERNAME_LENGHT) //13 = "FileMessaggi/"
#define MAX_PATHNAME_LEN_IND (13+MAX_USERNAME_LENGHT+4) //4 = "_ind"

#define CODE_ABNCLOSE "3A2L6dlj59"
#define CODE_OKCONN "kQLwn2Idy4"

#define CODE_SIGNUP "HAmhuRIi8z"
#define CODE_LOGIN "BohHjC35xh"
#define CODE_LOGOUT "3Z2X2l5VbN"
#define CODE_SNDMSG "l6Cgjpd8kJ"
#define CODE_RMVMSG "h8dkFrDJwQ"
#define CODE_SHOWMSG "jDPzNW5OwT"

#define CODE_LENBUFF "34XhjyW89L"

#define CODE_SNDUSRNM "aJ4WqzZVA6"
#define CODE_ERRSNDMSG "xwe4ovjxjp"
#define CODE_OKUSRNM "Qc7AGZskGf"
#define CODE_ERRORUSRNM "R1BFTtnH2K"
#define CODE_OLDUSRNM "dZGmkzp2IA"
#define CODE_TMPOLDUSRNM "JtMkfqQV6j"
#define CODE_SNDPASS "whl5IuY3LV"
#define CODE_OKPASS "3Z2X1c4VbN"
#define CODE_ERRORPASS "cbNPOdY2ZJ"
#define CODE_SNDPASSLEN "4Z3X2c1VbN"


#define CODE_SNDUSRMTT "D6cO0er16T"
#define CODE_ENDVIS "NJ1kqGEnp6"
#define CODE_CONVIS "ejlWQUsgmR"
#define CODE_IDMSG "vrFoeq6awb"

#define CODE_OKDEL "SV936ds9tO"
#define CODE_ERRORDEL "Jfu2Sa3h9Q"

#define CODE_OKSNDMSG "RWCn1jflC9"
#define CODE_SNDUSRDST "VWbozi85be"
#define CODE_OKUSRDST "t0Z64KjmWx"
#define CODE_ERRORUSRDST "mfk0HnbgjD"
#define CODE_SNDOBJ "NtCvc4UT18"
#define CODE_OKOBJ "imcIpwAypt"
#define CODE_SNDTXT "MTHkSVIrsw"
#define CODE_OKTXT "3Z2Mn03VbN"

extern const size_t lenght_code;
extern const char *dir_messages;
extern const int max_key;

extern const size_t len_max;
extern const char *padding;

extern pthread_mutex_t *userTable_mux;
extern pthread_mutex_t *authTable_mux;
extern pthread_mutex_t *tempUserNameTable_mux;
extern pthread_mutex_t *file_auth_mux;
extern pthread_mutex_t *clients_mux;
extern pthread_mutex_t *user_register_mux;

extern auth_entry *authTable[MAX_KEY];
extern user_entry *userTable[MAX_KEY];
extern char *tempUserNameTable[MAX_KEY];
//in realtà per tempUserNameTable il numero massimo di elementi è pari al numero massimo di client connessi contemporaneamente,
// ma per utilizzare lo stesso hashing, si è deciso di utilizzare la stessa dimensione della tabella degli utenti registrati

extern const char *authFile;

extern int online_clients;
extern int utenti_registrati;

extern const char* pass_set;
extern const char* salt_set;


void exit_client(int sock);
bool write_on_sock(int sock,const void *buffer, size_t n);
bool read_from_sock(int sock,void *buffer, size_t n);
char* padstring(char* input);
void mostra_messaggi(int sock, char *user, user_entry **hashTable);
void invia_messaggio(char *usermtt, int sock, user_entry **hashTable);
void rimuovi_messaggio(int sock, char *user, user_entry *utente);
void saveToFile(const char *filename, char *user, auth_entry **hashTable);
void loadFromFile(const char *filename, auth_entry **hashTable_auth, user_entry **hashTable_user);
char *login(int sock, auth_entry **hashTable);
char *registrazione(int sock, const char *filename, auth_entry **hashTable_auth, user_entry **hashTable_user, char **hashTable_temp);
char *autenticazione(int sock);
void *client_thread(void *param);

#endif






