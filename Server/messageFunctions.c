#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>

#include "globalVar.h"
#include "messageFunctions.h"

//Inserisce una nuova entry nella tabella hash
void newEntry_user(char* user, user_entry **hashTable) {

    int i = 0;
    user_entry *utente;
    utente = (user_entry *) calloc(1, sizeof(user_entry));
    if(utente == NULL) {
        printf("Errore allocazione memoria in newEntry_user.\n");
        exit(EXIT_FAILURE);
    }

    utente->mutex = (pthread_mutex_t *) calloc(1, sizeof(pthread_mutex_t));
    if(utente->mutex == NULL) {
        printf("Errore allocazione memoria in newEntry_user.\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_mutex_init(utente->mutex, NULL)){
        printf("Errore inizializzazione mutex in newEntry_user.\n");
        exit(EXIT_FAILURE);
    }
    utente->username = user;

    int hashIndex = hash(user, i, MAX_KEY);

    while(hashTable[hashIndex] != NULL) {
        i++;
        hashIndex = hash(user, i, MAX_KEY);
    }

    hashTable[hashIndex] = utente;
}

//Verifica che l'utente sia presente nella tabella hash
user_entry *verificaEntry_user(char *user, user_entry **hashTable) {

    int i = 0;
    int hashIndex = hash(user,i, MAX_KEY);

    while (hashTable[hashIndex] != NULL) {
        if (strcmp(hashTable[hashIndex]->username, user) == 0) {
            return hashTable[hashIndex];
        }
        i++;
        hashIndex = hash(user, i, MAX_KEY);
    }
    return NULL;
}

/*----------------------------------------------------------------------*/

//carica l'indice dal file index
index_t *carica_index(int fd_index){
    index_t *index;
    int res;

    index = (index_t *) calloc(1, sizeof(index_t));
    if(index == NULL){
        printf("Errore allocazione memoria in carica_index.\n");
        exit(EXIT_FAILURE);
    }
    off_t seek_start = lseek(fd_index, 0, SEEK_SET);
    off_t seek_end = lseek(fd_index, 0, SEEK_END);

    if(seek_start==seek_end){
        index->num_messaggi = 0;
        index->start_offset[0] = 0;
        index->end_offset[0] = 0;
        return index;
    }

    lseek(fd_index,0,SEEK_SET);
    res = read(fd_index, index, sizeof(index_t));
    if(res==-1){
        exit(EXIT_FAILURE);
    }
    return index;
}

//aggiorna l'indice sul file index
void aggiorna_index(int fd_index, index_t *index){

    lseek(fd_index,0,SEEK_SET);
    write(fd_index, index, sizeof(index_t));
    fsync(fd_index);
}

//inserisce il messaggio nel file dei messaggi
bool inserisci_messaggio(int fd_dest, int fd_index, message *msg){

    index_t *index;
    int num;

    off_t seek_start, seek_end;

    index = carica_index(fd_index);
    num = index->num_messaggi + 1;
    if(num>MAX_MESAGES){
        free(index);
        return false;
    }

    index->num_messaggi = (index->num_messaggi)+1;

    seek_start = lseek(fd_dest,0,SEEK_END);
    index->start_offset[(index->num_messaggi)-1] = seek_start;
    write(fd_dest, msg, sizeof(message));
    fsync(fd_dest);
    seek_end = lseek(fd_dest,0,SEEK_CUR);
    index->end_offset[(index->num_messaggi)-1] = seek_end;
    aggiorna_index(fd_index, index);
    free(index);

    return true;

}

//visualizza un messaggio preso dal file dei messaggi
message *visualizza_messaggio(int fd, index_t *index, int ID){

    int res;

    message *msg = (message *) calloc(1, sizeof(message));
    if(msg == NULL){
        printf("Errore allocazione memoria in visualizza_messaggi.\n");
        exit(EXIT_FAILURE);
    }

    if(ID>(index->num_messaggi) || ID<1) return NULL;
    lseek(fd, (index->start_offset[ID-1]), SEEK_SET);
    res = read(fd, msg, sizeof(message));
    if(res == -1){
        printf("Errore lettura messaggio in visualizza_messaggi.\n");
        exit(EXIT_FAILURE);
    }

    return msg;
}

//elimina un messaggio dal file dei messaggi
bool elimina_messaggio(int *fd, int *fd_index, int ID, char *path_main, char *path_ind){

    int res, ID_old;
    int new_fd;
    off_t start_del, end_del, size_delete;
    char *tmp_name;
    char *index_file;
    message *msg;

    index_t *index = carica_index(*fd_index);

    int i = ID-1;
    if(ID>(index->num_messaggi) || i<0) return false;

    start_del = index->start_offset[i];
    end_del = index->end_offset[i];
    size_delete =  end_del - start_del;

    index_t *ind_new = (index_t *) calloc(1, sizeof(index_t));
    if(ind_new == NULL){
        printf("Errore allocazione memoria in elimina_messaggio.\n");
        exit(EXIT_FAILURE);
    }

    ind_new->num_messaggi = (index->num_messaggi)-1;
    for(int j=0; j<i; j++){
        (ind_new->start_offset)[j] = (index->start_offset)[j];
        (ind_new->end_offset)[j] = (index->end_offset)[j];
    }

    for(int j=i+1; j<index->num_messaggi; j++){
        (ind_new->start_offset)[j-1] = (index->start_offset)[j]-size_delete;
        (ind_new->end_offset)[j-1] = (index->end_offset)[j]-size_delete;
    }

    close(*fd_index);
    remove(path_ind);
    *fd_index = open(path_ind, O_RDWR | O_CREAT, S_IRWXU);
    aggiorna_index(*fd_index, ind_new);

    tmp_name = (char *) calloc(strlen(path_main)+strlen("_tmp")+1, sizeof(char));
    if(tmp_name == NULL){
        printf("Errore allocazione memoria in elimina_messaggio.\n");
        exit(EXIT_FAILURE);
    }
    sprintf(tmp_name,"%s_tmp",path_main);
    new_fd = open(tmp_name, O_RDWR | O_CREAT, S_IRWXU);

    msg = (message *) calloc(1, sizeof(message));
    if(msg == NULL){
        printf("Errore allocazione memoria in elimina_messaggio.\n");
        exit(EXIT_FAILURE);
    }

    ID_old = 0;
    while(ID_old<(index->num_messaggi)){
        if(ID_old == i) {
            ID_old++;
            continue;
        }
        lseek(*fd, (index->start_offset)[ID_old], SEEK_SET);
        res = read(*fd, msg, sizeof(message));
        if (res == -1) {
            printf("Errore lettura messaggio in elimina_messaggio.\n");
            exit(EXIT_FAILURE);
        }
        write(new_fd, msg, sizeof(message));
        fsync(new_fd);
        ID_old++;
    }

    close(*fd);
    remove(path_main);
    rename(tmp_name, path_main);
    *fd = new_fd;

    free(tmp_name);
    free(msg);
    free(index);

    return true;
}
