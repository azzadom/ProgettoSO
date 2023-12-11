#ifndef MESSAGEFUNCTIONS_H
#define MESSAGEFUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern int hash(char *key, int i, int num_keys);

user_entry *verificaEntry_user(char *user, user_entry **hashTable);
void newEntry_user(char* user, user_entry **hashTable);
index_t *carica_index(int fd_index);
void aggiorna_index(int fd_index, index_t *index);
bool inserisci_messaggio(int fd_dest, int fd_index, message *msg);
message *visualizza_messaggio(int fd, index_t *index, int ID);
bool elimina_messaggio(int *fd, int *fd_index, int ID, char *path_main, char *path_ind);

#endif
