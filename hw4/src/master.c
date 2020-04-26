#include <stdlib.h>

#include "debug.h"
#include "polya.h"

//My Includes
#include <sys/types.h>
#include <sys/wait.h>
#include <csapp.h>

//My Variables
pid_t wrk_arr[MAX_WORKERS]; //array of workers
sig_atomic_t statuses[MAX_WORKERS]; //flags for the statuses
int fd[4*MAX_WORKERS]; //file descriptors
int temp_fd[2]; //temp file descriptors
int temp_fd2[2];
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

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
        int pid_index = fnd_pid(pid);

        //if it was stopped, use previous state to determine the next state
        if (WIFSTOPPED(status)){
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
        if (i == EXIT_SUCCESS && WIFEXITED(status)){
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

    extern int sf_suppress_chatter;
    sf_suppress_chatter = 1;

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

        pipe(temp_fd2);
        fd[i*4+2] = temp_fd2[0];
        fd[i*4+3] = temp_fd2[1]; //setting up the read and write pipes for worker

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
    struct problem *to_write[MAX_WORKERS]; //problem array

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
    if (chk_stt(WORKER_IDLE, workers, workers)){ //if they are all idle
        if (!done){
            int i; //counter
            for (i = 0; i < workers; i++){

                if((to_write[i] = ((struct problem *) get_problem_variant(workers, i))) == NULL){
                done = 1;
                break;          
                //no more problems
                    }       
        }

            if (done){
                for(i = 0; i < workers; i++){
                    //send sigterm first and then sigcont
                    debug("SIGTERM IS IT BEING HIT");
                    Kill(wrk_arr[i], SIGTERM);
                    Kill(wrk_arr[i], SIGCONT);
                }
            }

    debug("PROBLEMS HAVE BEEN ASSIGNED!");
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
    if (!done){
            for (i = 0; i < workers; i++){
        if(statuses[i] == WORKER_CONTINUED){ //if it's continued, send it the problem
            if (!done){
                Write(fd[i*4+1], to_write[i], to_write[i] -> size); //writing from write end of master 
                sf_send_problem(wrk_arr[i], to_write[i]);
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
    int correct = 0;
    for (i = 0; i < workers; i++){
        if (statuses[i] == WORKER_STOPPED){ //if it's stopped, check result
            struct result *to_read = Malloc(sizeof(struct result));
            debug("WORKER %u", wrk_arr[i]);
            Read(fd[i*4+2], to_read, sizeof(struct result));
            debug("WORKER %u", wrk_arr[i]);
            debug("SOLUTION CHECKING HAPPENED FOR WORKER %u", wrk_arr[i]);
            debug("%lu!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1", to_read -> size);
            if (to_read -> failed == 0){ //if it wasn't canceled 
                if ((to_read -> size - sizeof(struct result)) > 0){
                    to_read = Realloc(to_read, to_read -> size);
                    Read(fd[i*4+2], to_read -> data, to_read -> size - sizeof(struct result));
                }
                 debug("%lu!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!2", to_read -> size);
            }
            int correct_current = post_result(to_read, to_write[i]);
            Free(to_read);
            
            if (correct_current == 0 && correct == 0){ //if the result is correct, cancel other workers
                correct = 1;
                debug("%d++++++++++++++++++++++++++", statuses[0]);
                debug("%d++++++++++++++++++++++++++++", statuses[1]);
                debug("%d++++++++++++++++++++++++++=", i);
                snd_sig(workers, SIGHUP, i);

                // while (!chk_stt(WORKER_STOPPED, workers, workers)){
                //     debug("IS IT STUCK HERE FOREVER ");
                // }
          
            }
            
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
// debug("WORKER1 STATUS:%u\nWORKER2 STATUS:%u", statuses[0], statuses[1]);
    }


//SET IDLE
    if (chk_stt(WORKER_ABORTED, workers, 1)){ //if any of them aborted
        break;
    }

    }


    debug("it's ok, you can try again!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    sf_end(); //end of function

    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
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



