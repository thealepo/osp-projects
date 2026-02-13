#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80  /* The maximum length command */

int main(void)
{
char *args[MAX_LINE/2 + 1]; /* command line arguments */
int should_run = 1; /* flag to determine when to exit program */

char line[MAX_LINE]; // line to store the user input
char history[100][MAX_LINE]; // history to store the previous commands
int history_index = 0; // index to store the current history

while (should_run) {
    printf("osh&#x003E;");
    fflush(stdout);

    // -------------------- PART 2 --------------------------------
    fgets(line , MAX_LINE , stdin);

    // --- PART 3 (!! History) ---
    if (strcmp(line , "!!") == 0){ // if the user inputs !!, print the previous command
        if (history_index == 0){
            printf("No previous commands\n");
            continue;
        }
        strcpy(line , history[history_index - 1]);
    }
    else{ // if the user inputs a command, add it to the history
        strcpy(history[history_index++] , line);
    }

    char *token = strtok(line , " \n"); // splitting the line into tokens
    int i = 0;
    while (token != NULL){
        args[i++] = token;
        token = strtok(NULL , " \n");
    } // looping through the tokens until NULL, adding them to the args array
    args[i] = NULL; // adding NULL to the end of the args array
    // args -> [ps , ael , NULL] **EXAMPLE**

    // --- PART 4 (> and <)
    for (int j = 0 ; j < i ; j++){
        if (strcmp(args[j] , ">") == 0){
            // redirecting output to a file
            int fd = open(args[j+1] , O_CREAT | O_WRONLY | O_TRUNC , 0644);
            dup2(fd , STDOUT_FILENO);
        }
        else if (strcmp(args[j] , "<") == 0){
            // redirecting input from a file
            int fd = open(args[j+1] , O_RDONLY);
            dup2(fd , STDIN_FILENO);
        }

        // ------ PART 5 (PIPE)
        if (strcmp(args[j] , "|") == 0){
            int pipefd[j];
            pipe(pipefd); // creating a pipe
            pid_t pid = fork(); // forking a child process
            if (pid == 0){
                close(pipefd[0]);
                dup2(pipefd[1] , STDOUT_FILENO);
                execvp(args[j+1] , args); // executing the command
            }
            else{
                close(pipefd[1]);
            }
            wait(NULL);
        }
    }

    pid_t pid = fork(); // forking a child process
    if (pid == 0){
        execvp(args[0] , args); // executing the command if it is a child process
    }
    else{
        wait(NULL); // otherwise, wait for the child process to finish
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