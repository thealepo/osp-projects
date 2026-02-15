#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>

#define MAX_LINE 80  /* The maximum length command */

#define MAX_ALIASES 10
#define MAX_JOBS 10
#define MAX_HISTORY 10

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

// history array
char history[MAX_HISTORY][MAX_LINE];
int history_count = 0;

// THE FUNCTIONS
void handle_sigint(int signal);
void handle_sigchld(int signal);
int parse_input(char *line , char **args , int *background);
void execute_command(char **args , int background , char *raw_command);
void add_alias(char *name , char *command);
char *check_alias(char *name);
void add_job(pid_t pid , char *command);
void remove_job(pid_t pid);
void print_jobs();
void add_history(char *line);
void print_history();
void bring_job_foreground(int job_id);
char *get_history(int index);

// handle signal (ctrl+c)
void handle_sigint(int signal){
    write(STDOUT_FILENO , "\nosh> " , 7);
}
// handle signal (ctrl+z) (handles child process death)
void handle_sigchld(int signal){
    pid_t pid;
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
        printf("Alias added: %s -> %s\n" , name , command);
    }else{
        printf("Alias list is full.\n");
    }
}
// check an alias
char *check_alias(char *name){
    for (int i = 0 ; i < alias_count ; i++){
        if (strcmp(aliases[i].name , name) == 0) return aliases[i].command;
    }
    return NULL;
}

// add a job
void add_job(pid_t pid , char *command){
    if (job_count < MAX_JOBS){
        jobs[job_count].id = job_count + 1;
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command , command , MAX_LINE);
        job_count++;
    }
}
// remove a job
void remove_job(pid_t pid){
    for (int i = 0 ; i < job_count ; i++){
        if (jobs[i].pid == pid){
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
        printf("[%d] %d %s\n" , jobs[i].id , jobs[i].pid , jobs[i].command);
    }
}

void add_history(char *line){
    int idx = history_count % MAX_HISTORY;
    strncpy(history[idx] , line , MAX_LINE);
    history_count++;
}
void print_history() {
    if (history_count == 0){
        printf("No previous commands in history.\n");
        return;
    }

    // checking how many items there are to print, and if there are more than the max, we just print the max
    int items_to_print = history_count;
    if (items_to_print > MAX_HISTORY){
        items_to_print = MAX_HISTORY;
    }

    // finding the start index for the history
    int current_index = 0;
    if (history_count > MAX_HISTORY){
        current_index = history_count % MAX_HISTORY;
    }

    // loop to print
    for (int i = 0 ; i < items_to_print ; i++){
        printf("%d %s\n" , i + 1 , history[current_index]);
        current_index++;

        if (current_index >= MAX_HISTORY){
            current_index = 0;
        }
    }
}

void bring_job_foreground(int job_id){
    int found_idx = -1;
    for (int i = 0 ; i < job_count ; i++){
        if (jobs[i].id == job_id){
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1){
        printf("Job [%d] %s to foreground.\n" , job_id , jobs[found_idx].command);
        pid_t pid = jobs[found_idx].pid;

        // remove from background
        remove_job(pid);

        // wait for it to finish
        waitpid(pid , NULL , 0);
    }else{
        printf("Job [%d] not found.\n" , job_id);
    }
}

char *get_history(int index){
    if (history_count == 0) return NULL;

    int items_available = (history_count > MAX_HISTORY) ? MAX_HISTORY : history_count;
    int first_available_index = (history_count > MAX_HISTORY) ? (history_count - MAX_HISTORY + 1) : 1;

    // checking if the index is valid
    if (index < first_available_index || index > history_count){
        return NULL;
    }

    // mapping the display number
    int actual_index = (index - 1) % MAX_HISTORY;
    return history[actual_index];
}

// parses the input line into arguments
int parse_input(char *line , char **args , int *background){
    int i = 0;
    *background = 0;
    int length = strlen(line);
    int start = -1;
    int in_quotes = 0;

    for (int j = 0 ; j <= length ; j++) {
        if (line[j] == '\"') {
            in_quotes = !in_quotes;
            if (start == -1) start = j + 1; // start of quoted string (skipping quote)
            else if (!in_quotes) {
                 // end of quoted string
                 line[j] = '\0'; // terminate
                 args[i++] = &line[start];
                 start = -1;
            }
            continue;
        }

        if (!in_quotes) {
            if (isspace(line[j]) || line[j] == '\0') {
                if (start != -1) {
                    line[j] = '\0';
                    args[i++] = &line[start];
                    start = -1;
                }
            } else {
                if (start == -1) start = j;
            }
        }
    }
    args[i] = NULL;

    // checking & for background process
    if (i > 0 && strcmp(args[i-1] , "&") == 0){
        *background = 1;
        args[--i] = NULL;
    }
    return i;
}

// handles the pipes (|) and the redirects (> , <) and the background process (&)
void execute_command(char **args , int background , char *raw_command){

    if (strcmp(args[0] , "jobs") == 0){
        print_jobs();
        return;
    }
    if (strcmp(args[0] , "history") == 0){
        print_history();
        return;
    }
    if (strcmp(args[0] , "fg") == 0){
        if (args[1] == NULL){
            printf("Usage: fg [job_id]\n");
        }else{
            bring_job_foreground(atoi(args[1]));
        }
        return;
    }
    if (strcmp(args[0] , "alias") == 0 && args[1] && args[2]){
        add_alias(args[1] , args[2]);
        return;
    }

    /*
    char *alias_command = check_alias(args[0]);
    if (alias_command) args[0] = alias_command; // if current command is an alias, replace it with the actual command
    */

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
        int fd[2];
        if (pipe(fd) == -1){
            perror("Pipe Failed.");
            return;
        }

        if (fork() == 0){
            dup2(fd[1] , STDOUT_FILENO); // writing to the pipe
            close(fd[0]);
            close(fd[1]);
            execvp(args[0] , args);
            perror("Execution cmd1 Failed.");
            exit(1);
        }
        if (fork() == 0){
            dup2(fd[0] , STDIN_FILENO); // reading from the pipe
            close(fd[0]);
            close(fd[1]);
            execvp(args[pipe_idx + 1] , &args[pipe_idx + 1]);
            perror("Execution cmd2 Failed.");
            exit(1);
        }

        close(fd[0]);
        close(fd[1]);
        wait(NULL);
        wait(NULL);
        return;
    }

    // PART 2 & 4: > and < redirects & background process (&)
    pid_t pid = fork();
    if (pid == 0) {
        // child
        signal(SIGINT , SIG_DFL); // reset signal handler

        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], ">") == 0) {

                int fd = open(args[j + 1] , O_CREAT | O_WRONLY | O_TRUNC , 0644); // opening the file for writing
                dup2(fd , STDOUT_FILENO);
                close(fd);
                args[j] = NULL;

            } else if (strcmp(args[j], "<") == 0) {

                int fd = open(args[j + 1] , O_RDONLY); // opening the file for reading
                dup2(fd, STDIN_FILENO);
                close(fd);
                args[j] = NULL;

            }
        }

        // alias handling
        char *alias_command = check_alias(args[0]);
        if (alias_command != NULL){
            char *new_args[MAX_LINE];
            int index = 0;

            char alias_copy[MAX_LINE];
            strcpy(alias_copy , alias_command);

            // tokenizing the alias command
            char *token = strtok(alias_copy, " \t\n");
            while(token != NULL) {
                new_args[index++] = token;
                token = strtok(NULL, " \t\n");
            }

            for (int k = 1; args[k] != NULL; k++) {
                new_args[index++] = args[k];
            }
            new_args[index] = NULL; // null terminate the list

            execvp(new_args[0] , new_args);
        }else{
            // no alias found, execute the original command
            execvp(args[0] , args);
        }

        perror("Execution failed");
        exit(1);
    } else {
        // parent
        if (!background) waitpid(pid , NULL , 0);
        else add_job(pid , raw_command);
    }
}

