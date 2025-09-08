#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv){
// load config files, run command loop
shl_loop();
// perform shutdown/clean up

return EXIT_SUCCESS;
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

#define SHL_RL_BUFSIZE 1024
char *shl_read_line(void){
    int bufsize = SHL_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) *bufsize);
    int c;

    if (!buffer){
        fprintf(stderr, "lsh: allocation error\n");
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
                fprintf(stderr, 'lsh: allocation error\n');
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
        fprintf(stderr, "lsh: allocation error\n");
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
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, SHL_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}
