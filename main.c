/*********
 * NAME: Hamed Ghoochanian
 * StudentID: 9712762454
 * UNIX SHELL PROJECT
 *********/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "str.h"

#define BUFFERSIZE 512
#define WHITESPACE " \t\r\n\v\f"

/// built in commands
#define EXIT "exit"
#define PWD "pwd"
#define WHOAMI "whoami"
#define CD "cd"
#define WHATISTHIS "whatisthis"
enum io_type {
    STANDARD,
    OUT_WRITE_FILE,
    OUT_APPEND_FILE,
    IN_FILE
};

char *homeDir;
char currentDir[BUFFERSIZE],
        input[BUFFERSIZE],
        *token;

/***  init  ***/
void getDir() {
    getcwd(currentDir, BUFFERSIZE);
    homeDir = getenv("HOME");
}

/*** prompt the user ***/
void prompt() {
    char *res = replaceStrAtBeg(currentDir, homeDir, "~");
    if (strcmp(res, "~") == 0) res = "~/";
    printf("ZZsh %s@%s >>>", getenv("LOGNAME"), res);
}

/*** read line ***/
char *readFirstToken() {
    fgets(input, BUFFERSIZE, stdin);
    char *enter = strchr(input, '\n');
    if (enter != NULL) enter = '\0';
    token = strtok(input, WHITESPACE);
}

/*** cd ***/
int changeDirectory(char *newDir) {
    if (!newDir)
        return 2;

    char *homeDir_with_slash = malloc(strlen(homeDir) + 2);
    strcpy(homeDir_with_slash, homeDir);
    strcat(homeDir_with_slash, "/");

    char *chdir_path = replaceStrAtBeg(newDir, "~", homeDir);

    if (chdir(chdir_path) != 0) {
        perror("Failed changing directory");
    }
    getcwd(currentDir, BUFFERSIZE);
    return 1;
}

int executeCommand(char **arg_vector, bool is_bg, enum io_type ioType, char *ioFile, bool is_pipe, int fd_input) {
    int status;
    // 0 is for reading, 1 is for writing
    int fileDescriptor_in[2], fileDescriptor_out[2];
    pipe(fileDescriptor_in);
    pipe(fileDescriptor_out);
    int child_pid = fork();

    if (child_pid < 0)
        printf("bad forking\n");

        // in parent(main)
    else if (child_pid != 0) {
        if (fd_input != 0) {
            // close before doing stuff
            close(fileDescriptor_in[0]);
            char buffer[BUFFERSIZE];
            int res_file_read;
            while ((res_file_read = read(fd_input, buffer, BUFFERSIZE)))
                write(fileDescriptor_in[1], buffer, res_file_read);
            close(fileDescriptor_in[1]);
        }
        if (is_pipe)
            close(fileDescriptor_out[1]);

        // wait for child to finish
        if (!is_bg)
            while (wait(&status) != child_pid);

        if (is_pipe)
            return fileDescriptor_out[0];
        else
            return 0;
    }

        // child process
    else {
        if (fd_input != 0) {
            while ((dup2(fileDescriptor_in[0], 0) == -1) && errno == EINTR);
            close(fileDescriptor_in[1]);
            close(fileDescriptor_in[0]);
        }

        if (is_pipe) {
            while ((dup2(fileDescriptor_out[1], 1) == -1) && (errno == EINTR));
            close(fileDescriptor_out[1]);
            close(fileDescriptor_out[0]);
        }

        int file;
        if (ioType == OUT_WRITE_FILE) {
            file = open(ioFile, O_TRUNC | O_WRONLY | O_CREAT, 0666);
            close(1);
        } else if (ioType == OUT_APPEND_FILE) {
            file = open(ioFile, O_APPEND | O_WRONLY | O_CREAT, 0666);
            close(1);
        } else if (ioType == IN_FILE) {
            file = open(ioFile, O_RDONLY);
            close(0);
        }

        if (ioType != STANDARD)
            dup(file);

        if (execvp(arg_vector[0], arg_vector) == -1) {
            if (file)
                close(file);
            printf("bad command\n");
            exit(1);
        } else {
            if (file)
                close(file);
            exit(0);
        }
    }
}

int main(int argc, char **argv) {
    getDir();
    puts(currentDir);
    enum io_type ioType;
    char *arg_vector[256], *ioFile;
    int arg_count = 0;
    bool is_bg = false;

    while (true) {
        //// prompts the user
        prompt();
        int pipe_res = 0;
        arg_count = 0;
        is_bg = false;
        ioFile = "";
        ioType = STANDARD;

        //// read and process tokens(check for pipes and other stuff)
        readFirstToken();
        while (token != NULL) {
            // write to file
            if (strcmp(token, ">") == 0)
                ioType = OUT_WRITE_FILE;
                // append file
            else if (strcmp(token, ">>") == 0)
                ioType = OUT_APPEND_FILE;
                // read from file
            else if (strcmp(token, "<") == 0)
                ioType = IN_FILE;
                // pipe reached
            else if (strcmp(token, "|") == 0) {
                //TODO pipeline
                arg_vector[arg_count] = '\0';
                arg_count++;
                pipe_res = executeCommand(arg_vector, is_bg, ioType, ioFile, true, pipe_res);
                arg_count = 0;
            }
                // execute cmd
            else {
                // set the write file address
                if (ioType != STANDARD) {
                    if (strcmp(ioFile, "") != 0) {
                        printf("ERROR! BAD COMMAND! Invalid redirect implementation");
                        break;
                    } else
                        ioFile = token;
                }
                    // put the token in argument array
                else {
                    arg_vector[arg_count] = malloc(strlen(token) + 1);
                    strcpy(arg_vector[arg_count], token);
                    arg_count = arg_count + 1;
                }
            }
            token = strtok(NULL, WHITESPACE);
        }

        // input is now parsed and is in arg array
        if (arg_count) {
            arg_vector[arg_count] = '\0';
            arg_count++;


            /// check for built-in commands
            if (strcmp(arg_vector[0], EXIT) == 0) {
                printf("BYE BYE\n");
                exit(0);
            } else if (strcmp(arg_vector[0], CD) == 0) {
                changeDirectory(arg_vector[1]);
                continue;
            } else if (strcmp(arg_vector[0], WHOAMI) == 0) {
                printf("You are: %s\n", getenv("LOGNAME"));
                continue;
            } else if (strcmp(arg_vector[0], PWD) == 0) {
                printf("%s\n", currentDir);
                continue;
            } else if (strcmp(arg_vector[0], WHATISTHIS) == 0) {
                printf("Linux shell written by Hamed Ghoochanian\n");
                continue;
            }

            // check for &
            char *amper = strchr(arg_vector[arg_count - 2], '&');
            if (amper != NULL) {
                *amper = '\0';
                is_bg = true;
            }

            if (pipe_res) {
                //TODO pipe it
                pipe_res = executeCommand(arg_vector, is_bg, ioType, ioFile, true, pipe_res);
                char buffer[BUFFERSIZE];
                int file_read;
                while (file_read = read(pipe_res, buffer, BUFFERSIZE))
                    write(1, buffer, file_read);
            } else {
                //TODO execute it
                executeCommand(arg_vector, is_bg, ioType, ioFile, false, false);
            }
        }
    }
}