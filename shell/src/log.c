#include "shell.h"
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LOG_ENTRIES 15
#define MAX_COMMAND_LEN 1024

// Global log storage
static char command_log[MAX_LOG_ENTRIES][MAX_COMMAND_LEN];
static int log_count = 0;
static int log_start = 0; // Circular buffer start index
static char log_file_path[MAX_PATH_LEN];

/**
 * @brief Initialize the log system and load existing log from file
 * @param home_directory The shell's home directory for log file storage
 */
void init_log(const char* home_directory) {
    // Create log file path: ~/.myshell_log
    snprintf(log_file_path, sizeof(log_file_path), "%s/.myshell_log", home_directory);
    
    // Initialize log arrays
    for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
        command_log[i][0] = '\0';
    }
    log_count = 0;
    log_start = 0;
    
    // Load existing log from file
    load_log_from_file(home_directory);
}

/**
 * @brief Load command history from the persistent log file
 * @param home_directory The shell's home directory
 */
void load_log_from_file(const char* home_directory) {
    FILE *file = fopen(log_file_path, "r");
    if (file == NULL) {
        // File doesn't exist yet, which is fine for first run
        return;
    }
    
    char line[MAX_COMMAND_LEN];
    log_count = 0;
    log_start = 0;
    
    // Read commands from file (they are stored oldest to newest)
    while (fgets(line, sizeof(line), file) && log_count < MAX_LOG_ENTRIES) {
        // Remove newline character
        line[strcspn(line, "\n")] = '\0';
        
        if (strlen(line) > 0) {
            strcpy(command_log[log_count], line);
            log_count++;
        }
    }
    
    fclose(file);
}

/**
 * @brief Save command history to the persistent log file
 * @param home_directory The shell's home directory
 */
void save_log_to_file(const char* home_directory) {
    FILE *file = fopen(log_file_path, "w");
    if (file == NULL) {
        perror("Failed to save command log");
        return;
    }
    
    // Write commands from oldest to newest
    for (int i = 0; i < log_count; i++) {
        int index = (log_start + i) % MAX_LOG_ENTRIES;
        fprintf(file, "%s\n", command_log[index]);
    }
    
    fclose(file);
}

/**
 * @brief Reconstruct the full command string from current tokens
 * @return Dynamically allocated string containing the full command (caller must free)
 */
char* reconstruct_command_from_tokens(void) {
    if (token_count == 0) {
        return NULL;
    }
    
    // Calculate required buffer size
    int total_len = 0;
    for (int i = 0; i < token_count; i++) {
        total_len += strlen(tokens[i].value) + 1; // +1 for space or null terminator
    }
    
    char *command = malloc(total_len + 1);
    if (command == NULL) {
        return NULL;
    }
    
    command[0] = '\0';
    
    // Reconstruct command with proper spacing
    for (int i = 0; i < token_count; i++) {
        if (i > 0) {
            strcat(command, " ");
        }
        strcat(command, tokens[i].value);
    }
    
    return command;
}

/**
 * @brief Check if a command should be logged
 * @param command The command string to check
 * @return 1 if should log, 0 if should not log
 * 
 * Rules:
 * - Don't log if command starts with "log"
 * - Don't log if identical to the most recent command
 * - Don't log empty commands
 */
