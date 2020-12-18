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
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "str.h"

#define BUFFERSIZE 512
#define WHITESPACE " \t\r\n\v\f"
// log file address
#define LOG "/home/hamed/workspace/shell-new/history.log"

/// built in commands
#define EXIT "exit"
#define PWD "pwd"
#define WHOAMI "whoami"
#define CD "cd"
#define WHATISTHIS "whatisthis"
#define HISTORY "history"


char *homeDir;
char currentDir[BUFFERSIZE],
        input[BUFFERSIZE],
        *token;

FILE *historyFile;

/***  get home and current directories  ***/
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

/*** read line and print it to file ***/
void readTokens() {
    historyFile = fopen(LOG, "a");
    int l = fgets(input, BUFFERSIZE, stdin);
    if (l == NULL) {
        printf("bye bye");
        exit(0);
    }
    if (strcmp(input,"\n")!=0){
        fputs(input, historyFile);
    }
    fclose(historyFile);
    printf("%s", input);
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

int executeCommand(char **arg_vector, bool is_bg, unsigned int ioType, char *ioFile, bool is_pipe, int fd_input) {
    int status;
    // fd 0 is for reading & 1 is for writing
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
        if (ioType == 2) {
            file = open(ioFile, O_TRUNC | O_WRONLY | O_CREAT, 0666);
            close(1);
        } else if (ioType == 3) {
            file = open(ioFile, O_APPEND | O_WRONLY | O_CREAT, 0666);
            close(1);
        } else if (ioType == 4) {
            file = open(ioFile, O_RDONLY);
            close(0);
        }

        if (ioType != 1)
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

/// ctrl + c signal controller
int ctrlC() {
    printf("CTRL + C PRESSED!\n");
    fflush(stdin);
}

///checks if a file exists
int fileExists(const char *filename) {
    /* try to open file to read */
    FILE *file;
    if (file = fopen(filename, "r")) {
        fclose(file);
        return 1;
    }
    return 0;
}

/*** initiate interactive mode ***/
void interact() {
    getDir();
    puts(currentDir);
    unsigned int ioType;
    /*
     * 1 is normal
     * 2 is write to  file
     * 3 is append to file
     * 4 is read from file
     */
    char *arg_vector[256], *ioFile;
    int arg_count = 0;
    bool is_bg = false;

    while (true) {
        signal(SIGINT, ctrlC);

        //// prompts the user
        prompt();
        int pipe_res = 0;
        arg_count = 0;
        is_bg = false;
        ioFile = "";
        ioType = 1;

        //// read and process tokens(check for pipes and other stuff)
        readTokens();
        while (token != NULL) {
            // write to file
            if (strcmp(token, ">") == 0)
                ioType = 2;
                // append file
            else if (strcmp(token, ">>") == 0)
                ioType = 3;
                // read from file
            else if (strcmp(token, "<") == 0)
                ioType = 4;
                // pipe reached
            else if (strcmp(token, "|") == 0) {
                arg_vector[arg_count] = '\0';
                arg_count++;
                pipe_res = executeCommand(arg_vector, is_bg, ioType, ioFile, true, pipe_res);
                arg_count = 0;
            }
                // execute cmd
            else {
                // set the write file address
                if (ioType != 1) {
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
            arg_vector[arg_count] = NULL;
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
            } else if (strcmp(arg_vector[0], HISTORY) == 0) {
                historyFile = fopen(LOG, "r");
                char *line;
                size_t len = 0;
                int readFile;
                int i = 1;
                while (getline(&line, &len, historyFile) != -1) {
                    printf("%d %s", i, line);
                    i++;
                }
                continue;
            }

            // check for &
            char *amper = strchr(arg_vector[arg_count - 2], '&');
            if (amper != NULL) {
                *amper = '\0';
                is_bg = true;
            }

            if (pipe_res) {
                pipe_res = executeCommand(arg_vector, is_bg, ioType, ioFile, true, pipe_res);
                char buffer[BUFFERSIZE];
                int file_read;
                while ((file_read = read(pipe_res, buffer, BUFFERSIZE)))
                    write(1, buffer, file_read);
            } else {
                executeCommand(arg_vector, is_bg, ioType, ioFile, false, false);
            }
        }
    }
}

/*** run batch file ***/
void batch(char *fileName) {
    unsigned int ioType;
    /*
     * 1 is normal
     * 2 is write to  file
     * 3 is append to file
     * 4 is read from file
     */
    char *arg_vector[256], *ioFile;
    int arg_count = 0;
    bool is_bg = false;

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t readFile;

    fp = fopen(fileName, "r");
    while ((readFile = getline(&line, &len, fp)) != -1) {
        printf("%s\n", line);
        signal(SIGINT, ctrlC);

        //// prompts the user
        int pipe_res = 0;
        arg_count = 0;
        is_bg = false;
        ioFile = "";
        ioType = 1;

        //// read and process tokens(check for pipes and other stuff)
        token = strtok(line, WHITESPACE);
        while (token != NULL) {
            // write to file
            if (strcmp(token, ">") == 0)
                ioType = 2;
                // append file
            else if (strcmp(token, ">>") == 0)
                ioType = 3;
                // read from file
            else if (strcmp(token, "<") == 0)
                ioType = 4;
                // pipe reached
            else if (strcmp(token, "|") == 0) {
                arg_vector[arg_count] = '\0';
                arg_count++;
                pipe_res = executeCommand(arg_vector, is_bg, ioType, ioFile, true, pipe_res);
                arg_count = 0;
            }
                // execute cmd
            else {
                // set the write file address
                if (ioType != 1) {
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
            arg_vector[arg_count] = NULL;
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
                pipe_res = executeCommand(arg_vector, is_bg, ioType, ioFile, true, pipe_res);
                char buffer[BUFFERSIZE];
                int file_read;
                while ((file_read = read(pipe_res, buffer, BUFFERSIZE))) {
                    write(1, buffer, file_read);
                }
            } else {
                executeCommand(arg_vector, is_bg, ioType, ioFile, false, false);
            }
        }
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        interact();
    } else if (argc == 2) {
        char *fileName = argv[1];
        if (!fileExists(fileName)) {
            printf("%s does not exist", fileName);
            exit(1);
        }
        batch(fileName);
    } else {
        printf("too many arguments");
        exit(1);
    }
}