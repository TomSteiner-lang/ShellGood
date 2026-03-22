#include <unistd.h>
#include "global/generic_defs.h"
#ifndef SIGGOOD_H
#define SIGGOOD_H




void handle_sigint(int sig);
void handle_sigchld(int sig);
void toggle_sigchld(int on);
int init_shell_signals(int (*sigchld_pipe)[2], ArrayList_Job *jobs, SignalFlags *flags);
void init_child_signals();

#endif