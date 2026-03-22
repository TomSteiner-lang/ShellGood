#ifndef UTIL_GOOD
#define UTIL_GOOD

#include "global/generic_defs.h"

int getJobOfPid(pid_t pid, ArrayList_Job *jobs);
int findJobByPgid(pid_t pgid, ArrayList_Job *jobs);
int stringToNatNum(char *str);
void printUI(char *cwd);

#endif