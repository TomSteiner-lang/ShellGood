#include "../headers/global/generic_defs.h"

int findJobByPgid(pid_t pgid, ArrayList_Job *jobs) {
//returns the index of the job with the given pgid
    
    for (int i = 0; i < jobs->length; i++) {
        if (jobs->array[i].pgid == pgid) {
            return i;
        }
    }
    return -1;
}

int getJobOfPid(pid_t pid, ArrayList_Job *jobs) {
//returns the index of the first job that contains the given pid

    for (int i = 0; i < jobs->length; i++) {
        Job job = ArrayListGet_Job(jobs, i);
        for (int j = 0; j < job.processes; j++) {
            if (ArrayListGet_pid_t(&job.process_pids, j) == pid) {
                return i;
            }
        }
    }
    return -1;

}

int stringToNatNum(char *str) {
//converts a string to a natural number

    char current = *str;
    int natural = 0;
    while (current != 0) {
        int digit;
        
        if (current >= '0' && current <= '9') {
            digit = current - '0';
        } 
        else {
            return -1;
        }
        natural *= 10;
        natural += digit;
        str++;
        current = *str;

    }
    return natural;
}


void printUI(char cwd[MAX_DIR]) {
    printf("%s\n<Shell Good> ", cwd);
}