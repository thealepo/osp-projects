#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_LINE 80  /* The maximum length command */

void handle_signal(int signal);
int parse_input(char *line , char **args , int *background);
void execute_command(char **args , int background);

// handle signal
void handle_signal(int signal){
    write(STDOUT_FILENO , "Process terminated by signal\n" , 25);
    exit(1);
}

// parses the input line into arguments
int parse_input(char *line , char **args , int *background){
    int i = 0;
    *background = 0;
    char *token = strtok(line , " ");

    while (token != NULL){
        args[i++] = token;
        token = strtok(NULL , " ");
    }
    args[i] = NULL;

    if (i > 0 && strcmp(args[i-1] , "&") == 0){
        *background = 1;
        args[i-1] = NULL;
        i--;
    }
    return i;
}

// handles the pipes (|) and the redirects (> , <) and the background process (&)
void execute_command(char **args , int background){

    // finding the pipe index
    int pipe_idx = -1;
    for (int j = 0 ; args[j] != NULL ; j++){
        if (strcmp(args[j] , "|") == 0){
            pipe_idx = j;
            break;
        }
    }

    // --- PART 5: PIPES (|) ---
    if (pipe_idx != -1){
        args[pipe_idx] = NULL; // splitting the args
        char **cmd2 = &args[pipe_idx + 1];

        int pipefd[2];
        if (pipe(pipefd) == -1){
            return;
        }

        pid_t p1 = fork();
        if (p1 == 0){
            dup2(pipefd[1] , STDOUT_FILENO); // writing to the pipe
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(args[0] , args);
            exit(1);
        }
        
        pid_t p2 = fork();
        if (p2 == 0){
            dup2(pipefd[0] , STDIN_FILENO); // reading from the pipe
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(cmd2[0] , cmd2);
            exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);
        return;
    }

    // PART 2 & 4: > and < redirects & background process (&)
    pid_t pid = fork();
    if (pid == 0){
        // child process

        // handling > and < redirects
        for (int j = 0 ; j < i ; j++){
            if (strcmp(args[j] , ">") == 0){
                int fd = open(args[j+1] , O_CREAT | O_WRONLY | O_TRUNC , 0644);
                dup2(fd , STDOUT_FILENO);
                close(fd);
                args[j] = NULL;
            } else if (strcmp(args[j] , "<") == 0){
                int fd = open(args[j+1] , O_RDONLY);
                dup2(fd , STDIN_FILENO);
                close(fd);
                args[j] = NULL;
            }
        }

        execvp(args[0] , args);
        printf("Invalid command.\n");
        exit(1);
    }
    else{
        // parent process
        if (!background){
            wait(NULL);
        }
    }
}

int main(void)
{
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */

    char line[MAX_LINE]; // line to store the user input
    char history[MAX_LINE] = ""; // history to store the previous commands
    int has_history = 0; // flag to determine if the history is empty

    while (should_run) {
        printf("osh> ");
        fflush(stdout);

        if (!fgets(line , MAX_LINE , stdin)){
            if (feof(stdin)){
                printf("\n");
                break;
            }
            continue;
        }

        // remove the newline character from the input
        line[strcspn(line , "\n")] = 0;

        // skip any empty lines
        if (strlen(line) == 0) continue;

        // --- PART 3: HISTORY (!!) ---
        if (strcmp(line , "!!") == 0){
            if (!has_history){
                printf("No previous commands in history.\n");
                continue;
            }
            printf("%s\n" , history);
            strcpy(line , history);
        }else{
            strcpy(history , line);
            has_history = 1;
        }

        // PARSING
        int background = 0;
        int arg_count = parse_input(line , args , &background);

        if (arg_count == 0) continue;

        // exit command
        if (strcmp(args[0] , "exit") == 0){
            should_run = 0;
            continue;
        }

        execute_command(args , background);

        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command included &
        */
    }

    return 0;
}