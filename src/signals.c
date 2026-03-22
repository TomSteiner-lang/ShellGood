
#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include "../headers/global/generic_defs.h"

static int (*sigchld_pipe_address)[2];
sigset_t mask_sigchld;
struct sigaction base = {0};
static ArrayList_Job *jobs_address;
volatile sig_atomic_t *siginted_address;
volatile sig_atomic_t *sigtstped_address;
volatile sig_atomic_t *exitShell_address;

void handle_sigtstp(int sig) {
//raises flag in case ctrl-z was hit while processes were still starting
    *sigtstped_address = 1;
    write(STDOUT_FILENO, "\n<Shell Good> ", sizeof("\n<Shell Good>"));

}


void handle_sigint(int sig) {
//raises flag i case ctrl-c was hit while processes were still starting
    *siginted_address = 1;
    write(STDOUT_FILENO, "\n<Shell Good> ", sizeof("\n<Shell Good>"));
}

void handle_sigchld(int sig) {
//reaps all stopped processes and writes pids into a pipe so main loop can clean up memory
    int status;
    pid_t pid;
    while ((pid = waitpid(-1,&status,WNOHANG)) > 0) {

        write((*sigchld_pipe_address)[1], &pid, sizeof(pid_t));
           
    }
}

void handle_shell_death(int sig) {
    *exitShell_address = 1;
}

void toggle_sigchld(int on) {
    if (on == 0) {
        sigprocmask(SIG_BLOCK, &mask_sigchld, NULL);
    }
    else {
        sigprocmask(SIG_UNBLOCK, &mask_sigchld, NULL);
    }
}


int init_shell_signals(int (*sigchld_pipe)[2], ArrayList_Job *jobs, SignalFlags *flags) {
    
    //initallize variables
    jobs_address = jobs;
    sigchld_pipe_address = sigchld_pipe;
    siginted_address = &flags->siginted;
    sigtstped_address = &flags->sigtstped;
    exitShell_address = &flags->exitShell;
    
    
    //initiallize blank sigaction
    base.sa_handler = SIG_DFL;

    
    //mask for blocking sigchld
    sigemptyset(&mask_sigchld);
    sigaddset(&mask_sigchld, SIGCHLD);
    
    //sigint
    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    //sigchld
    struct sigaction sa_chld;
    sa_chld.sa_handler = handle_sigchld;
    sa_chld.sa_flags = SA_RESTART;
    sigemptyset(&sa_chld.sa_mask);
    sigaction(SIGCHLD, &sa_chld, NULL);

    //sigtstp
    struct sigaction sa_tstp;
    sa_tstp.sa_handler = handle_sigtstp;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);


    //shell termination signals
    struct sigaction sa_killshell;
    sa_killshell.sa_handler = handle_shell_death;
    sa_killshell.sa_flags = 0;
    sigemptyset(&sa_killshell.sa_mask);
    sigaction(SIGTERM, &sa_killshell, NULL);
    sigaction(SIGHUP, &sa_killshell, NULL);
    sigaction(SIGQUIT, &sa_killshell, NULL);


    //open sigchld pipe
    pipe(*sigchld_pipe_address);
    fcntl((*sigchld_pipe_address)[0], F_SETFL, O_NONBLOCK);
    fcntl((*sigchld_pipe_address)[1], F_SETFL, O_NONBLOCK);
    
    //ignore so shell can give terminal access
    signal(SIGPIPE,SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    // signal(SIGTSTP, SIG_IGN);

    return 0;
}

void init_child_signals() {
//reset signals to default    

    sigaction(SIGINT, &base, NULL);
    sigaction(SIGCHLD, &base, NULL);
    sigaction(SIGPIPE, &base, NULL);
    sigaction(SIGTTOU, &base, NULL);
    sigaction(SIGTTIN, &base, NULL);
    sigaction(SIGTSTP, &base, NULL);
    sigaction(SIGTERM, &base, NULL);
    sigaction(SIGHUP, &base, NULL);
    sigaction(SIGQUIT, &base, NULL);
    toggle_sigchld(1);



}