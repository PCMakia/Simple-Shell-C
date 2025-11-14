#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
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


#define SHL_RL_BUFSIZE 1024
char *shl_read_line(void){
    int bufsize = SHL_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) *bufsize);
    int c;

    if (!buffer){
        fprintf(stderr, "shl: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1){
        // read characters
        c = getchar();

        // if hit EOF, replace it with a null
        if (c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // if exceed buffer, relocate
        if ( position >= bufsize){
            bufsize += SHL_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer){
                fprintf(stderr, "shl: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define SHL_PIPE_BUFF 256
int shl_pipe(char *commands_line){
    // find | in commands, then separate them 
    // and execute split_line on each cluster
    char **args;
    int status;
    char *check = strchr(commands_line, '|');

    if (check == NULL) {
        // execute single
        args = shl_split_line(commands_line);
        status = shl_execute(args);

        free(args);
        
        return status;
    }
    char *command = strtok(commands_line, "|");
    // encapsule in loop to keep making pipe
    do{
    int pipefd[2];
    int pipeRet[2];
    int buff_size = SHL_PIPE_BUFF;
    char *buff = malloc(sizeof(char) * buff_size);
    
    int pip = pipe(pipefd);
    if (pip == -1) {
        perror("pipe input failed");
        exit(EXIT_FAILURE);
    }
    int pipR = pipe(pipeRet);
    if (pipR == -1) {
        perror("pipe return failed");
        exit(EXIT_FAILURE);
    }
    // from here divide work into child and parent if else loop
    // do pipe line work and return status code
    

    
    // prune white spaces between commands
    int start = 0;
    int end = strlen(command) - 1;
    
    while (isspace((unsigned char)command[end])) {end--;}
    while (isspace((unsigned char)command[start])) {start++; }
    char *newstr = (char *)malloc( sizeof(char) * (end - start + 2) );
    strncpy(newstr, command + start, end - start + 1);
    newstr[end - start + 1] = '\0';
    strcpy(command, newstr);
    free(newstr);
        

    pid_t pid_1,wpid;
    int status;

    pid_1 = fork();
    if (pid_1 == -1){
        perror("shl: fork failed");
        exit(EXIT_FAILURE);
    } 


        // execute command, get new command, pipe them?
    if (pid_1 == 0) {
        // child pipe (read from pipefd and write result to pipeRet)
        close(pipefd[0]);
        close(pipeRet[1]);

        dup2(pipefd[1], STDOUT_FILENO);
        
        args = shl_split_line(command);
        status = shl_execute(args);
        free(args);
        close(pipefd[1]);
    } else {
        // parent pipe (read from pipeRet, and write instructions to pipefd)
        close(pipefd[1]);
        close(pipeRet[0]);
        //command = strtok(NULL, "|");
        pid_t pid_2 = fork();
        if (pid_2 == 0) {
            // second child to read from first child
            dup2(pipefd[0], STDIN_FILENO);
            

            args = shl_split_line(command);
            status = shl_execute(args);
            free(args);
            close(pipefd[0]);
        } else if (pid_2 < 0) {
            perror("shl: fork failed");
            exit(EXIT_FAILURE);
        } else {
            // parent waits
            close(pipefd[0]);
            close(pipefd[1]);
            do {
               // wpid = waitpid(pid_1, &status, WUNTRACED);
                wpid = waitpid(pid_2, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        
    }
        command = strtok(NULL, "|");
    }while (command != NULL);
    return 0;
}

#define SHL_TOK_BUFSIZE 64
#define SHL_TOK_DELIM " \t\r\n\a"
char **shl_split_line(char *line){
    int bufsize = SHL_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens){
        fprintf(stderr, "shl: allocation error\n");
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
                fprintf(stderr, "shl: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, SHL_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}


void shl_loop(void){
    // initialize
    char *line;
    int status;

    // parse and execute loop
    do {
        printf("> ");
        line = shl_read_line();
        // make pipe to split code cluster
        // execute either version single or pipe

        status = shl_pipe(line);

        free(line);
    } while (status);
    
}

int main(int argc, char **argv){
// load config files, run command loop
shl_loop();
// perform shutdown/clean up

return EXIT_SUCCESS;
}
