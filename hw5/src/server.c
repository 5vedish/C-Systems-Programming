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
#include <string.h>

extern PBX *pbx; //to gain access to the pbx

void *pbx_client_service(void *arg){

    int connfd = *((int *) arg); //casting pointer to int pointer to retrieve file descriptor
    pthread_detach(pthread_self()); //detaching the thread

    Free(arg); //freeing the file descriptor here after detaching

    TU *tel; //the telephone unit
    tel = pbx_register(pbx, connfd); //register the client with the pbx module

    char *fin_str = Malloc(sizeof(char)); //final string
    char *temp = Malloc(sizeof(char)); //temp char holder

    char *pickup = "pickup\r\n"; //strings to compare to
    char *hangup = "hangup\r\n";
    char *dial = "dial ";
    char *chat = "chat"; 

    int ext_temp; //temp for extension

    //main loop
    while (1){
    //set up and resetting

    int f_cnt = 1; //counter for reallocation
    int ext = 0; //veriable for extension
    int end, done = 0; //for breaking out of loop
    
    do {

        fin_str = Realloc(fin_str, ++f_cnt); //reallocating for space
        end = Read(connfd, temp, 1); //read the char in
        if (end == 0){
            debug("IT HITS THE END");
            done = 1;
            break;
        }
        strcat(fin_str, temp); //add the parsed char to the overall string

    } while (*temp != '\n'); //stop if reaches the end

    if (done == 1){
        debug("IT HITS THIS THING");
        break; //end the loop if done
    }
    
    if (strcmp(fin_str, pickup) == 0){ //if the command is pickup
        debug("THIS WORKS");
        tu_pickup(tel);

    } else if (strcmp(fin_str, hangup) == 0){ //if the command is hangup
        debug("THIS WORKS HANGUP");
        tu_hangup(tel);

    } else if (strncmp(fin_str, dial, 5) == 0){ // if the command is dial #
        debug("DIAL WORKS");

        int sec_spc = 0; //to detect the second space
        int d_cnt = 5;

        while ( (ext_temp = *(fin_str+(d_cnt++))) != '\r'){

            if ((ext_temp < 48 || ext_temp > 57) && ext_temp != ' '){ //throw it an error if it's not a digit or space
                debug("GIVE IT AN ERROR");
                ext = -1; 
                break; 
            } else if (ext_temp == ' '){

                if(sec_spc == 1){
                    break;
                }

                debug("HIT LEADING SPACE");
            } else{
                ext *= 10;
                ext += (ext_temp - 48);
                sec_spc = 1;
            }
            
        }
        //dial the extension
        debug("%d", ext);
        tu_dial(tel, ext);

    } else if (strncmp(fin_str, chat, 4) == 0){
        debug("CHAT WORKS");
        char *chat_str = Malloc(strlen(fin_str)); //allocate everything but the two escape chars
        strncpy(chat_str, fin_str, strlen(fin_str)-2); //copy everything but the escapes
        *(chat_str + strlen(fin_str) - 2) = '\0'; //manually setting null terminator

        int start = 4; //where the actual message starts from
        while (*(chat_str+start) == ' '){
            start++;
        }
        debug("%d", start);
        debug("%s", chat_str);
        tu_chat(tel, chat_str+start); //to get rid of the spaces
        Free(chat_str); //free the string to chat

    }

    *fin_str = '\0'; //reset null terminator

    }
    debug("YOU'VE SUCCESFFULLY CLOSED IT");

    //freeing the pointers used
    Free(fin_str);
    Free(temp);

    pbx_unregister(pbx, tel); //unregister the telephone



    Close(connfd); //closing the file descriptor

    return NULL;
}

