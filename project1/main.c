#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_LINE 80  /* The maximum length command */

int main(void)
{
char *args[MAX_LINE/2 + 1]; /* command line arguments */
int should_run = 1; /* flag to determine when to exit program */

char line[MAX_LINE]; // line to store the user input
char history[100][MAX_LINE] = ""; // history to store the previous commands
int has_history = 0; // flag to determine if the history is empty

while (should_run) {
    printf("osh> "); // prompt
    fflush(stdout);

    if (!fgets(line , MAX_LINE , stdin)){
        break;
    }

    // --- PART 3 (!! History) ---
    if (strcmp(line , "!!\n") == 0){ // if the user inputs !!, print the previous command
        if (!has_history){
            printf("No previous commands.\n");
            continue;
        }
        printf("%s" , history);
        strcpy(line , history);
    }
    else{ // if the user inputs a command, add it to the history
        strcpy(line , line);
        has_history = 1;
    }

    // --- PART 2 ---
    char *token = strtok(line , " \n"); // splitting the line into tokens
    int i = 0;
    while (token != NULL){
        args[i++] = token;
        token = strtok(NULL , " \n");
    } // looping through the tokens until NULL, adding them to the args array
    args[i] = NULL; // adding NULL to the end of the args array
    // args -> [ps , ael , NULL] **EXAMPLE**

    // --- PART 5 (PIPE) ---
    int pipe_index = -1;
    for (int j = 0 ; j < i ; j++){
        if (strcmp(args[j] , "|") == 0){
            pipe_index = j;
            break;
        }
    }

    if (pipe_index != -1){
        args[pipe_index] = NULL;
        char **cmd1 = args; // first command
        char **cmd2 = &args[pipe_index + 1]; // second command

        int pipefd[2];
        pipe(pipefd); // creating a pipe

        if (fork() == 0){ // first child process
            dup2(pipefd[1] , STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(cmd1[0] , cmd1);
            exit(1);
        }
        if (fork() == 0){ // second child process
            dup2(pipefd[0] , STDIN_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(cmd2[0] , cmd2);
            exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);
        continue; // continue to the next iteration of the loop
    }


    // --- PART 4 (> and <) ---
    pid_t pid = fork(); // forking a child process
    if (pid == 0){
        for (int j = 0 ; j < i ; j++){
            if (strcmp(args[j] , ">") == 0){
                int fd = open(args[j+1] , O_CREAT | O_WRONLY | O_TRUNC , 0644);
                dup2(fd , STDOUT_FILENO);
                args[j] = NULL;
            } else if (strcmp(args[j] , "<") == 0){
                int fd = open(args[j+1] , O_RDONLY);
                dup2(fd , STDIN_FILENO);
                args[j] = NULL;
            }
        }

        execvp(args[0] , args);
        printf("Invalid command.\n");
        exit(1);
    }
    else{
        wait(NULL);
    }
    /**
    * After reading user input, the steps are:
    * (1) fork a child process using fork()
    * (2) the child process will invoke execvp()
    * (3) parent will invoke wait() unless command included &
    */
}

return 0;
}