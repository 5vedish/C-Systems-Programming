#include <stdlib.h>

#include "debug.h"
#include "polya.h"

//My Headers
#include <csapp.h>
#include <unistd.h>

//My Variables

volatile sig_atomic_t canceled;

//Signal Handlers
void sighup_handler(int sig){ //just setting the flag
    canceled = 1;
}

void sigterm_handler(int sig){ //terminate child process via exit  
    _exit(EXIT_SUCCESS);
}

/*
 * worker
 * (See polya.h for specification.)
 */
int worker(void) {

    Signal(SIGHUP, sighup_handler); //signal handlers
    Signal(SIGTERM, sigterm_handler);

    raise(SIGSTOP); //stop itself after initialization

    while (1){
    struct problem *to_read = Malloc(sizeof(struct problem)); //make space for problem

    //read from master process
    Read(STDIN_FILENO, to_read, sizeof(struct problem));

    to_read = Realloc(to_read, to_read -> size); //make space for the follow info
    //read the following data
    ((to_read -> size - sizeof(struct problem)) == 0) ? 0 : Read(STDIN_FILENO, to_read -> data, to_read -> size - sizeof(struct problem)); //don't read twice if size is 0

    //attempt to solve the problem
    struct result *to_write = solvers[to_read -> type].solve(to_read, &canceled);

    if (to_write == NULL){ //if null then send artifical result
        to_write = Malloc(sizeof(struct result));
        to_write -> size = sizeof(struct result);
        to_write -> failed = 1;
        to_write -> id = to_read -> id;
    }

    if (canceled){
        to_write -> failed = 1; //result is allocated but the problem is canceled
        canceled = 0; //so it could work on multiple problems
    }

    Free(to_read); //free the problem after things are written

    //write to master process
    Write(STDOUT_FILENO, to_write, sizeof(struct result));

    ((to_write -> size - sizeof(struct result)) == 0) ? 0 : Write(STDOUT_FILENO, to_write -> data, to_write -> size - sizeof(struct result)); //don't write twice if size is 0
    Free(to_write); //freeing the result

    raise(SIGSTOP); //stop itself after sending result  
    }
    
    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
}
