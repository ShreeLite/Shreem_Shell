#include "shell.h"
#include <sys/wait.h>

//LLM Generated Code Begins Here
/**
 * @brief Find segments of commands separated by semicolons and ampersands
 * @param segments Pointer to array that will hold segment boundaries (will be allocated)
 * @param num_segments Pointer to store number of segments found
 * @param separators Pointer to array that will hold the separators between segments
 * @return 0 on success, -1 on error
 * 
 * Each segment is represented by [start_index, end_index] pairs
 * Separators array contains the TokenType that comes after each segment
 * Caller is responsible for freeing the segments and separators arrays
 */
int find_command_segments(int **segments, int *num_segments, TokenType **separators) {
    *num_segments = 1; // At least one command
    
    // Count the number of semicolons and ampersands to determine number of segments
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_SEMICOLON || tokens[i].type == TOKEN_AMPERSAND) {
            (*num_segments)++;
        }
    }
    
    // Allocate memory for segment boundaries (2 integers per segment: start, end)
    *segments = malloc(*num_segments * 2 * sizeof(int));
    if (*segments == NULL) {
        perror("malloc segments");
        return -1;
    }
    
    // Allocate memory for separators (one less than segments, plus one for end marker)
    *separators = malloc(*num_segments * sizeof(TokenType));
    if (*separators == NULL) {
        perror("malloc separators");
        free(*segments);
        return -1;
    }
    
    int segment_index = 0;
    int current_start = 0;
    
    for (int i = 0; i <= token_count; i++) {
        // Found a separator or reached end of tokens
        if (i == token_count || tokens[i].type == TOKEN_SEMICOLON || tokens[i].type == TOKEN_AMPERSAND) {
            (*segments)[segment_index * 2] = current_start;     // start index
            (*segments)[segment_index * 2 + 1] = i;             // end index
            
            if (i < token_count) {
                (*separators)[segment_index] = tokens[i].type;  // store separator type
            } else {
                (*separators)[segment_index] = TOKEN_END;       // no separator (end of input)
            }
            
            segment_index++;
            current_start = i + 1; // Next segment starts after the separator
        }
    }
    
    return 0;
}

/**
 * @brief Reconstruct command string from token segment
 * @param start_index Starting token index
 * @param end_index Ending token index
 * @return Dynamically allocated string containing the command (caller must free)
 */
char* reconstruct_command_from_segment(int start_index, int end_index) {
    if (start_index >= end_index) {
        return NULL;
    }
    
    // Calculate required buffer size
    int total_len = 0;
    for (int i = start_index; i < end_index; i++) {
        total_len += strlen(tokens[i].value) + 1; // +1 for space or null terminator
    }
    
    char *command = malloc(total_len + 1);
    if (command == NULL) {
        return NULL;
    }
    
    command[0] = '\0';
    
    // Reconstruct command with proper spacing
    for (int i = start_index; i < end_index; i++) {
        if (i > start_index) {
            strcat(command, " ");
        }
        strcat(command, tokens[i].value);
    }
    
    return command;
}

/**
 * @brief Execute a single command segment
 * @param start_index Starting token index
 * @param end_index Ending token index
 * @param home_directory Shell home directory for built-in commands
 * @return 0 on success, -1 on error
 */
