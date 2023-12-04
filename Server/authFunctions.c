#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "globalVar.h"
#include "authFunctions.h"

//Inserisce una nuova entry nella tabella hash
void newEntry_auth(char* user,char* salt,char* pass, auth_entry **hashTable) {

    int i = 0;
    auth_entry *utente;
    utente = (auth_entry *) calloc(1, sizeof(auth_entry));
    if(utente == NULL) {
        printf("Errore allocazione memoria in newEntry_auth.\n");
        exit(EXIT_FAILURE);
    }

    utente->usersalt = salt;
    utente->userpass = pass;
    utente->username = user;

    int hashIndex = hash(user, i);

    while(hashTable[hashIndex] != NULL) {
        i++;
        hashIndex = hash(user, i);
    }

    hashTable[hashIndex] = utente;
}

//Verifica se l'utente è presente nella tabella hash
auth_entry *verificaEntry_auth(char *user, auth_entry **hashTable) {

    int i = 0;
    int hashIndex = hash(user, i);

    while (hashTable[hashIndex] != NULL) {
        if (strcmp(hashTable[hashIndex]->username, user) == 0) {
            return hashTable[hashIndex];
        }
        i++;
        hashIndex = hash(user, i);
    }
    return NULL;
}

//Inserisce il nome utente durante la registrazione
void newTempUsername(char * user, char **hashTable){

    int i = 0;
    char *utente;

    utente = strdup(user);
    if(utente == NULL) {
        printf("Errore allocazione memoria in newTempUsername.\n");
        exit(EXIT_FAILURE);
    }

    int hashIndex = hash(user, i);

    while(hashTable[hashIndex] != NULL) {
        i++;
        hashIndex = hash(user, i);
    }

    hashTable[hashIndex] = utente;

}

//Verifica se è in corso di registrazione un utente con lo stesso nome
bool verificaTempUsername(char *user, char **hashTable){

    int i = 0;
    int hashIndex = hash(user, i);

    while (hashTable[hashIndex] != NULL) {
        if (strcmp(hashTable[hashIndex], user) == 0) {
            return true;
        }
        i++;
        hashIndex = hash(user, i);
    }
    return false;

}

//Elimina il nome utente non più in corso di registrazione
void freeTempUsername(char *user, char **hashTable){

    int i = 0;
    int hashIndex = hash(user, i);

    while (hashTable[hashIndex] != NULL) {
        if (strcmp(hashTable[hashIndex], user) == 0) {
            free(hashTable[hashIndex]);
            return;
        }
        i++;
        hashIndex = hash(user, i);
    }
}

/*---------------------------------------------------------------------*/

//generatore di stringhe della lunghezza indicata e con i caratteri del set indicato
void genera_stringa(char *buff, int size, const char *char_set){

    srand(time(NULL));
    int num_chars = strlen(char_set);

    for(int i=0; i<size; i++){
        buff[i] = char_set[rand() % num_chars];
    }

    buff[size] = '\0';
}

//verifica la validità della password
bool verifica_pass(char *buff, int size){

    bool upper = false;
    bool lower = false;
    bool digit = false;
    bool symbol = false;

  //controlla la presenza di almeno un carattere maiuscolo, minuscolo, numerico e un simbolo
    for (int i = 0; i < size; i++){  
        if (isupper(buff[i])) upper = true;
        if (islower(buff[i])) lower = true;
        if (isdigit(buff[i])) digit = true;
        if (ispunct(buff[i])) symbol = true; 
    }
  
  // return false se non soddisfa i requisiti
    if (!upper) return false;
    if (!lower) return false;
    if (!digit) return false;
    if (!symbol) return false;
  
    return true;
}

//verifica la validità del sale
bool verifica_salt(char *buff, int size){

    bool upper = false;
    bool lower = false;
    bool digit = false;
    bool symbol = false;

  //controlla la presenza di almeno un carattere maiuscolo o minuscolo e numerico o simbolico
    for (int i = 0; i < size; i++){  
        if (isupper(buff[i])) upper = true;
        if (islower(buff[i])) lower = true;
        if (isdigit(buff[i])) digit = true;
        if (ispunct(buff[i])) symbol = true; 
    }
  
  // return false se non soddisfa i requisiti
    if((upper||lower) && (digit||symbol)) return true;
    else return false;
}

//genera una stringa valida come password o come sale
void generatore_valido(char *buff, int size, const char *char_set, bool verifica_funzione(char *buff, int size)){

    bool flag = false;

    while (!flag){
        genera_stringa(buff, size, char_set);
        flag = verifica_funzione(buff, size);
    }
}

//verifica la validità dell'username inserito affinchè sia composto da caratteri alfanumerici e minuscoli
bool verifica_username(char *buff, int size){

    for(int i=0; i<size; i++){
        if((!isalnum(buff[i])) || isupper(buff[i])) return false;
    }

    return true;
}