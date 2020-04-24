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

//Method Declarations
int ass_prb(int workers);
int chk_stt(sig_atomic_t state, int workers, int num);
int fnd_pid(pid_t pid);
void snd_sig(int workers, int sig);

//Signal Handlers
void sigchild_handler(int sig){ //SIGCHLD handler
    pid_t pid;
    int status;
    int old_status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WORKER_CONTINUED)) != 0){
        int pid_index = fnd_pid(pid);
        old_status = statuses[pid_index];

        //if it was stopped, use previous state to determine the next state
        if (WIFSTOPPED(status)){
            debug("WORKER %d HAS BEEN STOPPED!", pid_index);
            //either started/stopped -> idle
            if (statuses[pid_index] == WORKER_STARTED || statuses[pid_index] == WORKER_STOPPED){
                statuses[pid_index] = WORKER_IDLE;
            } else {
                //else it went from running -> stopped
                statuses[pid_index] = WORKER_STOPPED;
            }
        }

        int i = WIFEXITED(status);
        if (i == EXIT_SUCCESS){
            //if it exited normally
            statuses[pid_index] = WORKER_EXITED;
        } else {
            //there was an issue and it was aborted
            statuses[pid_index] = WORKER_ABORTED;
        }

        
        sf_change_state(pid,old_status, statuses[pid_index]);

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
    for (i = 0; i < workers; i++){ //generating the workers
        pipe(temp_fd);
        fd[i] = temp_fd[0];
        fd[i+workers] = temp_fd[1]; //setting up the read and write pipes for master

        pipe(temp_fd);
        fd[i+(2*workers)] = temp_fd[0];
        fd[i+(3*workers)] = temp_fd[1]; //setting up the read and write pipes for worker

        statuses[i] = WORKER_STARTED;
        if((wrk_arr[i] = Fork()) == 0){
            dup2(fd[i+2*workers], STDIN_FILENO); 
            dup2(fd[i+3*workers], STDOUT_FILENO); //redirecting input and output in worker processes
            execl("./bin/polya_worker", "./bin/polya_worker"); //execute the workers

            Close(fd[i]);
            Close(fd[i+workers]); //close fds for master in worker
        } else{
            Close(fd[i+2*workers]);
            Close(fd[i+3*workers]); //close fds for worker in master
        }
    }

    debug("PIPES ARE SET!");

    //main loop
    while (1){
    debug("RUNNING MAIN LOOP!");

    if (chk_stt(WORKER_IDLE, workers, workers)){ //if they are all stopped
       if (ass_prb(workers) == -1){ //assigning the problems and if no more problems
            if (chk_stt(WORKER_IDLE, workers, workers) == 1){
                snd_sig(workers, SIGTERM); //terminate the workers
            }

            if (chk_stt(WORKER_EXITED, workers, workers) == 1){
                debug("you did a great job");
                sf_end();
                return EXIT_SUCCESS;
            }
       } 

        for (i = 0; i < workers; i++){
            Kill(wrk_arr[i], SIGCONT); //resume the workers
            statuses[i] = WORKER_CONTINUED;
            sf_change_state(wrk_arr[i], WORKER_IDLE, WORKER_CONTINUED);
        }
    }

    for (i = 0; i < workers; i++){
        if(statuses[i] == WORKER_CONTINUED){ //if it's continued, send it the problem
            Write(fd[i+workers], prb_arr[i], prb_arr[i] -> size); //writing from write end of master
            sf_send_problem(wrk_arr[i], prb_arr[i]);
        }
    }   

    for (i = 0; i < workers; i++){
        if(statuses[i] != WORKER_STOPPED){ //if it has already solved it
            statuses[i] = WORKER_RUNNING; //set workers to running, as they are solving problems
        } 
    }

    for (i = 0; i < workers; i++){
        if (statuses[i] == WORKER_STOPPED){ //if it's stopped, check result
        debug("SOLUTION CHECKING HAPPENED!");
            struct result *solution = Malloc(sizeof(struct result));
            Read(fd[i], solution, sizeof(struct result));

            if (solution -> failed > 0){ //if it wasn't canceled 
                ((solution -> size - sizeof(struct result)) == 0) ? 0 : Realloc(solution, solution -> size); //read if there's a data section
                ((solution -> size - sizeof(struct result)) == 0) ? 0 : Read(fd[i], solution -> data, solution -> size - sizeof(struct result));
            }

            if (post_result(solution, prb_arr[i]) == 0){ //if the result is correct, cancel other works
                snd_sig(workers, SIGHUP);
                for (i = 0; i < workers; i++){
                    Free(prb_arr[i]); //freeing the problems that were malloced
                }
            }

        }
    }


    if (chk_stt(WORKER_ABORTED, workers, workers) == 1){ //if any of them aborted
        break;
    }

    }

    sf_end(); //end of function

    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
}

int ass_prb(int workers){ //returns -1 if no more problems

    struct problem *to_write = Malloc(sizeof(struct problem)); //space for a problem

    int i; //counter
    for (i = 0; i < workers; i++){
            if((to_write = ((struct problem *) get_problem_variant(i, workers))) == NULL){
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
    debug("CHECKING STATES!");
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

void snd_sig(int workers, int sig){ //send sighup to workers when they are to be canceled
    int i; //counter
    for (i = 0; i < workers; i++){
        Kill(wrk_arr[i], sig);
        sf_cancel(wrk_arr[i]);
    }
}



