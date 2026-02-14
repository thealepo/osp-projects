#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_LINE 80  /* The maximum length command */

#define MAX_ALIASES 10
#define MAX_JOBS 10

// Data Structures for additional features -----------
// aslias struct
struct Alias {
    char name[MAX_LINE];
    char command[MAX_LINE];
};
struct Alias aliases[MAX_ALIASES];
int alias_count = 0;

// bakcgroung process struct
struct Job {
    int id;
    pid_t pid;
    char command[MAX_LINE];
};
struct Job jobs[MAX_JOBS];
int job_count = 0;

void handle_signal(int signal);
int parse_input(char *line , char **args , int *background);
void execute_command(char **args , int background);

// handle signal (ctrl+c)
void handle_sigint(int signal){
    write(STDOUT_FILENO , "\nosh> " , 7);
}
// handle signal (ctrl+z) (handles child process death)
void handle_sigchld(int signal){
    int saved_errno = 0;
    pid_t = pid;
    while ((pid = waitpid(-1 , NULL , WNOHANG)) > 0){
        remove_job(pid); // remove job from the list when done
    }
}

// add an alias
void add_alias(char *name , char *command){
    if (alias_count < MAX_ALIASES){
        strcpy(aliases[alias_count].name , name);
        strcpy(aliases[alias_count].command , command);
        alias_count++;
        printf("Alias %s added.\n" , name);
    }else{
        printf("Maximum number of aliases reached.\n");
    }
}

// add a job
void add_job(pid_t pid , char *command){
    if (job_count < MAX_JOBS){
        jobs[job_count].id = job_count + 1;
        jobs[job_count].pid = pid;
        strcpy(jobs[job_count].command , command);
        job_count++;
    }
}
// remove a job
void remove_job(pid_t pid){
    int found = 0;
    for (int i = 0 ; i < job_count ; i++){
        if (jobs[i].pid == pid){
            found = 1;
            // you have to shift the jobs
            for (int j = i ; j < job_count - 1 ; j++){
                jobs[j] = jobs[j+1];
                jobs[j].id = j + 1;
            }
            job_count--;
            break;
        }
    }
}
// print jobs
void print_jobs(){
    printf("Running jobs:\n");
    for (int i = 0 ; i < job_count ; i++){
        printf("[%d] %s\n" , jobs[i].id , jobs[i].command);
    }
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

    // checking & for background process
    if (i > 0 && strcmp(args[i-1] , "&") == 0){
        *background = 1;
        args[i-1] = NULL; // removing the & from the args
        i--;
    }
    return i;
}

// handles the pipes (|) and the redirects (> , <) and the background process (&)
void execute_command(char **args , int background , char *raw_command){

    // --- ALIASES & JOBS --- (additional features)
    if (strcmp(args[0] , "jobs") == 0){
        print_jobs();
        return;
    }

    if (strcmp(args[0] , "alias") == 0){
        if (args[1] != NULL  && args[2] != NULL){
            // reconstruct the command part
            add_alias(args[1] , args[2]);
        }else{
            printf("Usage: alias <alias> <command>\n");
        }
        return;
    }

    char *alias_command = check_alias(args[0]);
    if (alias_command != NULL){
        args[0] = alias_command; // if current command is an alias, replace it with the actual command
    }


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

        // parent closes and waits for the child processes
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
        signal(SIGINT , SIG_DFL);
        signal(SIGCHLD , SIG_DFL);

        // handling > and < redirects
        for (int j = 0 ; args[j] != NULL ; j++){
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
            waitpid(pid , NULL , 0);
        } else {
            printf("[Job %d] %s\n" , job_count + 1 , args[0]);
            add_job(pid , raw_command);
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

    char raw_command[MAX_LINE]; // raw command to store the original command

    // setting up the signal handlers
    signal(SIGINT , handle_sigint);
    signal(SIGCHLD , handle_sigchld);
    signal(SIGTSTP , SIG_IGN);

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

        strcpy(raw_command , line);

        // PARSING
        int background = 0;
        int arg_count = parse_input(line , args , &background);

        if (arg_count == 0) continue;

        // exit command
        if (strcmp(args[0] , "exit") == 0){
            should_run = 0;
            continue;
        }

        execute_command(args , background , raw_command);

        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command included &
        */
    }

    return 0;
}