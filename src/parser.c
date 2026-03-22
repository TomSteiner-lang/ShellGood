
#include <string.h>
#include "../headers/global/settings.h"
#include "../headers/commands.h"



static char *(*args)[MAX_ARGS];
char next[] = "NEXT";
char prev[] = "PREV";


static inline void tokenize_args(char input[]) {
//converts input string into a null terminated list of arguments
    
    memset((*args), 0, sizeof((*args)));

    (*args)[0] = strtok(input, " ");
        
    int i = 1;
    if ((*args)[0] != NULL){
           
        while (i < MAX_ARGS) {
            (*args)[i] = strtok(NULL, " ");
            if ((*args)[i] == NULL) {
                break;
            }
            i++;
        }
    }
    (*args)[i] = NULL;

}

static inline JobInfo parseArgsToProcesses(Process (*processes)[MAX_PROCESSES]) {
/**fills the processes array with the parsed arguments
 * 
 * ">" and "<" set the input/output file to the next argument
 * "|" starts a new process
 * "&" marks the job as a background job
 * 
 * returns a struct with the amount of processes in the job 
 * and whether its in the background or not
 **/ 

    memset(processes, 0, sizeof(*processes) );
    
    int background = 0;
    int process = 0;
    int index = 0;
    int i = 0;

    JobInfo ret = {0};
    while ((*args)[i] != NULL) {

            //boundary check
            if (i >= MAX_ARGS || process > MAX_PROCESSES -1) {
                (*processes)[0].args[0] = NULL;
                    break;
            }

            //set output file to the next argument
            else if (!strcmp((*args)[i], ">")){
                i++;
                if((*args)[i] == NULL) {
                    (*processes)[0].args[0] = NULL;
                    break;
                }
                (*processes)[process].output_file = (*args)[i];
            }

            //set input file to the next argument
            else if (!strcmp((*args)[i], "<")){
                i++;
                if((*args)[i] == NULL) {
                    (*processes)[0].args[0] = NULL;
                    break;
                }
                (*processes)[process].input_file = (*args)[i];
            }

            //null terminate this process and start new process
            else if (!strcmp((*args)[i], "|")) {
                
                if((*args)[i+1] == NULL || process >= MAX_PROCESSES - 1) {
                    (*processes)[0].args[0] = NULL;
                    break;
                }
                (*processes)[process].args[index] = NULL;
                index = 0;
                process++;
                (*processes)[process].error_file = NULL;
                (*processes)[process].input_file = NULL;
                (*processes)[process].output_file = NULL;
            }

            else if (!strcmp((*args)[i], "&")) {
                background = 1;
            }

            //add arg into process
            else {
                (*processes)[process].args[index] = (*args)[i];
                index++;
            }
            i++;
        }
        (*processes)[process].args[index] = NULL;
        process++;
        (*processes)[process].args[0] = NULL;


        //null the first process of all irrelevant args
        for (int j = process + 1; j < MAX_PROCESSES; j++) {
            (*processes)[j].args[0] = NULL;
        }


        //return background = -1 if the process is invalid
        ret.background = background;
        ret.processes = process;
        if ((*processes)[0].args[0] == NULL) {
            ret.background = -1;
        }
        return ret;
        
}


static inline void checkJobValidity(JobInfo *jobinfo, Process (*processes)[MAX_PROCESSES]) {
//a job is invalid if it involves both built in commands and normal commands

    int shell = -1;
    int notshell = 0;
    for (int i = 0; i < (*jobinfo).processes; i++) {
        if (isShell((*processes)[i])) {
            shell = 1;
        }
        else {
            notshell = 1;
        }
    }
    if (shell == notshell) {
        (*jobinfo).invalid = 1;
    }


}


void wirepipes(JobInfo jobinfo, Process (*processes)[MAX_PROCESSES]) {
//marks adjacent processes that are supposed to pipe into each other

    for (int i = 0; i < jobinfo.processes -1; i++) {
        if((*processes)[i].output_file == NULL && (*processes)[i+1].input_file == NULL) {
            
            if (!isShell((*processes)[i]) && !isShell((*processes)[i+1])) {
                (*processes)[i].output_file = next;
                (*processes)[i+1].input_file = prev;
            }
        }
    }
}

JobInfo parseInputToProcesses(char input[], Process (*processes)[MAX_PROCESSES], char *(*args_address)[MAX_ARGS]) {
//sets the processes array and returns a stuct with all the relevant information about the job

    args = args_address;
    tokenize_args(input);
    JobInfo jobinfo = parseArgsToProcesses(processes);
    checkJobValidity(&jobinfo, processes);
    wirepipes(jobinfo, processes);
    if (jobinfo.background == -1) {
        jobinfo.invalid = 2;
    }
    return jobinfo;
}



