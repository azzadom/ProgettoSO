#ifndef AUTHFUNCTIONS_H
#define AUTHFUNCTIONS_H

#include <stdbool.h>

extern int hash(char *key, int i, int num_keys);

void newEntry_auth(char* user,char* salt,char* pass, auth_entry **hashTable);
auth_entry *verificaEntry_auth(char *user, auth_entry **hashTable);
void newTempUsername(char * user, char **hashTable);
bool verificaTempUsername(char *user, char **hashTable);
void freeTempUsername(char* user, char **hashTable);
void genera_stringa(char *buff, int size, const char *char_set);
bool verifica_pass(char *buff, int size);
bool verifica_salt(char *buff, int size);
void generatore_valido(char *buff, int size, const char *char_set, bool verifica_funzione(char *buff, int size));
bool verifica_username(char *buff, int size);

#endif
