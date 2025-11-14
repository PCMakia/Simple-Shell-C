
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
void shl_loop(void);

int shl_pipe(char *commands_line);




void clear_list(commands_t** head_ref);