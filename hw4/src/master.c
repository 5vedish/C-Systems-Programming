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
int statuses[MAX_WORKERS]; //flags for the statuses
int fd[4*MAX_WORKERS]; //file descriptors
int temp_fd[2]; //temp file descriptors

//Signal Handlers
void sigchild_handler(int sig){ //SIGCHLD handler
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG | WUNTRACED | WORKER_CONTINUED)) != 0){
        int pid_index = fnd_pid(pid);

        
        sf_change_state(pid, statuses[pid_index], )



    }
}

void sigpipe_handler(int sig){ //SIGPIPE handler, to preserve master

}

//Method Declarations
void ass_prb(int workers);
int chk_stt(sig_atomic_t state, int workers, int num);
int fnd_pid(pid_t pid);


/*
 * master
 * (See polya.h for specification.)
 */
int master(int workers) {

    sf_start(); //beginning of function

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

        if((wrk_arr[i] = Fork()) == 0){
            statuses[i] = WORKER_STARTED;
            dup2(fd[i+2*workers], STDIN_FILENO); 
            dup2(fd[i+3*workers], STDOUT_FILENO); //redirecting input and output in worker processes
            execl("bin/polya_worker", "bin/polya_worker"); //execute the workers

            Close(fd[i]);
            Close(fd[i+workers]); //close fds for master in worker
        } else{
            Close(fd[i+2*workers]);
            Close(fd[i+3*workers]); //close fds for worker in master
        }
    }

    //main loop
    while (1){

    if (chk_stt(WORKER_IDLE, workers, workers)){ //if they are all stopped

        ass_prb(workers); //assigning the problems

        for (i = 0; i < workers; i++){
            Kill(wrk_arr[i], SIGCONT); //resume the workers
        }

        for (i = 0; i < workers; i++){
        Write(fd[i+workers], prb_arr[i], prb_arr[i] -> size); //writing from write end of master
        }   

        for (i = 0; i < workers; i++){
        statuses[i] = WORKER_RUNNING; //set workers to running, as they are solving problems
        }


    }

    
//free the problem that you malloced
    











 
    }



    sf_end(); //end of function

    // TO BE IMPLEMENTED
    return EXIT_FAILURE;
}

void ass_prb(int workers){

    struct problem *to_write = Malloc(sizeof(struct problem)); //space for a problem

    int i; //counter
    for (i = 0; i < workers; i++){
            if((to_write = ((struct problem *) get_problem_variant(i, workers))) == NULL){
                //no more problems
                return;
            }
            prb_arr[i] = to_write; //saving the problem inside the array
        }

}

//returns 0 if num of statuses found isn't expected and 1 if it is
int chk_stt(sig_atomic_t state, int workers, int num){
    int i, j = 0; //counters
    for (i = 0; i < workers; i++){
        if (statuses[i] = state){
            j++;
        }
    }

    return (j == num) ? 1: 0;
}

int fnd_pid(pid_t pid){ //search for and return index of the pid
    int i; //counter
    for (i = 0; i < MAX_WORKERS; i++){
        if(pid = wrk_arr[i]){
            return i;
        }
    }

    return -1;
}




