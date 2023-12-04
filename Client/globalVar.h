#ifndef GLOBALVAR_H
#define GLOBALVAR_H

#include <stdio.h>
#include <stdbool.h>

#define fflush(stdin) while(getchar() != '\n')

#define MAXBUFF 1024

#define MAX_OBJECT_LENGHT 50
#define MAX_TEXT_LENGHT 1000
#define MAX_MESAGES 1000

#define MAX_USERNAME_LENGHT 10
#define MIN_USERNAME_LENGHT 8

#define MIN_PASSWORD_LENGHT 12
#define MAX_PASSWORD_LENGHT 20

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
#define CODE_ERRSNDMSG "xwe4ovjxjp"
#define CODE_SNDUSRDST "VWbozi85be"
#define CODE_OKUSRDST "t0Z64KjmWx"
#define CODE_ERRORUSRDST "mfk0HnbgjD"
#define CODE_SNDOBJ "NtCvc4UT18"
#define CODE_OKOBJ "imcIpwAypt"
#define CODE_SNDTXT "MTHkSVIrsw"
#define CODE_OKTXT "3Z2Mn03VbN"

extern size_t lenght_code;
extern size_t len_max; 
extern const char *padding;

void write_on_sock(int sock, void *buffer, size_t n);
void read_from_sock(int sock, void *buffer, size_t n);
char* padstring(char* input);
char *getText(int maxlen);
void visualizza_messaggi(int conn_s);
void invia_messaggio(int conn_s);
void elimina_messaggio(int conn_s);
bool login(int conn_s);
void registrazione(int conn_s);
void autenticazione(int conn_s);

#endif




