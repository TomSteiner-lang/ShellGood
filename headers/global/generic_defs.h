#ifndef GEN_GOOD
#define GEN_GOOD

    #include <fcntl.h>
    #include <signal.h>
    #include "arrayList.h"
    #include "settings.h"


    
    typedef struct {
        
        int background;
        int processes;
        int invalid;
    } JobInfo;

    typedef struct {
    
        char *input_file;
        char *output_file;
        char *error_file;
        char *args[MAX_ARGS];

    } Process;

    DEF_ARRAYLIST(pid_t);

    typedef struct {
        ArrayList_pid_t process_pids;
        pid_t pgid;
        int processes;
        char *tostring;
        int stopped;
    } Job;


    DEF_ARRAYLIST(Job);

    typedef struct {
        volatile sig_atomic_t siginted;
        volatile sig_atomic_t sigtstped;
        volatile sig_atomic_t exitShell;
    }   SignalFlags;


#endif