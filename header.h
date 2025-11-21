
typedef struct Command_L commands_t;

struct Command_L {
    char *line1;
    commands_t *line2;
};

// Shell command
int shl_cd(char **args);
int shl_help(char **args);
int shl_exit(char **args);

// Shell functions
int shl_launch(char **args);
int shl_execute(char **args);
char *shl_read_line(void);
char **shl_split_line(char *line);
char **shl_tokenizer(char *line);
void shl_loop(void);

int shl_pipe(char *commands_line);



void append_char(char **buf, int *len, int *cap, char c); 
void clear_list(commands_t** head_ref);

// File and directory commands
typedef unsigned int mode_t;
void shl_ls_mode(mode_t m, char *buf);
int shl_ls(const char *path, int Lfmt);
void shl_find_recursive(const char *dir, char **patterns);
int shl_find(char **args);
char **shl_goblin(char **commands); // globbing expansion