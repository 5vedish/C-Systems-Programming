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

static void terminate(int status);

//My Includes

#include <unistd.h>
#include <csapp.h>

//Signal Handlers
void sighup_handler(int sig){
    terminate(EXIT_FAILURE);
}

//My Macros/Declarations
#define MAX_DIGITS_PORT_NUM 5
void *thread(void *vargp);

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    //to check for proper flag and take in port number
    extern char *optarg;
    int c, d, port_num = 0;
    while ((c = getopt(argc, argv, "p:")) != EOF){
        switch(c){
            case 'p':{
                for (int i = 0; i < MAX_DIGITS_PORT_NUM; i++){
                    d = *(optarg+i);
                    if (d == 0){
                        break;
                    }
                    port_num *= 10;
                    port_num += (d-48); //grabbing proper digit
                }
                break;
            }
                
            default: 
                exit(EXIT_FAILURE);
        }
    }

    // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    Signal(SIGHUP, sighup_handler); //handle sighup for graceful termination

    int listenfd, *connfdp; //the descriptor to listen to and the connected descriptor
    socklen_t clientlen; //length of socket address
    struct sockaddr_storage clientaddr; //address of socket
    pthread_t tid; //identify thread

    listenfd = Open_listenfd(port_num); //listen to given port number

    while (1){
        clientlen = sizeof(struct sockaddr_storage); //hold size of storage
        connfdp = Malloc(sizeof(int)); //store fd
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); //accept connections
        pthread_create(&tid, NULL, thread, connfdp); //create a new thread for each connection
    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the PBX server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}

//Thread Function
void *thread(void *vargp){
    pthread_detach(pthread_self()); //detaching itself from other threads
    pbx_client_service(vargp); //passing the descriptor from which to communicate from
    return NULL;
}
