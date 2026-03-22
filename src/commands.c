#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "../headers/global/generic_defs.h"
#include "../headers/utils.h"
#include "../headers/global/settings.h"



static inline void cd(char *args[]);
static inline void jobs();
static void fg(char *args[]);
static void bg(char *args[]);


static Job *foreground_job_address;
static ArrayList_Job *jobs_address;
static char (*cwd)[MAX_DIR];

char *commands[] = {"cd", "jobs", "fg", "bg"};
void execGood(Process process) {
    char *command = process.args[0];
    
    //big switch statement
    if (!strcmp(command, "cd")) {
        cd(process.args);
    }

    if (!strcmp(command, "jobs")) {
        jobs();
    }

    if (!strcmp(command, "fg")) {
        fg(process.args);
    }
    
    if (!strcmp(command, "bg")) {
        bg(process.args);
    }
}

void init_shell_commands(char (*dir)[] ,ArrayList_Job *jobs, Job *foreground_job) {
    cwd = dir;
    jobs_address = jobs;
    foreground_job_address = foreground_job;
}

int isShell(Process process) {
    char *command = process.args[0];
    int n = sizeof(commands)/sizeof(commands[0]);

    for (int i = 0; i < n; i++) {
        if (!strcmp(commands[i], command)) {
            return 1;
        }
    }
    return 0;
}


static inline void jobs() {

    if (jobs_address->length == 0) {
        printf("no active jobs\n");
        fflush(stdout);
        return;
    }
    for (int i = 0; i < jobs_address->length; i++) {
        Job job = ArrayListGet_Job(jobs_address, i);
        char *running = "(paused)";
        if (!job.stopped) {
            running = "(running)";
        }
        printf("pid[%d]:  %s    %s\n",job.pgid ,job.tostring, running);

    }
}

static inline void cd(char *args[]) {
    //cd: args[1] should be the destination directory
    chdir(args[1]);
    getcwd(*cwd, MAX_DIR);
}

static void bg(char *args[]) {
    char *pgidstr = args[1];

    if (pgidstr == NULL) {
        //continue last stopped job in the background
        for (int i = jobs_address->length -1; i >= 0; i--) {
            Job job = ArrayListGet_Job(jobs_address, i);
            if (job.stopped == 1) {
                if (kill(-job.pgid, SIGCONT) == -1) {
                    printf("pid[%d]     couldnt continue job in the background", job.pgid);
                    fflush(stdout);
                }
                else {
                    jobs_address->array[i].stopped = 0;
                    printf("%s\n", job.tostring);
                }
                return;
            }
        }
        printf("no stopped jobs\n");
        return;
    }

    //continue selected job in the background
    int pgid = stringToNatNum(pgidstr);
    int index = findJobByPgid(pgid, jobs_address);

    if (index == -1) {
        printf("job with pid[%d] not found\n", pgid);
        fflush(stdout);
        return;
    }
    else {
        if (kill(-pgid, SIGCONT) == -1) {
            printf("pid[%d]     couldnt continue job in the background\n", pgid);
            fflush(stdout);
        } 
        else {
            jobs_address->array[index].stopped = 0;
            printf("%s\n", jobs_address->array[index].tostring);
        }
        
    }

}

static void fg(char *args[]) {

    char *pgidstr = args[1];

    if (pgidstr == NULL) {
        //continue last stopped job in the foreground
        for (int i = jobs_address->length -1; i >= 0; i--) {
            Job job = ArrayListGet_Job(jobs_address, i);
            if (job.stopped == 1) {
                if (kill(-job.pgid, SIGCONT) == -1) {
                    printf("pid[%d]     couldnt bring job to foreground", job.pgid);
                    fflush(stdout);
                }
                else {
                    jobs_address->array[i].stopped = 0;
                    *foreground_job_address = job;
                    printf("%s\n", job.tostring);
                }
                return;
            }
        }
        printf("no stopped jobs\n");
        return;
    }

    //continue/move selected job in the foreground
    int pgid = stringToNatNum(pgidstr);
    int index = findJobByPgid(pgid, jobs_address);

    if (index == -1) {
        printf("job with pid[%d] not found\n", pgid);
        fflush(stdout);
        return;
    }
    else {
        if (kill(-pgid, SIGCONT) == -1) {
            printf("pid[%d]     couldnt bring job to foreground\n", pgid);
            fflush(stdout);
        } 
        else {
            jobs_address->array[index].stopped = 0;
            *foreground_job_address = ArrayListGet_Job(jobs_address, index);
            printf("%s\n", jobs_address->array[index].tostring);
        }
        
    }


}

