#ifndef GLOBALVAR_H
#define GLOBALVAR_H

//Numero massimo di chiavi per le tabelle hash
#define MAX_KEY 1531 //Numeri primi, distanti da potenze di 2 (e di 10) come  13, 23, 47, 97, 193, 383, 769, 1531, 6143, 12289
#define MAX_USERS_ONLINE 10
#define MAX_KEY_TEMP 23 //Numero massimo di chiavi per la tabella hash dei nomi utente temporanei scelto come MAX_KEY e che sia maggiore di MAX_USERS_ONLINE*2

#define MAX_OBJECT_LENGHT 50
#define MAX_TEXT_LENGHT 1000
#define MAX_MESAGES 1000

#define MAX_USERNAME_LENGHT 10
#define MIN_USERNAME_LENGHT 8

#define MIN_PASSWORD_LENGHT 12
#define MAX_PASSWORD_LENGHT 20
#define SALT_LEN 8 //Se si vuole cambiare la lunghezza del sale bisogna tener conto che è valido solo se la funzione verifica_salt() lo accetta e dunque non può contenere meno di 2 caratteri
#define CODE_LEN 3 //"$6$"
#define CODE_CRYPT "$6$" //Codice per la funzione crypt() che indica l'algoritmo da usare, in questo caso SHA-512
#define MAX_PASSWORD_LENGHT_STORED 86 //Lunghezza della password criptata con SHA-512

typedef struct auth_entry {
    char *username;
    char *usersalt;
    char *userpass;
} auth_entry;

typedef struct index{
    int num_messaggi;
    off_t start_offset[MAX_MESAGES];
    off_t end_offset[MAX_MESAGES];
} index_t;

typedef struct message {
    char mittente[MAX_USERNAME_LENGHT+1];
    char oggetto[MAX_OBJECT_LENGHT+1];
    char testo[MAX_TEXT_LENGHT+1];
} message;

typedef struct user_entry {
    char *username;
    pthread_mutex_t *mutex;
} user_entry;


#endif
