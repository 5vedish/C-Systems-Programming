#include <stdlib.h>

#include "debug.h"
#include "polya.h"

//My Includes
#include <sys/types.h>
#include <sys/wait.h>
#include <csapp.h>

//My Variables
pid_t wrk_arr[MAX_WORKERS]; //array of workers
struct problem *prb_arr[MAX_WORKERS]; //array of problems
sig_atomic_t statuses[MAX_WORKERS]; //flags for the statuses
int fd[4*MAX_WORKERS]; //file descriptors
int temp_fd[2]; //temp file descriptors
sig_atomic_t done;

//Method Declarations
int ass_prb(int workers);
int chk_stt(sig_atomic_t state, int workers, int num);
int fnd_pid(pid_t pid);
void snd_sig(int workers, int sig, int no_inc);

//Signal Handlers
void sigchild_handler(int sig){ //SIGCHLD handler
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WORKER_CONTINUED)) > 0){
        int pid_index = fnd_pid(pid);

        //if it was stopped, use previous state to determine the next state
        if (WIFSTOPPED(status)){
            debug("WORKER %d HAS BEEN STOPPED!", pid_index);
            //either started/stopped -> idle
            if (statuses[pid_index] == WORKER_STARTED){
                sf_change_state(pid,statuses[pid_index], WORKER_IDLE);
                statuses[pid_index] = WORKER_IDLE;
            } else {
                //else it went from running -> stopped
                sf_change_state(pid,statuses[pid_index], WORKER_STOPPED);
                statuses[pid_index] = WORKER_STOPPED;
            }
        }

        int i = WEXITSTATUS(status);
        if (i == EXIT_SUCCESS){
            debug("WORKER %d HAS BEEN EXITED!", pid_index);
            //if it exited normally
            sf_change_state(pid,statuses[pid_index], WORKER_EXITED);
            statuses[pid_index] = WORKER_EXITED;
        } else if (i == EXIT_FAILURE) {
            //there was an issue and it was aborted
            sf_change_state(pid,statuses[pid_index], WORKER_ABORTED);
            statuses[pid_index] = WORKER_ABORTED;
        }

    }

}

void sigpipe_handler(int sig){ //SIGPIPE handler, to preserve master

}


/*
 * master
 * (See polya.h for specification.)
 */
