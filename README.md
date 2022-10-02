# Bash-Shell

This program is a small shell environment written in C.  Its main commands are cd, status, and exit.  It can also execute any other
normal linux bash commands.  Input and output files for redirection may be used.  Commands may be executed as foreground or background processes.  Signals can be received by the shell.

#####
Compile Instructions
#####

1. Type this command into the Terminal Shell

gcc -std=c99 -o smallsh smallsh.c

2. Then run this command to start the environment:

./smallsh
