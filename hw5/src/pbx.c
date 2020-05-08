#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"

//My Includes
#include <csapp.h>

struct tu{
    TU_STATE cur_stt; //current state
    int fd_ext; //serves as both file descriptor and extension number
    int conn; //the telephone unit that the current telephone is connected to
    sem_t stt_lck; //semaphore to lock the state of the telephone

};

struct pbx{
    TU tel_units[PBX_MAX_EXTENSIONS]; //holds all of the telephone units
    sem_t fun_lck; //semaphore to lock pbx functions
    int curr_ext; //current number of extensions  
};

PBX *pbx_init(){
    PBX new_pbx; //declaring new struct
    pbx = &new_pbx; //putting struct into global
    debug("%p", &pbx -> fun_lck);
    debug("%p", &new_pbx.fun_lck);
    sem_init(&(pbx -> fun_lck), 0, 1); //initializing the semaphore for function locks
    pbx -> curr_ext = 0; //current extensions are zero
    return pbx;
}

void pbx_shutdown(PBX *pbx){

}

TU *pbx_register(PBX *pbx, int fd){

    if (pbx -> curr_ext == PBX_MAX_EXTENSIONS){ //if full
        return NULL;
    }
    
    P(&(pbx -> fun_lck)); 
    debug("%p", &(pbx -> fun_lck));

    TU new_tu; //declaring new struct
    new_tu.cur_stt = TU_ON_HOOK; //telephone units start off on hook
    new_tu.fd_ext = fd; //setting fd and extension as the same value
    pbx -> tel_units[pbx -> curr_ext++] = new_tu; //assigned the tu to the proper position in the array
    sem_init(&new_tu.stt_lck, 0, 1); //intializing semaphore for state locks

    char *hook_msg = Malloc(100);
    sprintf(hook_msg, "ON HOOK %d\n", fd); //adding extension number to the string

    if (Write(fd, hook_msg, strlen(hook_msg)) == 0){ //getting the message to print on client
        return NULL;
    }

    Free(hook_msg); //freeing the string
    debug("%p", &(pbx -> fun_lck));
    V(&(pbx -> fun_lck));
    
    return &pbx -> tel_units[(pbx -> curr_ext) -1]; //returning the address in the array
}

int pbx_unregister(PBX *pbx, TU *tu){

    if (pbx -> curr_ext == 0){ //if somehow unregistering a TU that doesn't exist
        return -1;
    }

    P(&pbx -> fun_lck);

    int i; //counter
    for (i = 0; i < PBX_MAX_EXTENSIONS; i++){
        if ((*pbx).tel_units[i].fd_ext == (tu -> fd_ext)){
            break;
        }
    }

    for ( ; i < PBX_MAX_EXTENSIONS-1; i++){ //shifting the TU's over
        pbx -> tel_units[i] = pbx -> tel_units[i+1];
    }

    pbx -> curr_ext--; //decrementing current number of extensions

    V(&pbx -> fun_lck);

    return 0;
}

int tu_fileno(TU *tu){
    return tu -> fd_ext; //return the file descriptor/extension
}

int tu_extension(TU *tu){
    return tu -> fd_ext; //return the file descriptor/extension
}

int tu_pickup(TU *tu){
    return 0;
}

int tu_hangup(TU *tu){
    return 0;
}

int tu_dial(TU *tu, int ext){
    return 0;
}

int tu_chat(TU *tu, char *msg){
    return 0;
}














