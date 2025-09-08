#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



// Shell command
int shl_cd(char **args);
int shl_help(char **args);
int shl_exit(char **args);

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
    char **args;
    int status;

    // parse and execute loop
    do {
        printf("> ");
        line = shl_read_line();
        args = shl_split_line(line);
        status = shl_execute(args);

        free(line);
        free(args);
    } while (status);
    
}

int main(int argc, char **argv){
// load config files, run command loop
shl_loop();
// perform shutdown/clean up

return EXIT_SUCCESS;
}
