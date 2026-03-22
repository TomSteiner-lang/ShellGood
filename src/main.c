#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "../headers/global/settings.h"
#include "../headers/global/generic_defs.h"
#include "../headers/global/arrayList.h"
#include "../headers/signals.h"
#include "../headers/parser.h"
#include "../headers/commands.h"
#include "../headers/utils.h"


void cleanupAndExit();
void abortchild(int pipe1, int pipe2, int pipe3, int pipe4, int pipe5, int fd1, int fd2);


int sigchld_pipe[2];
char input[MAX_IN] = "";
char *args[MAX_ARGS];
char backgroundString[256]; 
Process processes[MAX_PROCESSES];
pid_t foreground_pgid;
pid_t prev_foreground;
ArrayList_Job jobs;
char cwd[MAX_DIR];
Job foreground_job = {0};
SignalFlags sigflags = {0};




int main() {

    //startup
    foreground_pgid = -1;
    init_shell_signals(&sigchld_pipe, &jobs, &sigflags);
    init_shell_commands(&cwd, &jobs, &foreground_job);
    jobs = createArrayList(Job);

    getcwd(cwd, sizeof(cwd));

    pid_t shell_pid = getpid();


    //main loop
    while (1) {

        foreground_pgid = -1;

        pid_t reaped_process;
        //look for finished pids from the signal handler pipe
        while ((read(sigchld_pipe[0], &reaped_process, sizeof(pid_t))) > 0) {
            
            int index = getJobOfPid(reaped_process, &jobs);
            
        
            if (index != -1) {
                jobs.array[index].processes--;
                int pid_index = -1;
                
                //remove finished process from job
                for (int i = 0; i <= jobs.array[index].processes; i++) {
                    if (ArrayListGet_pid_t(&jobs.array[index].process_pids, i) == reaped_process) {
                        pid_index = i;
                    }
                }
                if (pid_index != -1) {
                    ArrayListRemove_pid_t(pid_index, &jobs.array[index].process_pids);
                }

                //cleanup fully finished jobs
                if (jobs.array[index].processes == 0) {
                    
                    pid_t pgid = ArrayListGet_Job(&jobs, index).pgid;
                    ArrayList_pid_t arraylist = ArrayListGet_Job(&jobs, index).process_pids;

                    free(ArrayListGet_Job(&jobs, index).tostring);
                    ArrayListKill_pid_t(&arraylist);
                    ArrayListRemove_Job(index, &jobs);
                    printf("\n[%d] job done\n", pgid); 
                    fflush(stdout);
                }
            }
        }


        
        if (sigflags.exitShell == 1) {
            cleanupAndExit();
        }




        //UI
        printUI(cwd);
        fflush(stdout);


        //wipe previous args

        memset(processes, 0, sizeof(processes));
        memset(input, 0, MAX_IN);

        //get input        
        if (!(fgets(input, 1024, stdin))) {
            //exit on interrupted by signal (intended signals dont interrupt this)
            cleanupAndExit();
        }
        input[strcspn(input, "\n")] = 0;

        if (!strcmp(input, "exit")) {
            cleanupAndExit();
        }

        if (strlen(input) == 0) {
            continue;
        }
        

        //parse input        
        JobInfo jobinfo = parseInputToProcesses(input, &processes, &args);
        int background = jobinfo.background;
        int job_size = jobinfo.processes;
        
        //invalid jobs
        if (jobinfo.invalid == 1) {
            printf("Shell built-in commands cannot be in the same pipeline as normal commands\n");
            fflush(stdout);
            continue;
        }
        else if (jobinfo.invalid == 2) {
            printf("Invalid job\n");
            fflush(stdout);
            continue;
        }

        
        

        int syncpipe[2];
        if (pipe(syncpipe) == -1) {
            perror("sync pipe failed to open\n");
            continue;
        }

        //block sigchld until all processes are initiallized
        toggle_sigchld(0);

        //run all processes
        int pipeline[2] = {-1, -1};
        int prev_pipe = -1;
        pid_t process_group = -1;
        int first_process = 1;
        int shellonly = 0;
        int fail = 0;

        sigflags.siginted = 0;
        sigflags.sigtstped = 0;

        
        if (sigflags.exitShell == 1) {
            cleanupAndExit();
        }


        for (int i = 0; i < job_size; i++) {

            
            if (isShell(processes[0])) {
                shellonly = 1;
            }

            //run only shell commands
            if (shellonly) {
                if (isShell(processes[i])){
                    execGood(processes[i]);
                }
                continue;
            }
            
            //run only normal commands
            else {

                //prepare pipes for forking
                if (processes[i].input_file != NULL && strcmp(processes[i].input_file, "PREV")) {
                    if (prev_pipe != -1) {
                        //this should never happen but close pipe if it does
                        close(prev_pipe);
                    }
                }
                if (processes[i].output_file != NULL && !strcmp(processes[i].output_file, "NEXT")){
                    if (pipe(pipeline) == -1) {
                        //error (letting everything still run the process will just use stdin/out)
                    }
                }
                pid_t pid = fork();
                if (pid < 0) {
                    // error
                    perror("fork");
                    fail = 1;
                    break;
                }
                //child process
                else if (pid == 0) {
                    init_child_signals();

                    close(syncpipe[0]);
                    

                    // join/set process group
                    if(first_process == 1) {
                        if (setpgid(0, 0) == -1) {
                            perror("setpgid");
                            abortchild(pipeline[0], pipeline[1], prev_pipe, syncpipe[0], syncpipe[1], -1, -1);
                            exit(1);
                        }
                    }
                    else {
                        if (setpgid(0, process_group) == -1) {
                            perror("setpgid");
                            abortchild(pipeline[0], pipeline[1], prev_pipe, syncpipe[0], syncpipe[1], -1, -1);
                            exit(1);
                        }
                    }

                    //wire IO
                    if (pipeline[0] != -1) {
                        close(pipeline[0]);
                        pipeline[0] = -1;
                    }
                    if (pipeline[1] != -1) {

                        dup2(pipeline[1], STDOUT_FILENO);
                        close(pipeline[1]);
                        pipeline[1] = -1;
                    }
                    if (prev_pipe != -1) {

                        dup2(prev_pipe, STDIN_FILENO);
                        close(prev_pipe);
                        prev_pipe = -1;
                    }
                    int fdin = -1;
                    int fdout = -1;
                    //file IO
                    if (processes[i].output_file != NULL && strcmp(processes[i].output_file, "NEXT")) {
                        
                        
                        if ((fdout = open(processes[i].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
                            
                            abortchild(pipeline[0], pipeline[1], prev_pipe, syncpipe[0], syncpipe[1], fdin, fdout);
                            exit(1);
                        }
                        dup2(fdout, STDOUT_FILENO);
                        close(fdout);
                        fdout = -1;
                    }
                    if (processes[i].input_file != NULL && strcmp(processes[i].input_file, "PREV")) {

                        if ((fdin = open(processes[i].input_file, O_RDONLY)) == -1) {
                            abortchild(pipeline[0], pipeline[1], prev_pipe, syncpipe[0], syncpipe[1], fdin, fdout);

                            exit(1);
                        }
                        dup2(fdin, STDIN_FILENO);
                        close(fdin);
                        fdin = -1;
                    }

                    //tell shell process is ready and execute
                    close(syncpipe[1]);
                    execvp(processes[i].args[0], processes[i].args);
                    abortchild(pipeline[0], pipeline[1], prev_pipe, syncpipe[0], syncpipe[1], fdin, fdout);

                    exit(1);
                }
                //parent
                else{

                    //set pgid and log the job
                    if (first_process == 1) {
                        process_group = pid;

                        Job job = {0};
                        job.pgid = process_group;
                        job.processes = 1;
                        char *job_args = malloc(sizeof(input));
                        if (!job_args) {
                            perror("malloc");
                            fail = 1;
                            ArrayListAdd_Job(job, &jobs);

                            break;
                        }
                        memset(job_args, 0, sizeof(input));
                        int i = 0;
                        while (args[i] != NULL) {
                            strcat(job_args, args[i]);
                            strcat(job_args, " ");
                        i++;
                        }
                        job.tostring = job_args;


                        job.process_pids = NewArrayList_pid_t();

                        ArrayListAdd_pid_t(pid, &job.process_pids);

                        
                        if (background == 0) {
                            foreground_job = job;
                            
                        }
                        ArrayListAdd_Job(job, &jobs);


                        
                    }
                    else {
                        //add process to job
                        if (background == 1) {                
                            int index = findJobByPgid(process_group, &jobs);
                            jobs.array[index].processes++; 
                            ArrayListAdd_pid_t(pid, &jobs.array[index].process_pids);
                        }
                        else {
                            foreground_job.processes++;
                            fflush(stdout);
                            ArrayListAdd_pid_t(pid, &foreground_job.process_pids);
                        }

                    }
                    first_process = 0;


                    //close pipes
                    if (pipeline[1] != -1) {
                        close(pipeline[1]);
                        pipeline[1] = -1;
                    }
                    if (prev_pipe != -1) {
                        close(prev_pipe);
                        prev_pipe = -1;
                    }
                    if (pipeline[0] != -1) {
                        prev_pipe = pipeline[0];
                        pipeline[0] = -1;
                    }


                }
            }
        }


        //catch pipe/fork fails
        if (fail == 1) {
            //close pipes
            if (prev_pipe != -1) {
                close(prev_pipe);
            }
            if (pipeline[0] != -1) {
                close(pipeline[0]);
            }
            if (pipeline[1] != -1) {
                close(pipeline[1]);
            }

            //unlog the job if it was logged and interrupt the processes
            if (first_process != 1) {
                int failedJobIndex = jobs.length -1;

               kill(-jobs.array[failedJobIndex].pgid, SIGINT);

                ArrayListKill_pid_t(&jobs.array[failedJobIndex].process_pids);
                free(jobs.array[failedJobIndex].tostring);
                ArrayListRemove_Job(failedJobIndex, &jobs);
            }

            //unblock sigchld, the interrupted processes queue sigchld and will be reaped in the next loop
            toggle_sigchld(1);
            continue;
            
        }


        if (foreground_job.pgid != 0) {
            tcsetpgrp(STDIN_FILENO, foreground_job.pgid);
        }

        close(syncpipe[1]);
        
        //only start reaping foreground group once they all set their pgid
        char buffer[1];
        read(syncpipe[0],&buffer, 1);
        close(syncpipe[0]);
        

        if (background == 0) {
            int exit = -1;
            foreground_pgid = foreground_job.pgid;
            tcsetpgrp(STDIN_FILENO, foreground_pgid);


            //check for interrupts before foreground group was given terminal
            if (sigflags.siginted) {         
                kill(-(foreground_pgid), SIGINT);
                int index = getJobOfPid(foreground_pgid, &jobs);
                ArrayList_pid_t arraylist = ArrayListGet_Job(&jobs, index).process_pids;

                free(ArrayListGet_Job(&jobs, index).tostring);
                ArrayListKill_pid_t(&arraylist);
                ArrayListRemove_Job(index, &jobs);

                sigflags.siginted = 0;
                printf("Interrupted Job\n<Shell Good> ");   
            }

            if (sigflags.sigtstped) {         
                kill(-(foreground_pgid), SIGTSTP);
                sigflags.sigtstped = 0;
            }


            int status = 0;
            int stopped = 0;
            //reap the foreground group
            while (waitpid(-foreground_pgid, &status, WUNTRACED) > 0) {
                if (WIFEXITED(status)) {
                    if (WEXITSTATUS(status) == 1) {

                    }
                }
                else if (WIFSTOPPED(status)) {
                    stopped = 1;
                    break;
                }
            }
            exit = errno;
            if (stopped == 1) {
                //set job to stopped
                printf("\npid[%d]   process stopped: %s\n", foreground_job.pgid, foreground_job.tostring);
                int index = findJobByPgid(foreground_job.pgid, &jobs);
                jobs.array[index].stopped = 1;
                memset(&foreground_job, 0, sizeof(Job));
            }
            else if (0 != foreground_job.pgid) {
                //cleanup foreground group

                int index = findJobByPgid(foreground_job.pgid, &jobs);
                if (index != -1) {
                    Job job = ArrayListRemove_Job(index, &jobs);
                    free(job.tostring);
                    ArrayListKill_pid_t(&job.process_pids);
                }
            }
            if (exit == EINTR) {
                cleanupAndExit();
            }
        }

        tcsetpgrp(STDIN_FILENO, shell_pid);

        toggle_sigchld(1);

        
        if (sigflags.exitShell == 1) {
            cleanupAndExit();
        }

        foreground_pgid = -1;
        background = 0;


    
    }    
}

void cleanupAndExit() {
//kill all jobs and cleanup memory
    for (int i = 0; i < jobs.length; i++) {
        pid_t pgid = jobs.array[i].pgid;
        kill(-pgid, SIGTERM);
    }

    
    while (waitpid(-1, NULL, 0) > 0);

    
    for (int i = 0; i < jobs.length; i++) {

        ArrayListKill_pid_t(&jobs.array[i].process_pids);
        free(jobs.array[i].tostring);
    }
    ArrayListKill_Job(&jobs);


    printf("Goodbye...\n");
    fflush(stdout);
    exit(0);

}

void abortchild(int pipe1, int pipe2, int pipe3, int pipe4, int pipe5, int fd1, int fd2) {
//disconnect all fds so other children wont stall forever
    int fds[] = {pipe1, pipe2, pipe3, pipe4, pipe5, fd1, fd2};
    for (int i = 0; i < (sizeof(fds)/sizeof(fds[0])); i++) {
        if (fds[i] != -1) {
            close(fds[i]);
        }
    }
}