int should_log_command(const char* command) {
    if (command == NULL || strlen(command) == 0) {
        return 0;
    }
    
    // Don't log "log" commands
    if (strncmp(command, "log", 3) == 0 && 
        (command[3] == '\0' || command[3] == ' ')) {
        return 0;
    }
    
    // Don't log if identical to the most recent command
    if (log_count > 0) {
        int most_recent_index = (log_start + log_count - 1) % MAX_LOG_ENTRIES;
        if (strcmp(command, command_log[most_recent_index]) == 0) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief Add a command to the log with circular buffer management
 * @param command The command string to add
 */
void add_command_to_log(const char* command) {
    if (!should_log_command(command)) {
        return;
    }
    
    int insert_index;
    
    if (log_count < MAX_LOG_ENTRIES) {
        // Still have room in the log
        insert_index = log_count;
        log_count++;
    } else {
        // Log is full, overwrite oldest entry
        insert_index = log_start;
        log_start = (log_start + 1) % MAX_LOG_ENTRIES;
    }
    
    // Store the command
    strncpy(command_log[insert_index], command, MAX_COMMAND_LEN - 1);
    command_log[insert_index][MAX_COMMAND_LEN - 1] = '\0';
    
    // Save to file to ensure persistence
    save_log_to_file(getenv("HOME")); // Using HOME env var as fallback
}

/**
 * @brief Execute the log command with its various modes
 * @param home_directory The shell's home directory
 */
void execute_log(const char* home_directory) {
    if (token_count == 1) {
        // No arguments: print stored commands oldest to newest
        if (log_count == 0) {
            // No output if log is empty
            return;
        }
        
        for (int i = 0; i < log_count; i++) {
            int index = (log_start + i) % MAX_LOG_ENTRIES;
            printf("%s\n", command_log[index]);
        }
        
    } else if (token_count == 2 && strcmp(tokens[1].value, "purge") == 0) {
        // Purge: clear the history
        log_count = 0;
        log_start = 0;
        
        // Clear the file as well
        FILE *file = fopen(log_file_path, "w");
        if (file != NULL) {
            fclose(file);
        }
        
    } else if (token_count == 3 && strcmp(tokens[1].value, "execute") == 0) {
        // Execute <index>: execute command at given index (1-indexed, newest to oldest)
        int index = atoi(tokens[2].value);
        
        if (index < 1 || index > log_count) {
            printf("Error: Invalid log index %d (valid range: 1-%d)\n", index, log_count);
            return;
        }
        
        // Convert from 1-indexed newest-to-oldest to array index
        // Index 1 = newest = (log_start + log_count - 1)
        // Index 2 = second newest = (log_start + log_count - 2)
        // etc.
        int array_index = (log_start + log_count - index) % MAX_LOG_ENTRIES;
        char *command_to_execute = command_log[array_index];
        
        printf("%s\n", command_to_execute); // Print the command being executed
        
        // Check if we're in a pipeline context by checking if stdout is not a terminal
        int in_pipeline = !isatty(STDOUT_FILENO);
        
        // Save current shell state only if not in pipeline (to avoid issues in child processes)
        Token saved_tokens[1024];
        int saved_token_count = token_count;
        const char* saved_current_input = current_input;
        ParserState saved_parser_state = parser_state;
        
        if (!in_pipeline) {
            // Copy current tokens to saved state
            memcpy(saved_tokens, tokens, sizeof(Token) * token_count);
        }
        
        // Tokenize and execute the stored command
        current_input = command_to_execute;
        tokenise();
        
        // Parse and execute (similar to main loop, but don't log this execution)
        int parse_result = parse();
        if (parse_result) {
            if (has_pipes()) {
                execute_pipeline(home_directory);
            } else {
                // Check for built-in commands
                if (token_count > 0 && tokens[0].type == TOKEN_NAME) {
                    if (strcmp(tokens[0].value, "hop") == 0) {
                        execute_hop(home_directory);
                    } else if (strcmp(tokens[0].value, "reveal") == 0) {
                        execute_reveal(home_directory);
                    } else if (strcmp(tokens[0].value, "exit") == 0) {
                        printf("Shell terminated.\n");
                        // Only exit if not in pipeline context
                        if (!in_pipeline) {
                            exit(0);
                        }
                    } else {
                        // If in pipeline, redirect stdin from /dev/null for commands that expect input
                        if (in_pipeline) {
                            int null_fd = open("/dev/null", O_RDONLY);
                            if (null_fd != -1) {
                                int saved_stdin = dup(STDIN_FILENO);
                                dup2(null_fd, STDIN_FILENO);
                                close(null_fd);
                                
                                execute_command();
                                
                                dup2(saved_stdin, STDIN_FILENO);
                                close(saved_stdin);
                            } else {
                                execute_command();
                            }
                        } else {
                            execute_command();
                        }
                    }
                }
            }
        } else {
            printf("Invalid Syntax!\n");
        }
        
        // Restore shell state only if not in pipeline
        if (!in_pipeline) {
            memcpy(tokens, saved_tokens, sizeof(Token) * saved_token_count);
            token_count = saved_token_count;
            current_input = saved_current_input;
            parser_state = saved_parser_state;
        }
        
    } else {
        printf("Usage: log [purge | execute <index>]\n");
    }
}
