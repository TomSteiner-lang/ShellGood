#ifndef PROCGOOD_H
#define PROCGOOD_H


    #include "global/settings.h"
    #include "global/generic_defs.h"



    JobInfo parseInputToProcesses(char input[], Process (*processes)[MAX_PROCESSES], char *(*args)[MAX_ARGS]);


#endif