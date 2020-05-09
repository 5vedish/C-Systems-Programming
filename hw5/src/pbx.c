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
    sem_t stt_lck; //semaphore for tu state changes
};

struct pbx{
    TU tel_units[PBX_MAX_EXTENSIONS]; //holds all of the telephone units
    sem_t fun_lck; //semaphore to lock pbx functions
    int curr_ext; //current number of extensions  
};

PBX *pbx_init(){
    PBX *new_pbx = Malloc(sizeof(PBX)); //declaring new struct
    sem_init(&new_pbx -> fun_lck, 0, 1); //initializing the semaphore for pbx lock
    new_pbx -> curr_ext = 0; //initializing current extensions to 0

    for (int i = 0; i < PBX_MAX_EXTENSIONS; i++){ //initialize all tu semaphores
        Sem_init(&(new_pbx -> tel_units[i]).stt_lck, 0, 1);
    }

    return new_pbx;
}

void pbx_shutdown(PBX *pbx){
    //remember to check for shutdown in register, ie. set a flag 
}

TU *pbx_register(PBX *pbx, int fd){
    P(&pbx -> fun_lck); //lock pbx
    char *hook_msg = Malloc(20);

    if (pbx -> curr_ext == PBX_MAX_EXTENSIONS){ //if full
        return NULL;
    }

    TU *new_tu = &pbx -> tel_units[pbx -> curr_ext++]; //putting struct inside array
    P(&new_tu -> stt_lck); 
    new_tu -> cur_stt = TU_ON_HOOK; //telephone units start off on hook
    V(&new_tu -> stt_lck);
    new_tu -> fd_ext = fd; //setting fd and extension as the same value
    new_tu -> conn = -1; //no connection initially

    sprintf(hook_msg, "%s %d\n", tu_state_names[0], fd);
    Write(fd, hook_msg, strlen(hook_msg)); //print on hook
         
    Free(hook_msg);

    V(&pbx -> fun_lck); //unlock pbx
    return &pbx -> tel_units[(pbx -> curr_ext) - 1]; //returning the address in the array
}

int pbx_unregister(PBX *pbx, TU *tu){
    P(&pbx -> fun_lck); //lock pbx

    if (pbx -> curr_ext == 0){ //if empty
        return -1;
    }

    int i; 
    for (i = 0; i < PBX_MAX_EXTENSIONS; i++){
        if ((*pbx).tel_units[i].fd_ext == (tu -> fd_ext)){
            break;
        }
    }

    for ( ; i < PBX_MAX_EXTENSIONS-1; i++){ //shifting the tus over
        pbx -> tel_units[i] = pbx -> tel_units[i+1];
    }

    pbx -> curr_ext--; //decrementing current number of extensions

    V(&pbx -> fun_lck); //unlock pbx
    return 0;
}

int tu_fileno(TU *tu){
    return tu -> fd_ext; 
}

int tu_extension(TU *tu){
    return tu -> fd_ext; 
}

int tu_pickup(TU *tu){
    char *msg = Malloc(20);
    TU *other_tu;

    if (tu -> conn != -1){
        int i;
        for (i = 0; i <PBX_MAX_EXTENSIONS; i++){
            if ((*pbx).tel_units[i].fd_ext == tu -> conn){
                break;
            }
        }

        other_tu = &(pbx -> tel_units[i]); //the connected telephone
    }

    if (tu -> cur_stt == TU_ON_HOOK){
        tu -> cur_stt = TU_DIAL_TONE;
        sprintf(msg, "%s\n", tu_state_names[2]);
        Write(tu -> fd_ext, msg, strlen(msg)); //write dial tone message
    }
    else if (tu -> cur_stt == TU_RINGING){
        

        sprintf(msg, "%s %d\n", tu_state_names[5], other_tu -> fd_ext);

        tu -> cur_stt = TU_CONNECTED;
        Write(tu -> fd_ext, msg, strlen(msg)); //writing the connected message

        sprintf(msg, "%s %d\n", tu_state_names[5], tu -> fd_ext);

        other_tu -> cur_stt = TU_CONNECTED;
        Write(other_tu -> fd_ext, msg, strlen(msg));

    }

    return 0;
}

