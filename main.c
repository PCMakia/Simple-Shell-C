#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "header.h"




char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &shl_cd,
    &shl_help,
    &shl_exit
};

int shl_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}


int shl_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "shl: expected argument to \"cd\"\n");
    } else{
        if (chdir(args[1]) != 0){
            perror("shl");
        }
    }
    return 1;
}

int shl_help(char **args) {
    int i;
    printf("Simple Shell\n");
    printf("Type program names and arguements, and hit enter.\n");
    printf("Arguements are separated by space");
    printf("The following are built in:\n");

    for (i = 0; i < shl_num_builtins(); i++) {
        printf("   %s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;
}

int shl_exit(char **args){
    return 0;
}


int shl_launch(char **args){
    pid_t pid,wpid;
    int status;

    pid = fork();
    if (pid == 0){
        if (execvp(args[0], args) == -1){
            perror("shl");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0){
        perror("shl");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}


int shl_execute(char **args){
    int i;

    if (args[0] == NULL){
        return 1;
    }

    for (i = 0; i < shl_num_builtins(); i++){
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }
    return shl_launch(args);
}

int shl_pipe(commands_t **commands){
    size_t i, n;
    int prev_pipe, pfds[2];

    n = sizeof(commands) / sizeof(*commands);
    prev_pipe = STDIN_FILENO;

    for (i = 0; i < n - 1; i++) {
        pipe(pfds);

        if (fork() == 0) {
            // Redirect previous pipe to stdin
            if (prev_pipe != STDIN_FILENO) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }

            // Redirect stdout to current pipe
            dup2(pfds[1], STDOUT_FILENO);
            close(pfds[1]);

            // Start command
            //execvp(commands[i][0], commands[i]);

            perror("execvp failed");
            exit(1);
        }

        // Close read end of previous pipe (not needed in the parent)
        close(prev_pipe);

        // Close write end of current pipe (not needed in the parent)
        close(pfds[1]);

        // Save read end of current pipe to use in next iteration
        prev_pipe = pfds[0];
    }

    // Get stdin from last pipe
    if (prev_pipe != STDIN_FILENO) {
        dup2(prev_pipe, STDIN_FILENO);
        close(prev_pipe);
    }

    // Start last command
    //execvp(commands[i][0], commands[i]);

    perror("execvp failed");
    exit(1);
}


#define SHL_RL_BUFSIZE 1024
#define NUM_COMMANDS 3
commands_t *shl_read_line(void){
    int bufsize = SHL_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) *bufsize);
    int c;
    char** commands = (char**)malloc(NUM_COMMANDS * sizeof(char*));
    if (commands == NULL) {
        perror("shl: allocation commands array error");
    }
    // replace command_t by array of buffers
    // make split_line works with array of buffers instead of a buffer
    // update pipe to work with split_line and execute
    commands_t *current;
    current->line2 = NULL;

    if (!buffer){
        fprintf(stderr, "shl: allocation command error\n");
        exit(EXIT_FAILURE);
    }

    while (1){
        // read characters
        c = getchar();

        // if hit EOF, replace it with a null
        if (c == EOF || c == '\n'){
            buffer[position] = '\0';
            current->line1 = buffer;
            // return the end struct 
            return current;
        } else if (c == "|") {
            // If there is |, make struct commands and do pipeline
            commands_t *new_commands;
            // separate buffer - current command
            new_commands->line2 = NULL;
            current->line1 = buffer;
            current->line2 = new_commands->line1;
            current = new_commands;
            // reset values in buffer
            memset(buffer, 0, bufsize);
            // skip the following space and reset pointer
            c = getchar();
            position = 0;
            //back to the loop
            continue;
        } else {
            buffer[position] = c;
        }
        position++;

        // if exceed buffer, relocate
        if ( position >= bufsize){
            bufsize += SHL_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer){
                fprintf(stderr, "shl: allocation increase command size error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    free(buffer);
}

#define SHL_TOK_BUFSIZE 64
#define SHL_TOK_DELIM " \t\r\n\a"
char **shl_split_line(char *line){
    int bufsize = SHL_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens){
        fprintf(stderr, "shl: allocation tokens initial error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SHL_TOK_DELIM);
    while (token != NULL){
        tokens[position] = token;
        position++;

        if (position >= bufsize){
            bufsize += SHL_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens){
                fprintf(stderr, "shl: allocation tokens increase error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, SHL_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void clear_list(commands_t** head_ref) {
    commands_t* current = *head_ref;
    commands_t* next_node;

    while (current != NULL) {
        next_node = current->line2; 
        free(current);             
        current = next_node;       
    }

    *head_ref = NULL; 
}

void shl_loop(void){
    // initialize
    commands_t *line;
    char **args;
    int status;

    // parse and execute loop
    do {
        printf("> ");
        line = *shl_read_line();
        // simple command
        if (line->line2 == NULL){
            args = shl_split_line(line->line1);
            status = shl_execute(args);

            // free linked list line
            free(line);
            line = NULL;
            free(args);
        } 
        // pipeline command
        else {
            commands_t* next_node;
            char **next_args;

            while (line->line2 != NULL) {
                next_node = line->line2;
                args = shl_split_line(line->line1);
                next_args = shl_execute(args);
                free(line);             
                line = next_node;   
                free(args);    
            }
 
        }
    } while (status);
    
}

int main(int argc, char **argv){
// load config files, run command loop
shl_loop();
// perform shutdown/clean up

return EXIT_SUCCESS;
}
