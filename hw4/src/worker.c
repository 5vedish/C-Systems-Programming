#include <stdlib.h>

#include "debug.h"
#include "polya.h"

//My Headers
#include <csapp.h>
#include <unistd.h>

//My Variables

volatile sig_atomic_t terminated;
volatile sig_atomic_t cancelled;

//Signal Handlers
void sighup_handler(int sig){
    cancelled = 1;
}

void sigterm_handler(int sig){ //terminate child process via exit
    terminated = 1;
}


/*
 * worker
 * (See polya.h for specification.)
 */
int worker(void) {

    Signal(SIGHUP, sighup_handler); //signal handlers
    Signal(SIGTERM, sighup_handler);

    raise(SIGSTOP); //stop itself after initialization

    struct problem *to_read = Malloc(sizeof(struct problem));

    while (1){

    debug("________________________________________BOOM");

    //Read from master process
    fread(to_read, 1, sizeof(struct problem), stdin);
    // Read(STDIN_FILENO, to_read, sizeof(struct problem));
    // fgetc(stdin);

    debug("%lu", to_read -> size);

    debug("________________________________________BOOM2");

    if (terminated){ //exit upon sigterm
        exit(EXIT_SUCCESS);
    }    

    if (cancelled){ //if SIGHUP, stop itself *incomplete
        raise(SIGSTOP);
    }


    }
    
    
  
    

    


    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
}
