#include <stdlib.h>

#include "debug.h"
#include "polya.h"

//My Headers
#include <csapp.h>
#include <unistd.h>

//My Variables

volatile sig_atomic_t terminated;
volatile sig_atomic_t canceled;

//Signal Handlers
void sighup_handler(int sig){
    canceled = 1;
}

void sigterm_handler(int sig){ //terminate child process via exit
    canceled = 1;
    terminated = 1;
}


/*
 * worker
 * (See polya.h for specification.)
 */
int worker(void) {

    Signal(SIGHUP, sighup_handler); //signal handlers
    Signal(SIGTERM, sigterm_handler);

    raise(SIGSTOP); //stop itself after initialization

    struct problem *to_read = Malloc(sizeof(struct problem));

    while (1){

    //Read from master process
    Read(STDIN_FILENO, to_read, sizeof(struct problem));
    debug("Reading Problem Header:(%lu, %i, %i, %i, %i)", to_read -> size, to_read -> type, to_read -> id, to_read -> nvars, to_read -> var);

    Realloc(to_read, to_read -> size); //Make space for the follow info
    //Read the following data
    ((to_read -> size - sizeof(struct result)) == 0) ? 0 : Read(STDIN_FILENO, to_read -> data, to_read -> size - sizeof(struct problem)); //Don't read twice if size is 0
    debug("Reading Problem Info: (%s)", to_read -> data);

    debug("______________________________");
    //Attempt to solve the problem
    struct result *to_write = solvers[to_read -> type].solve(to_read, &canceled);

    debug("______________________________");
    //Write to master process
    Write(STDOUT_FILENO, to_write, sizeof(struct result));

    ((to_write -> size - sizeof(struct result)) == 0) ? 0 : Write(STDOUT_FILENO, to_write, to_write -> size - sizeof(struct result)); //Don't write twice if size is 0

    raise(SIGSTOP); //Stop itself after sending result
    
    if (terminated){ //exit upon sigterm
        exit(EXIT_SUCCESS);
    }    

    if (canceled){ //if SIGHUP, stop itself *incomplete
        //might have to write the result stuff here
        raise(SIGSTOP);
    }

    }
    
    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
}