int master(int workers) {

    sf_start(); //beginning of function
    debug("MASTER HAS STARTED!");

    //installing signals
    Signal(SIGCHLD, sigchild_handler);
    Signal(SIGPIPE, sigpipe_handler);


    int i; //counter
    pid_t pid;
    for (i = 0; i < workers; i++){ //generating the workers
        pipe(temp_fd);
        fd[i*4] = temp_fd[0];
        fd[i*4+1] = temp_fd[1]; //setting up the read and write pipes for master

        pipe(temp_fd);
        fd[i*4+2] = temp_fd[0];
        fd[i*4+3] = temp_fd[1]; //setting up the read and write pipes for worker

        statuses[i] = WORKER_STARTED;
        if((pid = Fork()) == 0){ 
            dup2(fd[i*4], STDIN_FILENO); 
            dup2(fd[i*4+3], STDOUT_FILENO); //redirecting input and output in worker processes

            char *argv[1];
            argv[0] = NULL;
            execl("bin/polya_worker", argv[0], (char *) NULL); //execute the workers

            Close(fd[i*4+1]);
            Close(fd[i*4+2]); //close fds for master in worker
        } else{
            wrk_arr[i] = pid; //save the pid in the worker
            Close(fd[i*4]);
            Close(fd[i*4+3]); //close fds for worker in master
        }
    }

    debug("PIPES ARE SET!");

    //main loop
    while (1){
//ALL EXITED
    if (chk_stt(WORKER_EXITED, workers, workers) == 1){
        debug("you did a great job");
        sf_end();
        return EXIT_SUCCESS;
        }
//ALL EXITED
//ALL IDLE
    if (chk_stt(WORKER_IDLE, workers, workers)){ //if they are all stopped
        if (!done){
            if (ass_prb(workers) == -1){ //assigning the problems and if no more problems
                done = 1;
                for (i = 0; i < workers; i++){ //send sigterm first and then sigcont
                    Kill(wrk_arr[i], SIGTERM);
                    Kill(wrk_arr[i], SIGCONT);
                }
       } 
        }

       if (!done){
           for (i = 0; i < workers; i++){
                Kill(wrk_arr[i], SIGCONT); //resume the workers
                statuses[i] = WORKER_CONTINUED;
                sf_change_state(wrk_arr[i], WORKER_IDLE, WORKER_CONTINUED);
            }
       }

    }
//ALL IDLE
//ALL CONTINUED
    for (i = 0; i < workers; i++){
        if(statuses[i] == WORKER_CONTINUED){ //if it's continued, send it the problem
            if (!done){
                Write(fd[i*4+1], prb_arr[i], prb_arr[i] -> size); //writing from write end of master 
                sf_send_problem(wrk_arr[i], prb_arr[i]);
            }
        }
    }   
    for (i = 0; i < workers; i++){
        if(statuses[i] == WORKER_CONTINUED && statuses[i] != WORKER_STOPPED){ //if it has already solved it or is continuing
            statuses[i] = WORKER_RUNNING; //set workers to running, as they are solving problems
            debug("A WORKER WAS SET TO RUNNING!");
        } 
    }
//ALL CONTINUED
//IF STOPPED
    for (i = 0; i < workers; i++){
        if (statuses[i] == WORKER_STOPPED){ //if it's stopped, check result
            struct result *solution = Malloc(sizeof(struct result));
            debug("WORKER %u", wrk_arr[i]);
            Read(fd[i*4+2], solution, sizeof(struct result));
            debug("WORKER %u", wrk_arr[i]);
            debug("SOLUTION CHECKING HAPPENED FOR WORKER %u", wrk_arr[i]);

            if (solution -> failed > 0){ //if it wasn't canceled 
                ((solution -> size - sizeof(struct result)) == 0) ? 0 : Realloc(solution, solution -> size); //read if there's a data section
                ((solution -> size - sizeof(struct result)) == 0) ? 0 : Read(fd[i*workers+2], solution -> data, solution -> size - sizeof(struct result));
            }
            
            if (post_result(solution, prb_arr[i]) == 0){ //if the result is correct, cancel other workers
                snd_sig(workers, SIGHUP, i);
                
            }
            Free(solution);
        }
    }

//IF STOPPED
//SET IDLE

    for (i = 0; i < workers; i++){ //set to idle if they stopped
        if (statuses[i] == WORKER_STOPPED){
            statuses[i] = WORKER_IDLE;
            sf_change_state(wrk_arr[i], WORKER_STOPPED, WORKER_IDLE);
        }
    }

    if (chk_stt(WORKER_STOPPED, workers, workers)){ //if all of them have been stopped
        for (i = 1; i < workers; i++){
                    Free(prb_arr[i]); //freeing the problems that were malloced
                }
    }

//SET IDLE
    if (chk_stt(WORKER_ABORTED, workers, workers) == 1){ //if any of them aborted
        break;
    }

    }
    debug("it's ok, you can try again");
    sf_end(); //end of function

    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
}

int ass_prb(int workers){ //returns -1 if no more problems

    struct problem *to_write; //pointer to problem

    int i; //counter
    for (i = 0; i < workers; i++){
            if((to_write = ((struct problem *) get_problem_variant(workers, i))) == NULL){
                //no more problems
                return -1;
            }
            prb_arr[i] = to_write; //saving the problem inside the array
        }
    debug("PROBLEMS HAVE BEEN ASSIGNED!");
    return 0;

}

//returns 0 if num of statuses found isn't expected and 1 if it is
int chk_stt(sig_atomic_t state, int workers, int num){
    int i, j = 0; //counters
    for (i = 0; i < workers; i++){
        if (statuses[i] == state){
            j++;
        }
    }

    return (j == num) ? 1: 0;
}

int fnd_pid(pid_t pid){ //search for and return index of the pid
    int i; //counter
    for (i = 0; i < MAX_WORKERS; i++){
        if(pid == wrk_arr[i]){
            return i;
        }
    }

    return -1;
}

void snd_sig(int workers, int sig, int no_inc){ //send sighup to workers when they are to be canceled
    int i; //counter
    for (i = 0; i < workers; i++){
        if (i != no_inc){
            Kill(wrk_arr[i], sig);
            (sig == SIGHUP) ? sf_cancel(wrk_arr[i]) : 0;
        }
    }
}



