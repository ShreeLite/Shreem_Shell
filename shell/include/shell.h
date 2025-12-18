#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_PATH_LEN 1024
#define MAX_TOKENS 512


//enum for token types
typedef enum {
    TOKEN_NAME, TOKEN_PIPE, TOKEN_AMPERSAND, TOKEN_SEMICOLON,
    TOKEN_INPUT, TOKEN_OUTPUT, TOKEN_DOUBLE_OUTPUT,
    TOKEN_END, TOKEN_INVALID
} TokenType;

//struct for holding tokens
typedef struct {
    TokenType type;
    char value[MAX_PATH_LEN];
} Token;

//global variables
extern Token tokens[1024];  //array of tokens
extern int token_count;   //token count so far
extern const char* current_input;  //pointer to start of the command



//Parser state
typedef struct {
    int current_token_index;
} ParserState;

extern ParserState parser_state;

//function prototypes

void tokenise(void);
void prompt(const char* home_directory);

// Parser functions
int parse(void);
int parse_shell_cmd(void);
int parse_cmd_group(void);
int parse_atomic(void);
int parse_input(void);
int parse_output(void);
int parse_name(void);

// Parser utility functions
Token current_token(void);
void consume_token(void);
bool expect_token(TokenType type);
bool match_token(TokenType type);
void init_parser(void);

// Built-in commands
void execute_hop(const char* home_directory);
void execute_reveal(const char* home_directory);
void execute_log(const char* home_directory);

// Log functionality
void init_log(const char* home_directory);
void add_command_to_log(const char* command);
int should_log_command(const char* command);
void save_log_to_file(const char* home_directory);
void load_log_from_file(const char* home_directory);
char* reconstruct_command_from_tokens(void);

// Command execution
int execute_command(void);
int execute_external_command(char **args);
int setup_input_redirection(int start_index, int end_index);
int setup_output_redirection(int start_index, int end_index);
int has_output_redirection(int start_index, int end_index);

// Pipe functionality
int execute_pipeline(const char* home_directory);
int count_pipes(int start_index, int end_index);
void execute_command_in_pipeline(int cmd_start, int cmd_end, int pipe_in, int pipe_out, const char* home_directory);
int find_pipe_segments(int **segments, int *num_segments);
int has_pipes(void);

extern char previous_cwd[MAX_PATH_LEN];

// Sequential execution functions
int execute_sequential_commands(const char* home_directory);
int find_command_segments(int **segments, int *num_segments, TokenType **separators);

// Background execution functions
#define MAX_BACKGROUND_JOBS 100

// Process states for activities command
typedef enum {
    PROCESS_RUNNING,
    PROCESS_STOPPED
} ProcessState;

typedef struct {
    pid_t pid;
    int job_number;
    char command[MAX_PATH_LEN];
    char command_name[MAX_PATH_LEN];  // For activities command sorting
    int is_active;
    ProcessState state;
} BackgroundJob;

extern BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
extern int next_job_number;

void init_background_jobs(void);
int add_background_job(pid_t pid, const char* command);
int add_stopped_job(pid_t pid, const char* command);
void check_background_jobs(void);
void cleanup_background_job(int job_index);
int execute_background_command(int start_index, int end_index, const char* command_str);

// Activities command
void execute_activities(void);

// Ping command
void execute_ping(void);

// Signal handling and job control
void setup_signal_handlers(void);
void set_foreground_process(pid_t pgid, const char* command);
void clear_foreground_process(void);
void handle_eof(void);

// Global variables for signal handling
extern pid_t current_foreground_pgid;
extern char current_foreground_command[MAX_PATH_LEN];

// fg and bg commands
void execute_fg(void);
void execute_bg(void);

#endif 
