# ShellGood - A Mini Shell in C
## Overview
ShellGood is a Unix-like shell written in C with support for pipelines, redirection, and full job control.
The shell implements POSIX process control primitives including process groups, signals, and terminal management. 
## Features
- Executes programs using fork + execvp
- Pipelines with multiple processes (|)
- Input/Output redirection (<,>)
- Background execution (&)
- Job control (jobs, fg, bg)
- Terminal control using tcsetpgrp
- Signal handling:
  - SIGCHLD for async child reaping
  - Proper handling of SIGINT and SIGTSTP
## Architecture
The shell is structured into separate components:
- Parser: tokenizes inputs and builds process structures
- Commands: built-in shell commands
- Signals: signal handlers
- Execution: pipeline creation, process groups, job tracking and launching
## Challenges/Highlights
- Race condition with SIGCHLD:
  - Ensured correct behavior by blocking signals during job creation and foreground job execution
  - Used a self-pipe to communicate between signal handler and main loop

- Foreground signal race handling:
  - Ensured signals like SIGINT/SIGTSTP are not lost when delivered before the shell transfers terminal control

- Correct job control implementation:
  - Used process groups so pipelines behave as single jobs and receive signals together

- Terminal ownership:
  - Transferred terminal control with tcsetpgrp so Ctrl+C/Z are delivered directly to the foreground job

- Pipeline execution:
  - Built a loop that correctly wires pipes and assigns a shared process group to all processes in a pipeline

## Technical Details
- Uses `setpgid` to group processes into jobs and ensure correct signal delivery
- Transfers terminal control using `tcsetpgrp` for proper foreground execution
- Implements a self-pipe mechanism for safe communication from signal handlers
- Avoids race conditions by blocking `SIGCHLD` during critical sections
- Tracks individual process IDs within jobs for precise lifecycle management
- Uses `waitpid` with `WUNTRACED` to correctly detect stopped processes

## Design Decisions
- Separation of built-in commands:
  - Built-ins are executed in the shell process and cannot be mixed with normal commands to avoid subshell semantics
- Parser-level validation:
  - Invalid jobs (e.g. mixing shell built-ins in pipelines) are rejected during parsing

## Building and Running
to build:
make

to run:
./ShellGood

## Notes
- This project was made as a learning exercise in systems programming and not as a replacement for production shells like bash
- Parsing is intentionally minimal (no quoting or advanced shell syntax)
- Focus is on correct process, signals, and job control behavior