int execute_single_segment(int start_index, int end_index, const char* home_directory) {
    if (start_index >= end_index) {
        return 0; // Empty segment
    }
    
    // Create a temporary tokens array for this segment
    Token original_tokens[1024];
    int original_count = token_count;
    
    // Save original tokens
    memcpy(original_tokens, tokens, sizeof(Token) * token_count);
    
    // Copy segment tokens to the beginning of tokens array
    int segment_size = end_index - start_index;
    for (int i = 0; i < segment_size; i++) {
        tokens[i] = original_tokens[start_index + i];
    }
    tokens[segment_size].type = TOKEN_END;
    tokens[segment_size].value[0] = '\0';
    token_count = segment_size;
    
    int result = 0;
    
    // Check if the segment contains pipes
    if (has_pipes()) {
        // Execute as pipeline
        result = execute_pipeline(home_directory);
    } else {
        // Check for built-in commands
        if (token_count > 0 && tokens[0].type == TOKEN_NAME) {
            if (strcmp(tokens[0].value, "hop") == 0) {
                execute_hop(home_directory);
                result = 0;
            } else if (strcmp(tokens[0].value, "reveal") == 0) {
                // Save original stdout for built-in commands with redirection
                int saved_stdout = -1;
                if (has_output_redirection(0, token_count)) {
                    saved_stdout = dup(STDOUT_FILENO);
                    if (setup_output_redirection(0, token_count) == -1) {
                        if (saved_stdout != -1) close(saved_stdout);
                        result = -1;
                    } else {
                        execute_reveal(home_directory);
                        // Restore original stdout
                        dup2(saved_stdout, STDOUT_FILENO);
                        close(saved_stdout);
                        result = 0;
                    }
                } else {
                    execute_reveal(home_directory);
                    result = 0;
                }
            } else if (strcmp(tokens[0].value, "log") == 0) {
                execute_log(home_directory);
                result = 0;
            } else if (strcmp(tokens[0].value, "activities") == 0) {
                // Save original stdout for built-in commands with redirection
                int saved_stdout = -1;
                if (has_output_redirection(0, token_count)) {
                    saved_stdout = dup(STDOUT_FILENO);
                    if (setup_output_redirection(0, token_count) == -1) {
                        if (saved_stdout != -1) close(saved_stdout);
                        result = -1;
                    } else {
                        execute_activities();
                        // Restore original stdout
                        dup2(saved_stdout, STDOUT_FILENO);
                        close(saved_stdout);
                        result = 0;
                    }
                } else {
                    execute_activities();
                    result = 0;
                }
            } else if (strcmp(tokens[0].value, "ping") == 0) {
                execute_ping();
                result = 0;
            } else if (strcmp(tokens[0].value, "fg") == 0) {
                execute_fg();
                result = 0;
            } else if (strcmp(tokens[0].value, "bg") == 0) {
                execute_bg();
                result = 0;
            } else if (strcmp(tokens[0].value, "exit") == 0) {
                printf("Shell terminated.\n");
                exit(0);
            } else {
                // Execute external command
                result = execute_command();
            }
        }
    }
    
    // Restore original tokens
    memcpy(tokens, original_tokens, sizeof(Token) * original_count);
    token_count = original_count;
    
    return result;
}

/**
 * @brief Execute sequential commands separated by semicolons and handle background execution
 * @return 0 on success, -1 on error
 * 
 * Requirements implemented:
 * - Sequential execution with semicolon operator
 * - Background execution with ampersand operator
 * - Each command executes in order
 * - Shell waits for each foreground command to complete
 * - Background commands don't block shell execution
 * - Continue execution even if a command fails
 */
int execute_sequential_commands(const char* home_directory) {
    if (token_count == 0) {
        return 0;
    }
    
    // Find command segments separated by ; and &
    int *segments = NULL;
    int num_segments = 0;
    TokenType *separators = NULL;
    
    if (find_command_segments(&segments, &num_segments, &separators) == -1) {
        return -1;
    }
    
    // Execute each command segment
    for (int i = 0; i < num_segments; i++) {
        int start_index = segments[i * 2];
        int end_index = segments[i * 2 + 1];
        TokenType separator = separators[i];
        
        if (start_index >= end_index) {
            continue; // Skip empty segments
        }
        
        // Reconstruct command string for logging/background tracking
        char *command_str = reconstruct_command_from_segment(start_index, end_index);
        
        if (separator == TOKEN_AMPERSAND) {
            // Execute in background
            if (command_str) {
                execute_background_command(start_index, end_index, command_str);
            }
        } else {
            // Execute in foreground (sequential)
            execute_single_segment(start_index, end_index, home_directory);
            
            // Log successful commands (but continue even if command failed)
            if (command_str && should_log_command(command_str)) {
                add_command_to_log(command_str);
            }
        }
        
        // Free command string
        if (command_str) {
            free(command_str);
        }
        
        // Note: We continue executing subsequent commands even if current command failed
        // This is the required behavior for sequential execution
    }
    
    free(segments);
    free(separators);
    return 0;
}

//LLM Generated Code ends here