int main(void)
{
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */
    char line[MAX_LINE];
    char raw_command[MAX_LINE];

    // setting up the signal handlers
    signal(SIGINT , handle_sigint);
    signal(SIGCHLD , handle_sigchld);
    signal(SIGTSTP , SIG_IGN); // ignore ctrl+z

    while (should_run) {
        printf("osh> ");
        fflush(stdout);

        if (!fgets(line , MAX_LINE , stdin)){
            printf("\n");
            break;
        }

        // remove the newline character from the input
        line[strcspn(line , "\n")] = 0;

        // skip any empty lines
        if (strlen(line) == 0) continue;

        // --- PART 3: HISTORY (!!) ---
        if (line[0] == '!'){
            char *retrieved_command = NULL;

            if (line[1] == '!'){
                // case is last command !!
                if (history_count == 0){
                    printf("No previous command.\n");
                    continue;
                }
                retrieved_command = get_history(history_count);
            }else if (isdigit(line[1])){
                // case is !N
                int target = atoi(&line[1]);
                retrieved_command = get_history(target);
                if (retrieved_command == NULL){
                    printf("No such command in history.\n");
                    continue;
                }
            }

            if (retrieved_command != NULL){
                printf("%s\n" , retrieved_command);
                strcpy(line , retrieved_command);
            }
        }
        add_history(line);
        strcpy(raw_command , line); // storing the original command

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