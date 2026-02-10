#include <stdio.h>
#include <unistd.h>

#define MAX_LINE 80  /* The maximum length command */

int main(void)
{
char *args[MAX_LINE/2 + 1]; /* command line arguments */
int should_run = 1; /* flag to determine when to exit program */

while (should_run) {
    printf("osh&#x003E;");
    fflush(stdout);

    // ---------- PART 1 -------------
    fgets(line , MAX_LINE , stdin);

    char *token = strtok(line , " \n"); // splitting the line into tokens
    int i = 0;
    while (token != NULL){
        args[i++] = token;
        token = strtok(NULL , " ");
    } // looping through the tokens until NULL, adding them to the args array
    args[i] = NULL; // adding NULL to the end of the args array

    // args -> [ps , ael , NULL]

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