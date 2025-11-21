#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fnmatch.h>
#include <glob.h>
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

#define SHL_PIPE_BUFF 20
int shl_pipe(char *line) {
    // Split the line into pipeline segments
    char *commands[SHL_PIPE_BUFF];  
    int cmd_count = 0;

    char *cmd = strtok(line, "|");
    while (cmd != NULL && cmd_count < SHL_PIPE_BUFF) {
        // trim whitespace
        while (isspace((unsigned char)*cmd)) cmd++;
        char *end = cmd + strlen(cmd) - 1;
        while (end > cmd && isspace((unsigned char)*end)) end--;
        end[1] = '\0';

        commands[cmd_count++] = cmd;
        cmd = strtok(NULL, "|");
        //printf("%i\n%s", cmd_count, commands[cmd_count-1]);
    }

    // If no pipe, execute normally
    if (cmd_count == 1) {
        char **args = shl_tokenizer(commands[0]);
        char **expanded = shl_goblin(args);
        free(args);   
        args = expanded;
        
        int status;
        // if command is sh_ls then call shl_ls
        if (strcmp(args[0], "sh_ls") == 0) {
                int Lfmt = 0;
                const char *path = ".";

                for (int i = 1; args[i]; i++) {
                    if (strcmp(args[i], "-l") == 0) {
                        Lfmt = 1;  
                    }
                    else {
                        path = args[i];
                    }
                }
                shl_ls(path, Lfmt);
        } else if (strcmp(args[0], "sh_find") == 0) {
            status = shl_find(args);
        } else {
            status = shl_execute(args);
            }
        free(args);
        return status;
    }

    int prev_pipe[2];      
    int next_pipe[2];
    pid_t pids[SHL_PIPE_BUFF];

    for (int i = 0; i < cmd_count; i++) {

        // Create pipe except for last command
        if (i < cmd_count - 1) {
            if (pipe(next_pipe) < 0) {
                perror("shl: pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("shl: fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // CHILDREN:

            // If not the first command, read from prev_pipe
            if (i > 0) {
                dup2(prev_pipe[0], STDIN_FILENO);
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }

            // If not the last command, write to next_pipe
            if (i < cmd_count - 1) {
                close(next_pipe[0]);
                dup2(next_pipe[1], STDOUT_FILENO);
                close(next_pipe[1]);
            }

            char **args = shl_tokenizer(commands[i]);
            char **expanded = shl_goblin(args);
            free(args);  
            args = expanded;
            
            // if command is sh_ls then call shl_ls
            if (strcmp(args[0], "sh_ls") == 0) {
                int Lfmt = 0;
                const char *path = ".";

                for (int i = 1; args[i]; i++) {
                    if (strcmp(args[i], "-l") == 0) {
                        Lfmt = 1;  
                    }
                    else {
                        path = args[i];
                    }
                }
                shl_ls(path, Lfmt);
            
            } else if (strcmp(args[0], "sh_find") == 0) {
                shl_find(args);
            } else {
                shl_execute(args);
            }

            free(args);

            exit(EXIT_FAILURE); // shl_execute only returns on failure
        }

        // PARENT
        pids[i] = pid;

        if (i > 0) {
            close(prev_pipe[0]);
            close(prev_pipe[1]);
        }

        if (i < cmd_count - 1) {
            prev_pipe[0] = next_pipe[0];
            prev_pipe[1] = next_pipe[1];
        }
    }

    // Wait for all children
    int status;
    for (int i = 0; i < cmd_count; i++) {
        waitpid(pids[i], &status, 0);
    }

    return status;
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

void append_char(char **buf, int *len, int *cap, char c) {
    if (*len + 2 >= *cap) {
        *cap *= 2;
        char *tmp = realloc(*buf, *cap);
        if (!tmp) {
            perror("append realloc failed");
            exit(1);
        }
        *buf = tmp;
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
}


char **shl_tokenizer(char *line){
    // iterate through command line, if see \: treat next as literal,
    // if see ": find " while still allow \ to escape,
    // if see ': find ' and ensure no escape,
    // if see *space*: separate to next term in list
    int bufsize = SHL_TOK_BUFSIZE, position = 0;
    char **commands = calloc(bufsize, sizeof(char*));

    int cmdCap = 64;
    int cmdLen = 0;
    char *cmd = malloc(cmdCap);
    if (!cmd) { fprintf(stderr, "shl: initialize tokenizer allocation error\n"); exit(EXIT_FAILURE); }
    cmd[0] = '\0';


    char **Rlist;
    int Rlength;
    if (!commands){
        fprintf(stderr, "shl: tokenizer allocation error\n");
        exit(EXIT_FAILURE);
    }
     // for wild *, "", '', [], ?
    bool wild = false, doubQ = false, singleQ = false, sq = false, sqR = false;
   
    for (int i = 0; line[i] != '\0'; i++){
       /* if (wild) {
            // add all totens till *space*
            if (isspace(line[i])) {
                wild = false;
                commands[position++] = strdup(cmd);
                cmdLen = 0;
                cmd[0] = '\0';
                // resize if needed
                if (position >= bufsize){
                    bufsize += SHL_TOK_BUFSIZE;
                    commands = realloc(commands, bufsize * sizeof(char*));
                    if (!commands){
                        fprintf(stderr, "shl: command list allocation error during wild\n");
                        exit(EXIT_FAILURE);
                    }
                }
                continue;
            }
            append_char(&cmd, &cmdLen, &cmdCap, line[i]);
            continue;
        }
        if (sqR) {
            // handle range matching
            for (int j = 0; j < Rlength; j++) {
                if (line[i] == Rlist[j][0]) {  // compare char to first char of string
                    append_char(&cmd, &cmdLen, &cmdCap, line[i]);
                }
            }
            sqR = false;
            continue;
        }    
        */
        switch (line[i]) {
            case '\\':
                if (doubQ || !singleQ){ // allow escape in double quotes
                    i++;
                    append_char(&cmd, &cmdLen, &cmdCap, line[i]);
                } else {
                    append_char(&cmd, &cmdLen, &cmdCap, line[i]);
                }
                break;
            case '"': // double quote, allow escape
                doubQ = !doubQ;
                break;
            case '\'': // single quote, no escape allowed
                singleQ = !singleQ;
                break;
            /*case '*':
                wild = true;
                break;
            case '?': // match any single character
                append_char(&cmd, &cmdLen, &cmdCap, line[i]);
                break;
            case '[': 
                sq = true;

                char *startP = &line[i];
                char *endP = strchr(startP, ']');
                if (!endP) {
                    perror("shl: unmatched ] in range");
                    exit(EXIT_FAILURE);
                    break;
                }

                // find '-' inside the bracket
                char *dash = strchr(startP, '-');
                if (dash && dash < endP) {
                    sqR = true;

                    char c1 = startP[1];
                    char c2 = dash[1];

                    if (isdigit(c1) && isdigit(c2)) {
                        int start = c1 - '0';
                        int end   = c2 - '0';

                        Rlength = end - start + 1;
                        Rlist = malloc(Rlength * sizeof(char*));

                        for (int n = start; n <= end; n++) {
                            char *buf = malloc(12);
                            sprintf(buf, "%d", n);
                            Rlist[n - start] = buf;
                        }
                        i = (endP - line);
                    }
                }
                
                break;
            case ']':
                if (!sqR) {
                    sq = false;
                    break;
                }
                perror("shl: unmatched ] in range");
                exit(EXIT_FAILURE);
                break;
            */case ' ':
                if (!doubQ && !singleQ) {
                    // end of command
                    commands[position++] = strdup(cmd);
                    cmdLen = 0;
                    cmd[0] = '\0';    

                    // resize if needed
                    if (position >= bufsize){
                        bufsize += SHL_TOK_BUFSIZE;
                        commands = realloc(commands, bufsize * sizeof(char*));
                        if (!commands){
                            fprintf(stderr, "shl: command list allocation error\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else {
                    append_char(&cmd, &cmdLen, &cmdCap, line[i]);
                }
                break;
            default:
                append_char(&cmd, &cmdLen, &cmdCap, line[i]);
        }
    }
    if (doubQ && singleQ && sq) {
        perror("shl: unmatched quotation mark");
        exit(EXIT_FAILURE);
    }
    if (cmdLen > 0) {
    commands[position++] = strdup(cmd);
    }
    commands[position] = NULL;
    free(cmd);
    return commands;
}
// helper function to get file stat convert to string (shl_ls)
void shl_ls_mode(mode_t m, char *buf) {
    buf[0] = S_ISDIR(m) ? 'd' :
             S_ISLNK(m) ? 'l' :
             S_ISCHR(m) ? 'c' :
             S_ISBLK(m) ? 'b' :
             S_ISSOCK(m)? 's' :
             S_ISFIFO(m)? 'p' : '-';

    buf[1] = (m & S_IRUSR) ? 'r' : '-';
    buf[2] = (m & S_IWUSR) ? 'w' : '-';
    buf[3] = (m & S_IXUSR) ? 'x' : '-';

    buf[4] = (m & S_IRGRP) ? 'r' : '-';
    buf[5] = (m & S_IWGRP) ? 'w' : '-';
    buf[6] = (m & S_IXGRP) ? 'x' : '-';

    buf[7] = (m & S_IROTH) ? 'r' : '-';
    buf[8] = (m & S_IWOTH) ? 'w' : '-';
    buf[9] = (m & S_IXOTH) ? 'x' : '-';

    buf[10] = '\0';
}


// sh_ls command to list out files in directory (-l flag for long format)
// print out all directories or files, inlcluding parent directory and its directory
// base from text book
int shl_ls(const char *path, int Lfmt){
    // open directory, read all contents and print to stdout
    DIR *dp = opendir(path);
    if (dp == NULL) {
        perror("shl_ls: No such file or directory");  
        return 1;
    }
    struct dirent *d;
    while ((d = readdir(dp)) != NULL) {
        if (!Lfmt) {
        printf("%lu %s\n", (unsigned long) d->d_ino, d->d_name);
        } else {
            char fullPath[1024];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, d->d_name);
            struct stat st;
            if (stat(fullPath, &st) < 0) {
                perror("shl_ls: stat getting error");
                continue;
            }

            char modeBuf[11];
            shl_ls_mode(st.st_mode, modeBuf); // converting binary to text

            struct passwd *pw = getpwuid(st.st_uid);
            struct group  *gr = getgrgid(st.st_gid);
            char timeBuf[64];
            strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", localtime(&st.st_mtime));

            printf("%s %3lu %-8s %-8s %8lu %s %s\n",
                modeBuf,
                (unsigned long) st.st_nlink,
                pw ? pw->pw_name : "?",
                gr ? gr->gr_name : "?",
                (unsigned long) st.st_size,
                timeBuf,
                d->d_name
            );
        }
        
    }
    closedir(dp);
    return 1;
}
// globbing helper for shl_find (goblin and globbing sounds similar - just a pun)
// return list of expanded commands (possible commands from patterns)
char **shl_goblin(char **commands) {

    int outSize = 32;
    int outPos  = 0;
    char **expanded = malloc(outSize * sizeof(char*));

    for (int i = 0; commands[i] != NULL; i++) {
        char *tok = commands[i];

        
        if (!strpbrk(tok, "*?["))
        {
            if (outPos >= outSize) {
                outSize *= 2;
                expanded = realloc(expanded, outSize * sizeof(char*));
            }
            expanded[outPos++] = strdup(tok);
            continue;
        }
        // expansion the patterns
        glob_t g;
        int check = glob(tok, 0, NULL, &g);

        if (check == 0) {
            for (size_t k = 0; k < g.gl_pathc; k++) {
                if (outPos >= outSize) {
                    outSize *= 2;
                    expanded = realloc(expanded, outSize * sizeof(char*));
                }
                expanded[outPos++] = strdup(g.gl_pathv[k]);
            }
        } else {
            if (outPos >= outSize) {
                outSize *= 2;
                expanded = realloc(expanded, outSize * sizeof(char*));
            }
            expanded[outPos++] = strdup(tok);
        }

        globfree(&g);
    }

    expanded[outPos] = NULL;
    return expanded;
}


// shl_find recursive helper to find inside directories
void shl_find_recursive(const char *dir, char **patterns) {
    DIR *dp = opendir(dir);
    if (!dp) return;

    struct dirent *d;
    char pathbuf[1024];

    while ((d = readdir(dp)) != NULL) {

        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue;
        // expanding path to deeper directory
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, d->d_name);

        for (int i = 0; patterns[i]; i++) {
                if (strcmp(d->d_name, patterns[i]) == 0)
                    printf("%s\n", pathbuf);        
            }
        struct stat st;
        // more directories to search into
        if (stat(pathbuf, &st) == 0 && S_ISDIR(st.st_mode)) {
            shl_find_recursive(pathbuf, patterns);
        }
    }

    closedir(dp);
}

// sh_find command to find files matching a pattern (globbing pattern support)
// from curent directory and subdirectories
int shl_find(char **args) {
    if (!args[1]) {
        perror("Usage: sh_find <patterns1> <pattern2> ...");
        exit(EXIT_FAILURE);
    }

    const char *start_dir = ".";
    int pat_start = 1;
 
    struct stat st;
    if (args[1] && stat(args[1], &st) == 0 && S_ISDIR(st.st_mode)) {
        start_dir = args[1];
        pat_start = 2;
    }

    if (!args[pat_start]) {
        perror("sh_find: no patterns provided");
        exit(EXIT_FAILURE);;
    }

    shl_find_recursive(start_dir, &args[pat_start]);
    return 1;
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