int tu_hangup(TU *tu){
    char *msg = Malloc(20);
    TU *other_tu; //possible other telephone

    if (tu -> conn != -1){
        int i;
        for (i = 0; i <PBX_MAX_EXTENSIONS; i++){
            if ((*pbx).tel_units[i].fd_ext == tu -> conn){
                break;
            }
        }

        other_tu = &(pbx -> tel_units[i]); //the connected telephone
    }

    if ((tu -> cur_stt == TU_DIAL_TONE) || (tu -> cur_stt == TU_BUSY_SIGNAL) || (tu -> cur_stt == TU_ERROR)){
        tu -> cur_stt = TU_ON_HOOK;
        sprintf(msg, "%s %d\n", tu_state_names[0], tu -> fd_ext);
        Write(tu -> fd_ext, msg, strlen(msg));
        Free(msg);
        return 0;
    } 

    if ((tu -> cur_stt == TU_CONNECTED) || (tu -> cur_stt == TU_RINGING)){
        tu -> conn = -1;
        other_tu -> conn = -1;
        other_tu -> cur_stt = TU_DIAL_TONE;
        sprintf(msg, "%s\n", tu_state_names[2]);
        Write(other_tu -> fd_ext, msg, strlen(msg));

    } else if (tu -> cur_stt == TU_RING_BACK){
        other_tu -> cur_stt = TU_ON_HOOK;
        sprintf(msg, "%s %d\n", tu_state_names[0], other_tu -> fd_ext);

    }

    tu -> cur_stt = TU_ON_HOOK;
    sprintf(msg, "%s %d\n", tu_state_names[0], tu -> fd_ext);
    Write(tu -> fd_ext, msg, strlen(msg));

    Free(msg);

    return 0;
}

int tu_dial(TU *tu, int ext){
    char *msg = Malloc(20);

    if (ext < 0){ //if invalid extension
        return -1;
    }

    int i;
    for ( i = 0; i < PBX_MAX_EXTENSIONS; i++){ //finding if valid extension
        if ((*pbx).tel_units[i].fd_ext == ext){
            break;
        }
    }

    debug("%d", i);

    if (i == PBX_MAX_EXTENSIONS){ //if it wasn't found
        tu -> cur_stt = TU_ERROR;
        sprintf(msg, "%s\n", tu_state_names[6]);
        Write(tu -> fd_ext, msg, strlen(msg)); //error message
        return -1;
    }

    TU *other_tu = &(pbx -> tel_units[i]); //the TU to dial

    if (tu -> cur_stt == TU_DIAL_TONE){

        if (other_tu -> cur_stt == TU_ON_HOOK){
            tu -> conn = other_tu -> fd_ext; //setting potential connections
            other_tu -> conn = tu -> fd_ext;
            tu -> cur_stt = TU_RING_BACK;
            sprintf(msg, "%s\n", tu_state_names[3]);
            Write(tu -> fd_ext, msg, strlen(msg)); //writing ring back message
            other_tu -> cur_stt = TU_RINGING;
            sprintf(msg, "%s\n", tu_state_names[1]);
            Write(other_tu -> fd_ext, msg, strlen(msg)); //writing ringing message
        } 
        else {
            tu -> cur_stt = TU_BUSY_SIGNAL;
            sprintf(msg, "%s\n", tu_state_names[4]);
            Write(tu -> fd_ext, msg, strlen(msg)); //writing busy signal message
        }

    }
    Free(msg);

    return 0;
}

int tu_chat(TU *tu, char *msg){
    char *p_msg = Malloc(20);

    if (tu -> cur_stt != TU_CONNECTED){
        return -1;
    }

    sprintf(p_msg, "chat %s\n", msg);
    Write(tu -> conn, p_msg, strlen(p_msg));
    
    sprintf(p_msg, "%s %d\n", tu_state_names[5], tu -> conn);
    Write(tu -> fd_ext, p_msg, strlen(p_msg));

    Free(p_msg);

    return 0;
}














