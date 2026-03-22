#ifndef COM_GOOD
#define COM_GOOD

#include "global/settings.h"
#include "global/generic_defs.h"



void execGood(Process process);
void init_shell_commands(char (*dir)[] ,ArrayList_Job *jobs, Job *foreground_job);
int isShell(Process process);





#endif