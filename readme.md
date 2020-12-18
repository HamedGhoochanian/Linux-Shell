# ZZ Shell
A simple linux shell implemented in C, supports pipes and system commands

## Build
* Run `make` or `make build` to compile the code

## Starting the shell
* run `./shell` to start it in interactive mode
* run `./shell <filename>` to run the commands within a batch file

## Features
1. pipelining 2 or more commands
2. running commands from a batch file
3. saves the input commands in `history.log` and prints them upon entering `history` command
4. terminates programs with `CTRL+C`
5. Exit the shell with `CTRL+D`
6. changed present directory using cd commands
7. print the current directory using `pwd` command
8. supports output redirevtion to file using `<`,`<<` and `>`
9. prints the current user using `whoami`
10. exits the shell using `exit`
11. supports system commands(ls, grep, cat ...)

## examples
* run `cat main.c | head -7 | tail -5` to print line 3 to 7 as a demonstration of pipes
* run `./shell sample.sh` to execute the batch file(it's a simple hello world)

## info
* commands are read from `stdin` and handled using `readtoken()` function
* if the given batch file doesn't exist proper error message will be printed
* every command entered in interactive mode is appended to `history.log` file
* commands are executed using `execvp` system call
* `CTRL+c` is handled using `signal` function and CTRLC signal handled
* if entered command is `NULL` or `CTRL + D` the shell will close

